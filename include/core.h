#pragma once

#include <stdbool.h>
#include <board-types.h>
#include <component/component.h>
#include <driver/driver.h>
#include <time/time.h>

typedef struct {
    board_info_t bi;
    // TODO Optimize this
    component_t *components[CONFIG_COMP_MAX];

    struct {
        timezone_t tz;
        bool dst;
    } timeinfo;
} laritos_t;

// Global structure used as an entry point for accessing the main pieces of information
// of the OS
extern laritos_t _laritos;

extern void kernel_entry(void);
