#pragma once

#include <loader/elf.h>

int arch_relocate_symbol(void *imgaddr, const Elf32_Rel *rel, const Elf32_Sym *sym);
