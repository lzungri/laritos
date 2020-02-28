#include <log.h>

#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <cpu/core.h>
#include <irq/core.h>
#include <irq/types.h>
#include <mm/heap.h>
#include <mm/spprot.h>
#include <loader/loader.h>
#include <loader/loader-elf.h>
#include <loader/elf.h>
#include <arch/elf32.h>
#include <process/core.h>
#include <sched/core.h>
#include <sched/context.h>
#include <utils/utils.h>
#include <generated/autoconf.h>


static int load_image_from_memory(Elf32_Ehdr *elf, void *addr) {
    char *dest = addr;
    char *frombase = (char *) elf;

    uint16_t i;
    // Traverse each ELF section and check whether we need to copy its data
    // into the process image or initialize it to zero
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Shdr *sect = (Elf32_Shdr *) ((char *) elf + elf->e_shoff + elf->e_shentsize * i);

        if (sect->sh_flags & SHF_ALLOC) {
            if (sect->sh_type & SHT_PROGBITS) {
                verbose_async("Copying %lu bytes from ELF section #%u into 0x%p", sect->sh_size, i, dest);
                // Data is in the elf image, it must be copied into memory
                memcpy(dest, frombase + sect->sh_offset, sect->sh_size);
                dest += sect->sh_size;
            } else if (sect->sh_type & SHT_NOBITS) {
                // Zero-initialized data
                verbose_async("Zero-initializing %lu bytes at 0x%p", sect->sh_size, addr + sect->sh_addr);
                memset(addr + sect->sh_addr, 0, sect->sh_size);
            }
        }
    }

    return 0;
}

static inline int setup_pcb_context(Elf32_Ehdr *elf, pcb_t *pcb) {
    // Setup the stack protector
    spprot_setup(pcb);

    // Initialize process stack pointer
    pcb->mm.sp_ctx = (spctx_t *) ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size - 8);
    verbose_async("Stack pointer at 0x%p", pcb->mm.sp_ctx);

    context_init(pcb, (char *) pcb->mm.imgaddr + elf->e_entry, CPU_MODE_USER);

    return 0;
}

static int relocate_section(Elf32_Ehdr *elf, pcb_t *pcb, uint32_t rel_offset, uint16_t rel_entries, uint32_t symtab_offset) {
    debug_async("Relocating %u entries from section at offset=0x%0lx", rel_entries, rel_offset);
    int i;
    for (i = 0; i < rel_entries; i++) {
        const Elf32_Rel *rel = (Elf32_Rel *)
                ((char *) elf + rel_offset + i * sizeof(Elf32_Rel));
        const Elf32_Sym *sym = (Elf32_Sym *)
                ((char *) elf + symtab_offset + ELF32_R_SYM(rel->r_info) * sizeof(Elf32_Sym));

        verbose_async("Relocating offset=0x%lX, type=0x%lX, symbol value=0x%lX", rel->r_offset, ELF32_R_TYPE(rel->r_info), sym->st_value);
        if (arch_elf32_relocate_symbol(pcb->mm.imgaddr, rel, sym, pcb->mm.got_start) < 0) {
            error_async("Failed to relocate offset=0x%lX, type=0x%lX", rel->r_offset, ELF32_R_TYPE(rel->r_info));
            return -1;
        }
    }
    return 0;
}

static inline Elf32_Shdr *get_section_header(char *section, Elf32_Ehdr *elf, uint32_t shstrtab_offset) {
    int i;
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Shdr *sect = (Elf32_Shdr *) ((char *) elf + elf->e_shoff + elf->e_shentsize * i);
        char *sectname = (char *) elf + shstrtab_offset + sect->sh_name;
        if (strncmp(sectname, section, strlen(section)) == 0) {
            return sect;
        }
    }
    return NULL;
}

static inline int populate_addr_and_size(char *section, void **addr, secsize_t *size, pcb_t *pcb, Elf32_Ehdr *elf, uint32_t shstrtab_offset) {
    Elf32_Shdr *sect = get_section_header(section, elf, shstrtab_offset);
    if (sect == NULL) {
        return -1;
    }
    *addr = (char *) pcb->mm.imgaddr + sect->sh_addr;
    *size = sect->sh_size;
    return 0;
}

