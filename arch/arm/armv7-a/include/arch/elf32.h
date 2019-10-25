#pragma once

#include <stdbool.h>
#include <loader/elf.h>

static inline bool arch_elf32_is_supported(Elf32_Ehdr *elf) {
    return elf->e_machine == EM_ARM;
}

int arch_elf32_relocate_symbol(void *imgaddr, const Elf32_Rel *rel, const Elf32_Sym *sym, uint32_t *got);
