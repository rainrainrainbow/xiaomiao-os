/*
 * app_manager.c — Application registry & lifecycle implementation
 *
 * Architecture:
 *   - Single global registry s_app_registry
 *   - System apps (kind=SYSTEM) are pre-registered; cannot be uninstalled
 *   - User apps (kind=USER) are loaded by scanning /sdcard/apps/ *.app and
 *     extracting manifest.json via miniz-free ZIP reader
 *   - Desktop ordering is persisted to /vfs/desktop.json
 *   - Settings (brightness/volume/wifi) are persisted to /vfs/settings.json
 *
 * ZIP parsing strategy:
 *   Since miniz is large and the .app format is constrained (manifest.json is
 *   always the first stored entry, never compressed), we read the central
 *   directory at the end of the file, locate manifest.json's local file
 *   header, then extract just that entry using raw stored-mode DEFLATE=0.
 *
 *   This avoids pulling in zlib for read-only access.
 */
#include "app_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static const char *TAG = "APP_MGR";

static app_registry_t s_app_registry;

/* ── Built-in system apps ─────────────────────────────────────────────── */

static const struct {
    const char *package;
    const char *name;
    const char *version;
    uint8_t     icon_glyph;
    const char *icon_str;
} s_system_apps[] = {
    { "sys.applist",    "Apps",     "1.0", 'A', "" },
    { "sys.settings",   "Settings", "1.0", 'S', "" },
    { "sys.editor",     "Editor",   "1.0", 'E', "" },
    { "sys.store",      "Store",    "1.0", 'T', "" },
    { "sys.about",      "About",    "1.0", 'i', "" },
};
#define NUM_SYSTEM_APPS (sizeof(s_system_apps) / sizeof(s_system_apps[0]))

/* ── Registry access ──────────────────────────────────────────────────── */

app_registry_t *app_manager_get_registry(void)
{
    return &s_app_registry;
}

int app_manager_count(void)
{
    return s_app_registry.count;
}

const app_entry_t *app_manager_get(int index)
{
    if (index < 0 || index >= s_app_registry.count) return NULL;
    return &s_app_registry.apps[index];
}

const app_entry_t *app_manager_find(const char *package_name)
{
    if (package_name == NULL) return NULL;
    for (int i = 0; i < s_app_registry.count; i++) {
        if (strcmp(s_app_registry.apps[i].package_name, package_name) == 0)
            return &s_app_registry.apps[i];
    }
    return NULL;
}

/* ── Initialization ──────────────────────────────────────────────────── */

static void register_system_apps(void)
{
    s_app_registry.sys_count = 0;
    for (size_t i = 0; i < NUM_SYSTEM_APPS && i < APP_MAX_COUNT; i++) {
        app_entry_t *e = &s_app_registry.apps[i];
        memset(e, 0, sizeof(*e));
        e->kind = APP_KIND_SYSTEM;
        e->is_system = true;
        e->valid = true;
        e->icon_glyph = s_system_apps[i].icon_glyph;
        strncpy(e->package_name, s_system_apps[i].package, APP_ID_MAX_LEN - 1);
        strncpy(e->display_name, s_system_apps[i].name, APP_NAME_MAX_LEN - 1);
        strncpy(e->version, s_system_apps[i].version, APP_VERSION_MAX_LEN - 1);
        strncpy(e->icon_str, s_system_apps[i].icon_str, sizeof(e->icon_str) - 1);
        strncpy(e->entry_point, "main.py", sizeof(e->entry_point) - 1);
        s_app_registry.sys_count++;
    }
    s_app_registry.count = s_app_registry.sys_count;

    /* Initialize identity permutation */
    for (int i = 0; i < APP_MAX_COUNT; i++) {
        s_app_registry.order[i] = i;
    }
}

void app_manager_init(void)
{
    memset(&s_app_registry, 0, sizeof(s_app_registry));
    register_system_apps();

    /* Try to load saved order */
    app_manager_load_order();

    ESP_LOGI(TAG, "App manager initialized: %d system apps", s_app_registry.sys_count);
}

/* ── JSON parsing (tiny hand-rolled parser) ───────────────────────────── *
 *
 * We do NOT pull in a full JSON library. Manifests are small and constrained:
 *   {
 *     "id": "com.example.app",
 *     "name": "My App",
 *     "version": "1.0.0",
 *     "entry": "main.py",
 *     "icon": "G"           // single char OR emoji
 *   }
 *
 * The parser extracts string values for known keys. Unknown keys ignored.
 * No nesting (manifests are flat).
 */

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

/* Extract string value starting at c->pos pointing to opening quote.
 * Writes up to dst_len-1 chars to dst, null-terminates.
 * Advances c->pos past closing quote. Returns false on malformed input.
 */
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

