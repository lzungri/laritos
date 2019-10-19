#include <log.h>

#include <stdint.h>
#include <string.h>
#include <mm/heap.h>
#include <userspace/app.h>
#include <loader/loader.h>

static inline void *allocate_app_image(appheader_t *apph) {
    size_t imgsize = apph->text_size + apph->data_size + apph->bss_size +
                     apph->heap_size + apph->stack_size;
    return malloc(imgsize);
}

static int load_image_from_memory(appheader_t *apph, void *addr) {
    size_t offset = 0;
    char *destbase = addr;
    char *frombase = (char *) apph;

    // Load code section
    memcpy(destbase + offset, frombase + apph->text_start, apph->text_size);
    offset += apph->text_size;

    // Load data section (includes the GOT)
    memcpy(destbase + offset, frombase + apph->data_start, apph->data_size);
    offset += apph->data_size;

    // Initializes .bss to zero
    memset(destbase + apph->bss_start, 0, apph->bss_size);

    return 0;
}

static int setup_image_context(appheader_t *apph, void *addr) {
    return 0;
}

int loader_load_app(uint16_t appidx) {
    appheader_t *apph = (appheader_t *) __apps_start;

    if (apph->magic != APPMAGIC) {
        error("No app #%u or invalid magic number", appidx);
        return -1;
    }

    void *imgaddr = allocate_app_image(apph);
    if (imgaddr == NULL) {
        error("Couldn't allocate memory for app #%u", appidx);
        return -1;
    }

    if (load_image_from_memory(apph, imgaddr) < 0) {
        error("Failed to load app #%u image at 0x%p", appidx, imgaddr);
        goto error_load;
    }

    if (setup_image_context(apph, imgaddr) < 0) {
        error("Failed to setup context for app #%u", appidx);
        goto error_setup;
    }

    int (*main)(void) = (int (*)(void)) imgaddr;
    return main();

error_setup:
error_load:
    free(imgaddr);
    return -1;
}
