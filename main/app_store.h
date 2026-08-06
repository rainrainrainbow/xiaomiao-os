/*
 * app_store.h — Application store (PoC)
 *
 * Scope: a local app-store concept that scans /sdcard/store/ *.app and lets
 * the user install any of them into /sdcard/apps/. This is a PoC; the real
 * "store" would download from a remote URL over WiFi (out of scope here).
 *
 * UI integration: the store page is implemented in ui_appstore.{h,c}.
 * It reuses ui_applist rendering primitives for visual consistency.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../app_manager.h"

#define STORE_MAX_ITEMS 32

typedef struct {
    char     filename[APP_ID_MAX_LEN];   /* basename of .app in /sdcard/store/ */
    char     full_path[APP_PATH_MAX_LEN]; /* /sdcard/store/xxx.app */
    char     display_name[APP_NAME_MAX_LEN];
    char     version[APP_VERSION_MAX_LEN];
    char     icon_str[8];
    uint8_t  icon_glyph;
    uint32_t file_size;
    bool     installed;
    bool     valid;
} store_item_t;

typedef struct {
    store_item_t items[STORE_MAX_ITEMS];
    int          count;
} store_catalog_t;

void app_store_init(void);
int  app_store_scan(void);                                  /* return new items */
const store_catalog_t *app_store_get_catalog(void);
int  app_store_get_count(void);
const store_item_t *app_store_get_item(int index);

/* Returns true if the store item's package is already installed */
bool app_store_is_installed(const store_item_t *item);

/* Install item by index: copies .app from /sdcard/store to /sdcard/apps
 * and registers it via app_manager. */
bool app_store_install(int index);