#pragma once

#include <board.h>
#include <driver.h>

typedef struct {
    board_info_t bi;

} laritos_t;

// Global structure used as an entry point for accessing the main pieces of information
// of the OS
extern laritos_t _laritos;

extern void kernel_entry(void);