/* Read next key (string). Returns false if next non-ws char is '}' (end of obj). */
static bool json_read_key(json_ctx_t *c, char *dst, size_t dst_len)
{
    json_skip_ws(c);
    if (c->pos < c->len && c->json[c->pos] == '}') return false;
    return json_read_string(c, dst, dst_len);
}

static bool json_parse_manifest(const char *json, int len, app_entry_t *e)
{
    json_ctx_t ctx = { json, len, 0 };
    char key[32];
    char val[APP_ID_MAX_LEN];

    if (!json_expect(&ctx, '{')) return false;
    while (json_read_key(&ctx, key, sizeof(key))) {
        if (!json_expect(&ctx, ':')) return false;
        /* value can be string */
        if (!json_read_string(&ctx, val, sizeof(val))) {
            /* skip non-string values (numbers, etc.) */
            json_skip_ws(&ctx);
            while (ctx.pos < ctx.len && ctx.json[ctx.pos] != ',' && ctx.json[ctx.pos] != '}') ctx.pos++;
            json_expect(&ctx, ',');
            continue;
        }
        if (strcmp(key, "id") == 0) {
            strncpy(e->package_name, val, APP_ID_MAX_LEN - 1);
        } else if (strcmp(key, "name") == 0) {
            strncpy(e->display_name, val, APP_NAME_MAX_LEN - 1);
        } else if (strcmp(key, "version") == 0) {
            strncpy(e->version, val, APP_VERSION_MAX_LEN - 1);
        } else if (strcmp(key, "entry") == 0) {
            strncpy(e->entry_point, val, sizeof(e->entry_point) - 1);
        } else if (strcmp(key, "icon") == 0) {
            /* may be a single char or short emoji */
            if (val[0] != '\0') {
                /* count UTF-8 bytes to detect multibyte */
                int bytes = 1;
                unsigned char c0 = (unsigned char)val[0];
                if      ((c0 & 0x80) == 0)    bytes = 1;
                else if ((c0 & 0xE0) == 0xC0) bytes = 2;
                else if ((c0 & 0xF0) == 0xE0) bytes = 3;
                else if ((c0 & 0xF8) == 0xF0) bytes = 4;
                if (bytes == 1) {
                    e->icon_glyph = (uint8_t)val[0];
                } else if (bytes <= (int)sizeof(e->icon_str) - 1) {
                    strncpy(e->icon_str, val, bytes);
                    e->icon_str[bytes] = '\0';
                }
            }
        }
        json_skip_ws(&ctx);
        if (!json_expect(&ctx, ',')) {
            /* last key-value pair */
            break;
        }
    }
    json_skip_ws(&ctx);
    json_expect(&ctx, '}');
    /* required fields */
    return e->package_name[0] != '\0';
}

/* ── ZIP reader (raw DEFLATE=0 only, manifest.json only) ──────────────── */

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

/* Find End-of-Central-Directory record by scanning backwards.
 * Returns offset in file, or -1 if not found.
 */
static long zip_find_eocd(const uint8_t *buf, size_t size)
{
    const size_t max_back = 65557; /* EOCD comment <= 65535 bytes */
    size_t start = (size > max_back) ? size - max_back : 0;
    for (size_t i = size; i > start; i--) {
        if (i >= 4 && zip_read_u32(buf + i - 4) == ZIP_END_SIG) {
            return (long)(i - 4);
        }
    }
    return -1;
}

/* Search central directory for filename; return entry offset (local header) and size. */
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

/* Read entire file into malloc'd buffer. Caller frees. Returns NULL on error. */
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

