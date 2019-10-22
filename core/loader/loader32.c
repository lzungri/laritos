#include <log.h>

#include <stdint.h>
#include <string.h>
#include <mm/heap.h>
#include <userspace/app.h>
#include <loader/loader.h>
#include <loader/elf.h>
#include <arch/loader.h>

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

static int setup_image_context(void *addr) {
    appheader_t *apph = addr;
    asm("mov r9, %0" : : "r" ((char *) addr + apph->got_start - 0x30));

    uint32_t *ptr = (uint32_t *) ((char *) addr + apph->got_start + 0x0);
    *ptr = (uint32_t) ((char *) addr + 0xf4);
    return 0;
}

static int relocate_image(void *addr) {
    appheader_t *apph = addr;
    int i;
    uint32_t nrels = apph->reloc_size / sizeof(Elf32_Rel);
    for (i = 0; i < nrels; i++) {
        const Elf32_Rel *rel = (Elf32_Rel *)
                ((char *) addr + apph->reloc_start + i * sizeof(Elf32_Rel));
        const Elf32_Sym *sym = (Elf32_Sym *)
                ((char *) addr + apph->symbol_start + ELF32_R_SYM(rel->r_info) * sizeof(Elf32_Sym));

        verbose("Relocating symbol #%lu, type=%lu", ELF32_R_SYM(rel->r_info), ELF32_R_TYPE(rel->r_info));
        if (arch_relocate_symbol(addr, rel, sym) < 0) {
            error("Failed to relocate symbol #%lu, type=%lu", ELF32_R_SYM(rel->r_info), ELF32_R_TYPE(rel->r_info));
            return -1;
        }
    }
    return 0;
}

int loader_load_app(uint16_t appidx) {
    appheader_t *apph = (appheader_t *) __apps_start;

    if (apph->magic != APPMAGIC) {
        error("No app #%u or invalid magic number", appidx);
        return -1;
    }

    if (apph->hdrver != HEADER_VERSION) {
        error("Invalid version for app #%u", appidx);
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

    if (setup_image_context(imgaddr) < 0) {
        error("Failed to setup context for app #%u", appidx);
        goto error_setup;
    }

    if (relocate_image(imgaddr) < 0) {
        error("Failed to relocate app #%u", appidx);
        goto error_reloc;
    }

    int (*main)(void) = (int (*)(void)) (sizeof(appheader_t) + (char *) imgaddr);
    return main();

error_reloc:
error_setup:
error_load:
    free(imgaddr);
    return -1;
}
