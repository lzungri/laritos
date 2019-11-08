#pragma once

#include <stdint.h>
#include <loader/elf.h>
#include <process/pcb.h>

/**
 * Temporary api for loading apps
 */
pcb_t *loader_load_executable_from_memory(uint16_t appidx);
