#include <log.h>

#include <loader/elf.h>
#include <arch/loader.h>

int arch_relocate_symbol(void *imgaddr, const Elf32_Rel *rel, const Elf32_Sym *sym) {
    return 0;
}
