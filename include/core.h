#pragma once

#include <board-types.h>
#include <driver.h>
#include <component/component.h>

typedef struct {
    board_info_t bi;
    component_t *components[CONFIG_MAX_COMPONENTS];

} laritos_t;

// Global structure used as an entry point for accessing the main pieces of information
// of the OS
extern laritos_t _laritos;

extern void kernel_entry(void);
