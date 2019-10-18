#include <log.h>

#include <stdint.h>
#include <userspace/app.h>
#include <loader/loader.h>

int loader_load_app(uint16_t appidx) {
    appheader_t *apph = (appheader_t *) __apps_start;

    if (apph->magic != APPMAGIC) {
        error("No app #%u or invalid magic number", appidx);
        return -1;
    }

    int (*appmain)(void) = (int (*)(void)) (__apps_start + (uint32_t) apph->text_start);
    return appmain();
}
