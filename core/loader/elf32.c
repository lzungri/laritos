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
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <generated/autoconf.h>

static inline int read_all_from_file(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    int nread = vfs_file_read(f, buf, blen, offset);
    return nread < 0 || nread < blen ? -1 : 0;
}

static int copy_image_into_memory(fs_file_t *f, Elf32_Ehdr *elf, Elf32_Section *sections, void *addr) {
    char *dest = addr;
    uint16_t i;
    // Traverse each ELF section and check whether we need to copy its data
    // into the process image or initialize it to zero
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Section *sect = &sections[i];
        if (sect->parent.sh_flags & SHF_ALLOC) {
            if (sect->parent.sh_type & SHT_PROGBITS) {
                verbose_async("Copying %lu bytes from ELF section #%u into 0x%p", sect->parent.sh_size, i, dest);
                // Data is in the elf image, it must be copied into memory
                if (read_all_from_file(f, dest, sect->parent.sh_size, sect->parent.sh_offset) < 0) {
                    error_async("Couldn't read data from ELF section #%d", i);
                    return -1;
                }
                dest += sect->parent.sh_size;
            } else if (sect->parent.sh_type & SHT_NOBITS) {
                // Zero-initialized data
                verbose_async("Zero-initializing %lu bytes at 0x%p", sect->parent.sh_size, addr + sect->parent.sh_addr);
                memset(addr + sect->parent.sh_addr, 0, sect->parent.sh_size);
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

static int relocate_section(fs_file_t *f, Elf32_Ehdr *elf, pcb_t *pcb, uint32_t rel_offset, uint16_t rel_entries, uint32_t symtab_offset) {
    debug_async("Relocating %u entries from section at offset=0x%0lx", rel_entries, rel_offset);
    int i;
    for (i = 0; i < rel_entries; i++) {
        Elf32_Rel rel;
        if (read_all_from_file(f, &rel, sizeof(rel), rel_offset + i * sizeof(rel)) < 0) {
            error_async("Couldn't read ELF relocation #%d", i);
            return -1;
        }
        Elf32_Sym sym;
        if (read_all_from_file(f, &sym, sizeof(sym), symtab_offset + ELF32_R_SYM(rel.r_info) * sizeof(sym)) < 0) {
            error_async("Couldn't read ELF relocation #%d", i);
            return -1;
        }

        verbose_async("Relocating offset=0x%lX, type=0x%lX, symbol value=0x%lX", rel.r_offset, ELF32_R_TYPE(rel.r_info), sym.st_value);
        if (arch_elf32_relocate_symbol(pcb->mm.imgaddr, &rel, &sym, pcb->mm.got_start) < 0) {
            error_async("Failed to relocate offset=0x%lX, type=0x%lX", rel.r_offset, ELF32_R_TYPE(rel.r_info));
            return -1;
        }
    }
    return 0;
}

static inline Elf32_Section *get_section(char *section, Elf32_Ehdr *elf, Elf32_Section *sections) {
    int i;
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Section *sect = &sections[i];
        if (strncmp(sect->name, section, sizeof(sect->name)) == 0) {
            return sect;
        }
    }
    return NULL;
}

static inline int populate_addr_and_size(fs_file_t *f, char *section, void **addr, secsize_t *size, pcb_t *pcb, Elf32_Ehdr *elf, Elf32_Section *sections) {
    Elf32_Section *sect = get_section(section, elf, sections);
    if (sect == NULL) {
        error_async("Couldn't read '%s' section header", section);
        return -1;
    }
    *addr = (char *) pcb->mm.imgaddr + sect->parent.sh_addr;
    *size = sect->parent.sh_size;
    return 0;
}

static inline int setup_image_sections(fs_file_t *f, pcb_t *pcb, Elf32_Ehdr *elf, Elf32_Section *sections) {
#define POPULATE_ADDR_AND_SIZE(_sect) \
    if (populate_addr_and_size(f, "." #_sect, &pcb->mm._sect##_start, &pcb->mm._sect##_size, pcb, elf, sections) < 0) { \
        error_async("Couldn't find ." #_sect " section"); \
        return -1; \
    } \
    debug_async(#_sect ":          0x%p-0x%p", pcb->mm._sect##_start, (char *) pcb->mm._sect##_start + pcb->mm._sect##_size);

    POPULATE_ADDR_AND_SIZE(text);
    POPULATE_ADDR_AND_SIZE(data);
    POPULATE_ADDR_AND_SIZE(bss);
    POPULATE_ADDR_AND_SIZE(got);
    POPULATE_ADDR_AND_SIZE(heap);

    if (populate_addr_and_size(f, ".stack", &pcb->mm.stack_bottom, &pcb->mm.stack_size, pcb, elf, sections) < 0) {
        error_async("Couldn't find .stack section");
        return -1;
    }
    pcb->mm.stack_top = (void *) ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size - 4);
    debug_async("stack:         0x%p-0x%p", pcb->mm.stack_bottom, (char *) pcb->mm.stack_bottom + pcb->mm.stack_size);

    return 0;
}

static inline int relocate_image(fs_file_t *f, Elf32_Ehdr *elf, Elf32_Section *sections, pcb_t *pcb, uint32_t symtab_offset) {
    char *relocs[] = { ".rel.text", ".rel.data" };
    int i;
    for (i = 0; i < ARRAYSIZE(relocs); i++) {
        Elf32_Section *sect = get_section(relocs[i], elf, sections);
        if (sect == NULL) {
            debug_async("No relocation section for '%s'", relocs[i]);
            return -1;
        }

        uint32_t rel_offset;
        uint16_t rel_entries;
        rel_offset = sect->parent.sh_offset;
        rel_entries = sect->parent.sh_size / sizeof(Elf32_Rel);

        if (relocate_section(f, elf, pcb, rel_offset, rel_entries, symtab_offset) < 0) {
            error_async("Failed to relocate '%s'", f->dentry->name);
            return -1;
        }
    }
    return 0;
}

static int populate_sections(fs_file_t *f, Elf32_Ehdr *elf, Elf32_Section *sections) {
    uint32_t shstrtab_offset = U32_MAX;

    int i;
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Section *sect = &sections[i];
        // Read section header from ELF
        if (read_all_from_file(f, &sect->parent, sizeof(Elf32_Shdr), elf->e_shoff + elf->e_shentsize * i) < 0) {
            error_async("Couldn't read ELF section #%d", i);
            return -1;
        }

        if (sect->parent.sh_type == SHT_STRTAB) {
            shstrtab_offset = sect->parent.sh_offset;
        }
    }

    if (shstrtab_offset == U32_MAX) {
        error_async("Couldn't find offset/s for the shstrtab section");
        return -1;
    }

    // Populate sections names
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Section *sect = &sections[i];
        if (read_all_from_file(f, sect->name, sizeof(sect->name), shstrtab_offset + sect->parent.sh_name) < 0) {
            error_async("Couldn't read section name from ELF string table");
            return -1;
        }
    }

    return 0;
}

static pcb_t *load(fs_file_t *f) {
    debug_async("Loading ELF32 from '%s'", f->dentry->name);

    Elf32_Ehdr elf;
    if (read_all_from_file(f, &elf, sizeof(elf), 0) < 0) {
        error_async("Couldn't read ELF header from file");
        return NULL;
    }

    if (!arch_elf32_is_supported(&elf)) {
        error_async("Executable not supported by this platform");
        return NULL;
    }

    Elf32_Section *sections = calloc(elf.e_shnum, sizeof(Elf32_Section));
    if (sections == NULL) {
        error_async("Could not allocate memory for ELF sections");
        return NULL;
    }

    if (populate_sections(f, &elf, sections) < 0) {
        error_async("Couldn't populate ELF sections");
        goto error_popu_sects;
    }

    // Allocate PCB structure for this new process
    pcb_t *pcb = process_alloc();
    if (pcb == NULL) {
        error_async("Could not allocate space for PCB");
        goto error_pcb;
    }

    pcb->kernel = false;

    uint32_t symtab_offset = U32_MAX;

    int i;
    // Calculate required image size based on the sections marked for allocation
    // and obtain the offset for the symbol tables
    for (i = 0; i < elf.e_shnum; i++) {
        Elf32_Section *sect = &sections[i];

        if (sect->parent.sh_flags & SHF_ALLOC) {
            pcb->mm.imgsize += sect->parent.sh_size;
        }

        if (sect->parent.sh_type == SHT_SYMTAB) {
            verbose_async("Found symbol table at ELF offset 0x%lX", sect->sh_offset);
            symtab_offset = sect->parent.sh_offset;
        }
    }

    if (symtab_offset == U32_MAX) {
        error_async("Couldn't find offset for the symtab section");
        goto error_symoffset;
    }

    verbose_async("Allocating %lu bytes for process", pcb->mm.imgsize);
    pcb->mm.imgaddr = malloc(pcb->mm.imgsize);
    if (pcb->mm.imgaddr == NULL) {
        error_async("Couldn't allocate memory for '%s'", f->dentry->name);
        goto error_imgalloc;
    }

    debug_async("Process image: 0x%p-0x%p", pcb->mm.imgaddr, (char *) pcb->mm.imgaddr + pcb->mm.imgsize);

    if (setup_image_sections(f, pcb, &elf, sections) < 0) {
        error_async("Failed to populate sections for '%s'", f->dentry->name);
        goto error_sections;
    }

    if (copy_image_into_memory(f, &elf, sections, pcb->mm.imgaddr) < 0) {
        error_async("Failed to copy '%s' image into 0x%p", f->dentry->name, pcb->mm.imgaddr);
        goto error_load;
    }

    if (setup_pcb_context(&elf, pcb) < 0) {
        error_async("Failed to setup context for '%s'", f->dentry->name);
        goto error_setup;
    }

    if (relocate_image(f, &elf, sections, pcb, symtab_offset) < 0) {
        error_async("Failed to handle relocations for '%s'", f->dentry->name);
        goto error_reloc;
    }

    process_set_priority(pcb, CONFIG_SCHED_PRIORITY_MAX_USER);

    if (process_register(pcb) < 0) {
        error_async("Could not register process '%s'", f->dentry->name);
        goto error_register;
    }

    // Free temp sections array
    free(sections);
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
error_popu_sects:
    free(sections);
    return NULL;
}

static bool can_handle(fs_file_t *f) {
    char buf[SELFMAG] = { 0 };
    if (read_all_from_file(f, buf, sizeof(buf), 0) < 0) {
        error_async("Couldn't read magic number from file");
        return false;
    }
    if (memcmp(buf, ELFMAG, SELFMAG) != 0) {
        return false;
    }

    if (read_all_from_file(f, buf, 1, EI_CLASS) < 0) {
        error_async("Couldn't read ELF class from file");
        return false;
    }
    return buf[0] == ELFCLASS32;
}

LOADER_MODULE(elf32, can_handle, load);
