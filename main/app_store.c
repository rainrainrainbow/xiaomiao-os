/*
 * app_store.c — Application store (PoC) implementation
 *
 * Scans /sdcard/store/*.app, parses manifest.json from each, and presents
 * them as installable items. Installation copies the .app to /sdcard/apps/
 * and registers it via app_manager.
 */
#include "app_store.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *TAG = "STORE";

static store_catalog_t s_catalog;

/* ── Internal: parse manifest.json from a .app file ───────────────────── */

/* Minimal ZIP reader (reuses logic from app_manager.c but simplified) */
#define ZIP_LOCAL_SIG     0x04034B50
#define ZIP_CENTRAL_SIG   0x02014B50
#define ZIP_END_SIG       0x06054B50
#define ZIP_METHOD_STORED 0

static uint16_t zip_read_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t zip_read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static long zip_find_eocd(const uint8_t *buf, size_t size)
{
    const size_t max_back = 65557;
    size_t start = (size > max_back) ? size - max_back : 0;
    for (size_t i = size; i > start; i--) {
        if (i >= 4 && zip_read_u32(buf + i - 4) == ZIP_END_SIG) {
            return (long)(i - 4);
        }
    }
    return -1;
}

static bool zip_find_manifest(const uint8_t *buf, size_t size,
                              uint32_t *out_offset, uint32_t *out_size)
{
    long eocd_off = zip_find_eocd(buf, size);
    if (eocd_off < 0) return false;
    const uint8_t *eocd = buf + eocd_off;
    uint16_t n_entries = zip_read_u16(eocd + 10);
    uint32_t cd_off    = zip_read_u32(eocd + 16);

    const uint8_t *p = buf + cd_off;
    for (uint16_t i = 0; i < n_entries; i++) {
        if ((size_t)(p - buf) + 46 > size) break;
        if (zip_read_u32(p) != ZIP_CENTRAL_SIG) break;
        uint16_t method = zip_read_u16(p + 10);
        uint32_t compsize = zip_read_u32(p + 20);
        uint32_t uncomp   = zip_read_u32(p + 24);
        uint16_t namelen  = zip_read_u16(p + 28);
        uint16_t extralen = zip_read_u16(p + 30);
        uint16_t commentlen = zip_read_u16(p + 32);
        uint32_t local_off  = zip_read_u32(p + 42);
        const uint8_t *name = p + 46;

        if (namelen == 11 && memcmp(name, "manifest.json", 11) == 0 && method == ZIP_METHOD_STORED) {
            *out_offset = local_off;
            *out_size   = uncomp ? uncomp : compsize;
            return true;
        }
        p += 46 + namelen + extralen + commentlen;
    }
    return false;
}

/* Minimal JSON parser for manifest (same as app_manager.c) */
typedef struct {
    const char *json;
    int len;
    int pos;
} json_ctx_t;

static void json_skip_ws(json_ctx_t *c)
{
    while (c->pos < c->len) {
        char ch = c->json[c->pos];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') c->pos++;
        else break;
    }
}

static bool json_expect(json_ctx_t *c, char ch)
{
    json_skip_ws(c);
    if (c->pos < c->len && c->json[c->pos] == ch) {
        c->pos++;
        return true;
    }
    return false;
}

static bool json_read_string(json_ctx_t *c, char *dst, size_t dst_len)
{
    if (!json_expect(c, '"')) return false;
    size_t i = 0;
    while (c->pos < c->len && c->json[c->pos] != '"') {
        char ch = c->json[c->pos];
        if (ch == '\\' && c->pos + 1 < c->len) {
            c->pos++;
            char esc = c->json[c->pos];
            switch (esc) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case '"': ch = '"'; break;
                case '\\': ch = '\\'; break;
                default: ch = esc; break;
            }
        }
        if (i + 1 < dst_len) dst[i++] = ch;
        c->pos++;
    }
    dst[i] = '\0';
    if (!json_expect(c, '"')) return false;
    return true;
}

static bool json_read_key(json_ctx_t *c, char *dst, size_t dst_len)
{
    json_skip_ws(c);
    if (c->pos < c->len && c->json[c->pos] == '}') return false;
    return json_read_string(c, dst, dst_len);
}

static bool parse_manifest(const char *json, int len, store_item_t *item)
{
    json_ctx_t ctx = { json, len, 0 };
    char key[32];
    char val[APP_ID_MAX_LEN];

    if (!json_expect(&ctx, '{')) return false;
    while (json_read_key(&ctx, key, sizeof(key))) {
        if (!json_expect(&ctx, ':')) return false;
        if (!json_read_string(&ctx, val, sizeof(val))) {
            json_skip_ws(&ctx);
            while (ctx.pos < ctx.len && ctx.json[ctx.pos] != ',' && ctx.json[ctx.pos] != '}') ctx.pos++;
            json_expect(&ctx, ',');
            continue;
        }
        if (strcmp(key, "name") == 0) {
            strncpy(item->display_name, val, APP_NAME_MAX_LEN - 1);
        } else if (strcmp(key, "version") == 0) {
            strncpy(item->version, val, APP_VERSION_MAX_LEN - 1);
        } else if (strcmp(key, "icon") == 0) {
            if (val[0] != '\0') {
                int bytes = 1;
                unsigned char c0 = (unsigned char)val[0];
                if      ((c0 & 0x80) == 0)    bytes = 1;
                else if ((c0 & 0xE0) == 0xC0) bytes = 2;
                else if ((c0 & 0xF0) == 0xE0) bytes = 3;
                else if ((c0 & 0xF8) == 0xF0) bytes = 4;
                if (bytes == 1) {
                    item->icon_glyph = (uint8_t)val[0];
                } else if (bytes <= (int)sizeof(item->icon_str) - 1) {
                    strncpy(item->icon_str, val, bytes);
                    item->icon_str[bytes] = '\0';
                }
            }
        }
        json_skip_ws(&ctx);
        if (!json_expect(&ctx, ',')) break;
    }
    json_skip_ws(&ctx);
    json_expect(&ctx, '}');
    return item->display_name[0] != '\0';
}