static inline int populate_sections(pcb_t *pcb, Elf32_Ehdr *elf, uint32_t shstrtab_offset) {
#define POPULATE_ADDR_AND_SIZE(_sect) \
    if (populate_addr_and_size("." #_sect, &pcb->mm._sect##_start, &pcb->mm._sect##_size, pcb, elf, shstrtab_offset) < 0) { \
        error_async("Couldn't find ." #_sect " section"); \
        return -1; \
    } \
    debug_async(#_sect ":          0x%p-0x%p", pcb->mm._sect##_start, (char *) pcb->mm._sect##_start + pcb->mm._sect##_size);

    POPULATE_ADDR_AND_SIZE(text);
    POPULATE_ADDR_AND_SIZE(data);
    POPULATE_ADDR_AND_SIZE(bss);
    POPULATE_ADDR_AND_SIZE(got);
    POPULATE_ADDR_AND_SIZE(heap);

    if (populate_addr_and_size(".stack", &pcb->mm.stack_bottom, &pcb->mm.stack_size, pcb, elf, shstrtab_offset) < 0) {
        error_async("Couldn't find .stack section");
        return -1;
    }
    pcb->mm.stack_top = (void *) ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size - 4);
    debug_async("stack:         0x%p-0x%p", pcb->mm.stack_bottom, (char *) pcb->mm.stack_bottom + pcb->mm.stack_size);

    return 0;
}

static inline int relocate_image(Elf32_Ehdr *elf, pcb_t *pcb, uint32_t shstrtab_offset, uint32_t symtab_offset) {
    char *relocs[] = { ".rel.text", ".rel.data" };
    int i;
    for (i = 0; i < ARRAYSIZE(relocs); i++) {
        uint32_t rel_offset;
        uint16_t rel_entries;

        Elf32_Shdr *sect = get_section_header(relocs[i], elf, shstrtab_offset);
        if (sect == NULL) {
            debug_async("No relocation section for '%s'", relocs[i]);
            continue;
        }
        rel_offset = sect->sh_offset;
        rel_entries = sect->sh_size / sizeof(Elf32_Rel);

        if (relocate_section(elf, pcb, rel_offset, rel_entries, symtab_offset) < 0) {
            error_async("Failed to relocate process at 0x%p", pcb);
            return -1;
        }
    }
    return 0;
}

static pcb_t *load(void *executable) {
    Elf32_Ehdr *elf = executable;
    debug_async("Loading ELF32 from 0x%p", elf);

    if (!arch_elf32_is_supported(elf)) {
        error("Executable not supported by this platform");
        return NULL;
    }

    // Allocate PCB structure for this new process
    pcb_t *pcb = process_alloc();
    if (pcb == NULL) {
        error_async("Could not allocate space for PCB");
        goto error_pcb;
    }

    pcb->kernel = false;

    uint32_t symtab_offset = U32_MAX;
    uint32_t shstrtab_offset = U32_MAX;

    int i;
    // Calculate required image size based on the sections marked for allocation
    // and obtain the offset for the symbol tables
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Shdr *sect = (Elf32_Shdr *) ((char *) elf + elf->e_shoff + elf->e_shentsize * i);
        if (sect->sh_flags & SHF_ALLOC) {
            pcb->mm.imgsize += sect->sh_size;
        }

        if (sect->sh_type == SHT_SYMTAB) {
            verbose_async("Found symbol table at ELF offset 0x%lX", sect->sh_offset);
            symtab_offset = sect->sh_offset;
        } else if (sect->sh_type == SHT_STRTAB) {
            shstrtab_offset = sect->sh_offset;
        }
    }

    if (symtab_offset == U32_MAX || shstrtab_offset == U32_MAX) {
        error_async("Couldn't find offset/s for the symtab and/or shstrtab sections");
        goto error_symoffset;
    }

    verbose_async("Allocating %lu bytes for process", pcb->mm.imgsize);
    pcb->mm.imgaddr = malloc(pcb->mm.imgsize);
    if (pcb->mm.imgaddr == NULL) {
        error_async("Couldn't allocate memory for process at 0x%p", pcb);
        goto error_imgalloc;
    }

    debug_async("Process image: 0x%p-0x%p", pcb->mm.imgaddr, (char *) pcb->mm.imgaddr + pcb->mm.imgsize);

    if (populate_sections(pcb, elf, shstrtab_offset) < 0) {
        error_async("Failed to populate sections for process at 0x%p", pcb->mm.imgaddr);
        goto error_sections;
    }

    if (load_image_from_memory(elf, pcb->mm.imgaddr) < 0) {
        error_async("Failed to load process from 0x%p into 0x%p", elf, pcb->mm.imgaddr);
        goto error_load;
    }

    if (setup_pcb_context(elf, pcb) < 0) {
        error_async("Failed to setup context for process at 0x%p", elf);
        goto error_setup;
    }

    if (relocate_image(elf, pcb, shstrtab_offset, symtab_offset) < 0) {
        error_async("Failed to handle relocations for process at 0x%p", pcb);
        goto error_reloc;
    }

    process_set_priority(pcb, CONFIG_SCHED_PRIORITY_MAX_USER);

    if (process_register(pcb) < 0) {
        error_async("Could not register process at 0x%p", pcb);
        goto error_register;
    }

    return pcb;

error_register:
error_reloc:
error_setup:
error_load:
error_sections:
    free(pcb->mm.imgaddr);
    pcb->mm.imgaddr = NULL;
error_imgalloc:
error_symoffset:
    process_free(pcb);
error_pcb:
    return NULL;
}

static bool can_handle(void *executable) {
    if (memcmp(executable, ELFMAG, sizeof(ELFMAG) - 1) != 0) {
        return false;
    }
    return ((char *) executable)[EI_CLASS] == ELFCLASS32;
}

LOADER_MODULE(elf32, can_handle, load);
