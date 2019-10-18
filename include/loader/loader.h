#pragma once

#include <stdint.h>

/**
 * Offset in laritos.bin where the apps are loaded.
 * Apps are (for now) appended to laritos.bin via the tools/apps/install.sh script
 * This is temporary until we implement a better mechanism for flashing apps,
 * such as a file system on sd card.
 */
extern char __apps_start[];

/**
 * Temporary api for loading apps
 */
int loader_load_app(uint16_t appidx);
