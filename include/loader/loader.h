#pragma once

#include <stdint.h>
#include <userspace/app.h>
#include <loader/elf.h>

/**
 * Temporary api for loading apps
 */
int loader_load_app_from_memory(uint16_t appidx);
