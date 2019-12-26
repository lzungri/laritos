#pragma once

#include <loader/elf.h>
#include <process/core.h>

pcb_t *loader_elf32_load_from_memory(Elf32_Ehdr *elf);
