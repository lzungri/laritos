#pragma once

#include <stdint.h>
#include <userspace/app.h>

/**
 * Temporary api for loading apps
 */
int loader_load_app_from_memory(uint16_t appidx);
int loader32_load_app_from_memory(appheader_t *apph);
