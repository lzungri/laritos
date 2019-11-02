#define DEBUG
#include <log.h>

#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <mm/heap.h>
#include <loader/loader.h>
#include <loader/loader-elf.h>
#include <loader/elf.h>
#include <arch/elf32.h>
#include <arch/cpu.h>
#include <process/pcb.h>
#include <sched/sched.h>

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
                verbose("Copying %lu bytes from ELF section #%u into 0x%p", sect->sh_size, i, dest);
                // Data is in the elf image, it must be copied into memory
                memcpy(dest, frombase + sect->sh_offset, sect->sh_size);
                dest += sect->sh_size;
            } else if (sect->sh_type & SHT_NOBITS) {
                // Zero-initialized data
                verbose("Zero-initializing %lu bytes at 0x%p", sect->sh_size, addr + sect->sh_addr);
                memset(addr + sect->sh_addr, 0, sect->sh_size);
            }
        }
    }

    return 0;
}

static inline int setup_pcb_context(Elf32_Ehdr *elf, pcb_t *pcb, uint32_t *got) {
    return arch_set_got(got);
}

static int relocate_image(Elf32_Ehdr *elf, void *baseaddr, uint32_t rel_offset, uint16_t rel_entries,
        uint32_t symtab_offset, uint32_t *got) {
    int i;
    for (i = 0; i < rel_entries; i++) {
        const Elf32_Rel *rel = (Elf32_Rel *)
                ((char *) elf + rel_offset + i * sizeof(Elf32_Rel));
        const Elf32_Sym *sym = (Elf32_Sym *)
                ((char *) elf + symtab_offset + ELF32_R_SYM(rel->r_info) * sizeof(Elf32_Sym));

        verbose("Relocating offset=0x%lX, type=0x%lX, symbol value=0x%lX", rel->r_offset, ELF32_R_TYPE(rel->r_info), sym->st_value);
        if (arch_elf32_relocate_symbol(baseaddr, rel, sym, got) < 0) {
            error("Failed to relocate offset=0x%lX, type=0x%lX", rel->r_offset, ELF32_R_TYPE(rel->r_info));
            return -1;
        }
    }
    return 0;
}

static inline int get_got_virtual_addr(Elf32_Ehdr *elf, char *shstrtab, uint32_t *got_vaddr) {
    int i;
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Shdr *sect = (Elf32_Shdr *) ((char *) elf + elf->e_shoff + elf->e_shentsize * i);
        char *sectname = shstrtab + sect->sh_name;
        if (strncmp(sectname, ".got", 5) == 0) {
            verbose("GOT virtual address 0x%lX", sect->sh_addr);
            *got_vaddr = sect->sh_addr;
            return 0;
        }
    }
    return -1;
}

int loader_elf32_load_from_memory(Elf32_Ehdr *elf) {
    debug("Loading ELF32 from 0x%p", elf);

    if (!arch_elf32_is_supported(elf)) {
        error("Executable not supported by this platform");
        return -1;
    }

    // Allocate PCB structure for this new process
    pcb_t *pcb = pcb_alloc();
    if (pcb == NULL) {
        error("Could not allocate space for PCB");
        goto error_pcb;
    }

    uint16_t rel_entries = 0;
    uint32_t rel_offset = U32_MAX;
    uint32_t symtab_offset = U32_MAX;
    uint32_t shstrtab_offset = U32_MAX;

    int i;
    // Calculate required image size based on the sections marked for allocation
    // and obtain the offset of relocation-related sections
    for (i = 0; i < elf->e_shnum; i++) {
        Elf32_Shdr *sect = (Elf32_Shdr *) ((char *) elf + elf->e_shoff + elf->e_shentsize * i);
        if (sect->sh_flags & SHF_ALLOC) {
            pcb->imgsize += sect->sh_size;
        }

        if (sect->sh_type == SHT_SYMTAB) {
            verbose("Found symbol table at ELF offset 0x%lX", sect->sh_offset);
            symtab_offset = sect->sh_offset;
        } else if (sect->sh_type == SHT_REL) {
            verbose("Found relocations at ELF offset 0x%lX", sect->sh_offset);
            rel_offset = sect->sh_offset;
            rel_entries = sect->sh_size / sizeof(Elf32_Rel);
        } else if (sect->sh_type == SHT_STRTAB) {
            shstrtab_offset = sect->sh_offset;
        }
    }

    if (rel_offset == U32_MAX || symtab_offset == U32_MAX || shstrtab_offset == U32_MAX) {
        error("Couldn't find offset/s for the relocation, symtab, and/or shstrtab sections");
        goto error_reloc_offset;
    }

    // Find the got table offset
    uint32_t got_vaddr;
    if (get_got_virtual_addr(elf, (char *) elf + shstrtab_offset, &got_vaddr) < 0) {
        error("Couldn't find got section virtual address");
        goto error_got;
    }

    verbose("Allocating %lu bytes for process", pcb->imgsize);
    pcb->imgaddr = malloc(pcb->imgsize);
    if (pcb->imgaddr == NULL) {
        error("Couldn't allocate memory for process at 0x%p", elf);
        goto error_imgalloc;
    }
    debug("Process image located at 0x%p", pcb->imgaddr);

    uint32_t *got = (uint32_t *) ((char *) pcb->imgaddr + got_vaddr);
    verbose("GOT located at 0x%p", got);

    if (load_image_from_memory(elf, pcb->imgaddr) < 0) {
        error("Failed to load process from 0x%p into 0x%p", elf, pcb->imgaddr);
        goto error_load;
    }

    if (setup_pcb_context(elf, pcb, got) < 0) {
        error("Failed to setup context for process at 0x%p", elf);
        goto error_setup;
    }

    if (relocate_image(elf, pcb->imgaddr, rel_offset, rel_entries, symtab_offset, got) < 0) {
        error("Failed to relocate process loaded at 0x%p", pcb->imgaddr);
        goto error_reloc;
    }

    if (pcb_register(pcb) < 0) {
        error("Could not register process loaded at 0x%p", pcb->imgaddr);
        goto error_pcbreg;
    }

//    int (*main)(void) = (int (*)(void)) ((char *) pcb->imgaddr + elf->e_entry);
//    info("process loaded at 0x%p exited with %d", pcb->imgaddr, main());

    sched_move_to_zombie(pcb);
    if (pcb_unregister(pcb) < 0) {
        error("Could not un-register process loaded at 0x%p", pcb->imgaddr);
        goto error_pcbunreg;
    }

    return 0;

error_pcbunreg:
error_pcbreg:
error_reloc:
error_setup:
error_load:
    free(pcb->imgaddr);
error_imgalloc:
error_got:
error_reloc_offset:
    pcb_free(pcb);
error_pcb:
    return -1;
}