/* Try to register a single .app file */
static bool register_app_file(const char *app_path)
{
    size_t fsz = 0;
    uint8_t *fbuf = read_file_all(app_path, &fsz);
    if (!fbuf) {
        ESP_LOGW(TAG, "Cannot read %s", app_path);
        return false;
    }

    uint32_t manifest_off = 0, manifest_sz = 0;
    if (!zip_find_manifest(fbuf, fsz, &manifest_off, &manifest_sz)) {
        ESP_LOGW(TAG, "No manifest.json in %s", app_path);
        free(fbuf);
        return false;
    }

    /* Verify local file header */
    if (manifest_off + 30 > fsz || zip_read_u32(fbuf + manifest_off) != ZIP_LOCAL_SIG) {
        ESP_LOGW(TAG, "Bad local header in %s", app_path);
        free(fbuf);
        return false;
    }
    uint16_t namelen  = zip_read_u16(fbuf + manifest_off + 26);
    uint16_t extralen = zip_read_u16(fbuf + manifest_off + 28);
    uint32_t data_off = manifest_off + 30 + namelen + extralen;

    if (data_off + manifest_sz > fsz) {
        ESP_LOGW(TAG, "Truncated manifest in %s", app_path);
        free(fbuf);
        return false;
    }

    /* Parse manifest */
    app_entry_t e = { 0 };
    if (!json_parse_manifest((const char *)fbuf + data_off, (int)manifest_sz, &e)) {
        ESP_LOGW(TAG, "Bad manifest JSON in %s", app_path);
        free(fbuf);
        return false;
    }

    /* Dedup: if package already registered, skip */
    if (app_manager_find(e.package_name) != NULL) {
        free(fbuf);
        return false;
    }

    if (s_app_registry.count >= APP_MAX_COUNT) {
        ESP_LOGW(TAG, "Registry full");
        free(fbuf);
        return false;
    }

    e.kind       = APP_KIND_USER;
    e.is_system  = false;
    e.valid      = true;
    strncpy(e.entry_point, e.entry_point[0] ? e.entry_point : "main.py", sizeof(e.entry_point) - 1);
    strncpy(e.app_path, app_path, APP_PATH_MAX_LEN - 1);

    /* data_path = /sdcard/data/<package> */
    snprintf(e.data_path, APP_PATH_MAX_LEN, "/sdcard/data/%s", e.package_name);

    /* file_size */
    struct stat st;
    if (stat(app_path, &st) == 0) {
        e.file_size = (uint32_t)st.st_size;
    }

    /* default icon glyph if empty */
    if (e.icon_glyph == 0 && e.icon_str[0] == '\0') {
        e.icon_glyph = (uint8_t)e.display_name[0];
    }

    int idx = s_app_registry.count++;
    s_app_registry.apps[idx] = e;
    free(fbuf);

    ESP_LOGI(TAG, "Registered %s v%s (%s)", e.display_name, e.version, e.package_name);
    return true;
}

int app_manager_scan_sdcard(void)
{
    int found = 0;
    int before = s_app_registry.count;

    DIR *dir = opendir("/sdcard/apps");
    if (!dir) {
        ESP_LOGW(TAG, "/sdcard/apps not accessible (SD card not mounted?)");
        mkdir("/sdcard/apps", 0755);
        return 0;
    }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        const char *name = de->d_name;
        size_t nlen = strlen(name);
        if (nlen < 5) continue;
        if (strcmp(name + nlen - 4, ".app") != 0) continue;

        char full_path[APP_PATH_MAX_LEN];
        snprintf(full_path, sizeof(full_path), "/sdcard/apps/%s", name);
        if (register_app_file(full_path)) found++;
    }
    closedir(dir);

    ESP_LOGI(TAG, "Scanned /sdcard/apps: +%d new (total %d)", found, s_app_registry.count - before);
    return found;
}

/* ── Desktop ordering persistence (/vfs/desktop.json) ─────────────────── *
 *
 * JSON format (handwritten, no escaping needed since names are alphanumeric):
 *   {"order":["sys.applist","sys.settings",...],"selected":0}
 */

void app_manager_save_order(void)
{
    FILE *f = fopen("/vfs/desktop.json", "w");
    if (!f) {
        ESP_LOGW(TAG, "Cannot write /vfs/desktop.json");
        return;
    }
    fputs("{\"order\":[", f);
    for (int i = 0; i < s_app_registry.count; i++) {
        if (i > 0) fputc(',', f);
        fputc('"', f);
        fputs(s_app_registry.apps[s_app_registry.order[i]].package_name, f);
        fputc('"', f);
    }
    fputs("]}", f);
    fclose(f);
    ESP_LOGI(TAG, "Saved /vfs/desktop.json (%d entries)", s_app_registry.count);
}

void app_manager_load_order(void)
{
    FILE *f = fopen("/vfs/desktop.json", "r");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 4096) { fclose(f); return; }
    char buf[4096];
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); return; }
    buf[sz] = '\0';
    fclose(f);

    /* Find "order":[ */
    const char *p = strstr(buf, "\"order\"");
    if (!p) return;
    p = strchr(p, '[');
    if (!p) return;
    p++;

    int new_order[APP_MAX_COUNT];
    int n = 0;
    while (*p && *p != ']' && n < APP_MAX_COUNT) {
        while (*p && (*p == ' ' || *p == ',' || *p == '\n')) p++;
        if (*p != '"') break;
        p++;
        char pkg[APP_ID_MAX_LEN];
        int i = 0;
        while (*p && *p != '"' && i < APP_ID_MAX_LEN - 1) {
            pkg[i++] = *p++;
        }
        pkg[i] = '\0';
        if (*p == '"') p++;

        /* find this package index */
        int found = -1;
        for (int k = 0; k < s_app_registry.count; k++) {
            if (strcmp(s_app_registry.apps[k].package_name, pkg) == 0) {
                found = k;
                break;
            }
        }
        if (found >= 0) new_order[n++] = found;
    }

    /* Append any apps not in the saved order */
    for (int k = 0; k < s_app_registry.count; k++) {
        bool seen = false;
        for (int j = 0; j < n; j++) {
            if (new_order[j] == k) { seen = true; break; }
        }
        if (!seen) new_order[n++] = k;
    }

    if (n > 0) {
        memcpy(s_app_registry.order, new_order, sizeof(int) * n);
        ESP_LOGI(TAG, "Loaded /vfs/desktop.json (%d entries)", n);
    }
}

