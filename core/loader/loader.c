#include <log.h>

#include <loader/loader.h>
#include <userspace/app.h>

/**
 * Offset in laritos.bin where the apps are loaded.
 * Apps are (for now) appended to laritos.bin via the tools/apps/install.sh script
 * This is temporary until we implement a better mechanism for flashing apps,
 * such as a file system on sd card.
 */
extern char __apps_start[];

int loader_load_app_from_memory(uint16_t appidx) {
    appheader_t *apph = (appheader_t *) __apps_start;

    if (apph->magic != APPMAGIC) {
        error("No app #%u or invalid magic number", appidx);
        return -1;
    }

    if (apph->hdrver != HEADER_VERSION) {
        error("Invalid version for app #%u", appidx);
        return -1;
    }

    return loader32_load_app_from_memory(apph);
}