static uint8_t *read_file_all(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 2 * 1024 * 1024) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *out_size = (size_t)sz;
    return buf;
}

/* ── Public API ───────────────────────────────────────────────────────── */

void app_store_init(void)
{
    memset(&s_catalog, 0, sizeof(s_catalog));
    ESP_LOGI(TAG, "App store initialized");
}

int app_store_scan(void)
{
    int found = 0;
    s_catalog.count = 0;

    DIR *dir = opendir("/sdcard/store");
    if (!dir) {
        ESP_LOGW(TAG, "/sdcard/store not accessible");
        mkdir("/sdcard/store", 0755);
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL && s_catalog.count < STORE_MAX_ITEMS) {
        const char *name = de->d_name;
        size_t nlen = strlen(name);
        if (nlen < 5) continue;
        if (strcmp(name + nlen - 4, ".app") != 0) continue;

        char full_path[APP_PATH_MAX_LEN];
        snprintf(full_path, sizeof(full_path), "/sdcard/store/%s", name);

        size_t fsz = 0;
        uint8_t *fbuf = read_file_all(full_path, &fsz);
        if (!fbuf) continue;

        uint32_t manifest_off = 0, manifest_sz = 0;
        if (!zip_find_manifest(fbuf, fsz, &manifest_off, &manifest_sz)) {
            free(fbuf);
            continue;
        }

        if (manifest_off + 30 > fsz || zip_read_u32(fbuf + manifest_off) != ZIP_LOCAL_SIG) {
            free(fbuf);
            continue;
        }
        uint16_t namelen  = zip_read_u16(fbuf + manifest_off + 26);
        uint16_t extralen = zip_read_u16(fbuf + manifest_off + 28);
        uint32_t data_off = manifest_off + 30 + namelen + extralen;

        if (data_off + manifest_sz > fsz) {
            free(fbuf);
            continue;
        }

        store_item_t *item = &s_catalog.items[s_catalog.count];
        memset(item, 0, sizeof(*item));
        strncpy(item->filename, name, sizeof(item->filename) - 1);
        strncpy(item->full_path, full_path, sizeof(item->full_path) - 1);
        item->file_size = (uint32_t)fsz;

        if (parse_manifest((const char *)fbuf + data_off, (int)manifest_sz, item)) {
            item->valid = true;
            item->installed = app_store_is_installed(item);
            s_catalog.count++;
            found++;
            ESP_LOGI(TAG, "Store item: %s (%s)", item->display_name, item->version);
        }
        free(fbuf);
    }
    closedir(dir);

    ESP_LOGI(TAG, "Scanned /sdcard/store: %d items", s_catalog.count);
    return found;
}

const store_catalog_t *app_store_get_catalog(void)
{
    return &s_catalog;
}

int app_store_get_count(void)
{
    return s_catalog.count;
}

const store_item_t *app_store_get_item(int index)
{
    if (index < 0 || index >= s_catalog.count) return NULL;
    return &s_catalog.items[index];
}

bool app_store_is_installed(const store_item_t *item)
{
    if (!item) return false;
    /* Check if the same .app file exists in /sdcard/apps/ */
    char dst_path[APP_PATH_MAX_LEN];
    snprintf(dst_path, sizeof(dst_path), "/sdcard/apps/%s", item->filename);
    struct stat st;
    return stat(dst_path, &st) == 0;
}

bool app_store_install(int index)
{
    const store_item_t *item = app_store_get_item(index);
    if (!item || !item->valid) return false;

    /* Ensure /sdcard/apps exists */
    mkdir("/sdcard/apps", 0755);

    /* Copy src -> /sdcard/apps/<filename> */
    char dst_path[APP_PATH_MAX_LEN];
    snprintf(dst_path, sizeof(dst_path), "/sdcard/apps/%s", item->filename);

    FILE *src = fopen(item->full_path, "rb");
    if (!src) {
        ESP_LOGE(TAG, "Install: cannot open %s", item->full_path);
        return false;
    }
    FILE *out = fopen(dst_path, "wb");
    if (!out) {
        fclose(src);
        ESP_LOGE(TAG, "Install: cannot write %s", dst_path);
        return false;
    }
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, out);
    }
    fclose(src);
    fclose(out);

    /* Register via app_manager */
    bool ok = app_manager_install_from_path(dst_path);
    if (ok) {
        /* Update installed flag */
        s_catalog.items[index].installed = true;
        ESP_LOGI(TAG, "Installed %s from store", item->display_name);
    }
    return ok;
}