void app_manager_move(int from_index, int to_index)
{
    if (from_index < 0 || from_index >= s_app_registry.count) return;
    if (to_index   < 0 || to_index   >= s_app_registry.count) return;
    if (from_index == to_index) return;

    int v = s_app_registry.order[from_index];
    if (from_index < to_index) {
        memmove(&s_app_registry.order[from_index],
                &s_app_registry.order[from_index + 1],
                sizeof(int) * (to_index - from_index));
    } else {
        memmove(&s_app_registry.order[to_index + 1],
                &s_app_registry.order[to_index],
                sizeof(int) * (from_index - to_index));
    }
    s_app_registry.order[to_index] = v;

    app_manager_save_order();
}

/* ── Install / Uninstall ──────────────────────────────────────────────── */

bool app_manager_install_from_path(const char *app_file_path)
{
    if (!app_file_path) return false;

    /* Try registering directly (covers case where it's already in /sdcard/apps) */
    if (register_app_file(app_file_path)) {
        app_manager_save_order();
        return true;
    }

    /* Copy src -> /sdcard/apps/<basename> */
    const char *base = strrchr(app_file_path, '/');
    base = base ? base + 1 : app_file_path;
    char dst[APP_PATH_MAX_LEN];
    snprintf(dst, sizeof(dst), "/sdcard/apps/%s", base);

    FILE *src = fopen(app_file_path, "rb");
    if (!src) { ESP_LOGE(TAG, "Install: cannot open src"); return false; }
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(src); ESP_LOGE(TAG, "Install: cannot write dst"); return false; }
    char buf[1024]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, n, out);
    fclose(src); fclose(out);

    bool ok = register_app_file(dst);
    if (ok) app_manager_save_order();
    return ok;
}

bool app_manager_uninstall(const char *package_name)
{
    if (!package_name) return false;
    int idx = -1;
    for (int i = 0; i < s_app_registry.count; i++) {
        if (strcmp(s_app_registry.apps[i].package_name, package_name) == 0) {
            idx = i; break;
        }
    }
    if (idx < 0) return false;
    if (s_app_registry.apps[idx].is_system) {
        ESP_LOGW(TAG, "Cannot uninstall system app %s", package_name);
        return false;
    }

    /* Remove .app file */
    if (s_app_registry.apps[idx].app_path[0]) {
        unlink(s_app_registry.apps[idx].app_path);
    }

    /* Shift apps array */
    memmove(&s_app_registry.apps[idx], &s_app_registry.apps[idx + 1],
            sizeof(app_entry_t) * (s_app_registry.count - idx - 1));
    s_app_registry.count--;

    /* Rebuild order permutation: iterate only over the active prefix
     * (registry slots beyond count may contain stale indices). */
    int new_order[APP_MAX_COUNT];
    int n = 0;
    for (int i = 0; i < s_app_registry.count; i++) {
        int oi = s_app_registry.order[i];
        if (oi == idx) continue;
        if (oi > idx) oi--;
        new_order[n++] = oi;
    }
    /* Pad the rest with identity to keep invariants (unused slots) */
    for (int i = n; i < APP_MAX_COUNT; i++) new_order[i] = i;
    memcpy(s_app_registry.order, new_order, sizeof(s_app_registry.order));

    app_manager_save_order();
    ESP_LOGI(TAG, "Uninstalled %s", package_name);
    return true;
}

/* ── Launch ───────────────────────────────────────────────────────────── */

bool app_manager_launch_index(int index)
{
    if (index < 0 || index >= s_app_registry.count) return false;
    const app_entry_t *app = &s_app_registry.apps[index];

    if (app->is_system) {
        /* System apps are handled by ui_main.c directly */
        ESP_LOGI(TAG, "System app launch: %s -> handled by UI", app->package_name);
        return true;
    }

    ESP_LOGI(TAG, "Launch user app: %s -> MicroPython VM", app->package_name);
    
    /* Hand off to MicroPython runtime */
    extern bool micropython_launch_app(const app_entry_t *app);
    return micropython_launch_app(app);
}

bool app_manager_launch(const char *package_name)
{
    const app_entry_t *app = app_manager_find(package_name);
    if (!app) return false;
    int idx = (int)(app - s_app_registry.apps);
    return app_manager_launch_index(idx);
}