#pragma once

#include <board-types.h>

#include <generated/autoconf.h>

typedef struct {
    char *name;
    int (*process)(board_comp_t *comp);
} driver_mgr_t;


#define DEF_DRIVER_MANAGER(_name, _process_func) \
    static driver_mgr_t _driver_mgr_ ## _name = { \
        .name = #_name, \
        .process = (_process_func), \
    }; \
    driver_mgr_t *_driver_mgr_ ## _name ## _ptr __attribute__ ((section (".driver_mgrs"))) = &_driver_mgr_ ## _name;

// Null-terminated array of pointers to driver managers
extern driver_mgr_t *__driver_mgrs_start[];

int driver_process_board_components(board_info_t *bi);
