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
    char *dest = addr;
    char *frombase = (char *) apph;

    // Load header (we may want to remove this later, all this info will be
    // in the PCB)
    memcpy(dest, frombase, sizeof(appheader_t));
    dest += sizeof(appheader_t);

    // Load code section
    memcpy(dest, frombase + apph->text_start, apph->text_size);
    dest += apph->text_size;

    // Load data section (includes the GOT)
    memcpy(dest, frombase + apph->data_start, apph->data_size);
    dest += apph->data_size;

    // Initializes .bss to zero
    memset(addr + apph->bss_start, 0, apph->bss_size);

    return 0;
}

static int setup_image_context(appheader_t *apph, void *addr) {
    asm("mov r9, %0" : : "r" ((char *) addr + apph->got_start - 0x30));

    uint32_t *ptr = (uint32_t *) ((char *) addr + apph->got_start + 0x0);
    *ptr = (uint32_t) ((char *) addr + 0xf4);
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

    int (*main)(void) = (int (*)(void)) (sizeof(appheader_t) + (char *) imgaddr);
    return main();

error_setup:
error_load:
    free(imgaddr);
    return -1;
}
