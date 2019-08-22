#pragma once

#include <board.h>

#define DRIVER_NAME_MAX_LEN 10

//typedef enum {
//    DRIVER_STATE_NOT_INITIALIZED,
//    DRIVER_STATE_INITIALIZED,
//
//    DRIVER_STATE_LEN,
//} driver_state_t;

typedef struct {
    char *name;
    int (*process)(board_comp_t *comp);
} driver_mgr_t;

// Null-terminated array of pointers to driver managers
extern driver_mgr_t *__driver_mgrs_start[];

int driver_process_board_components(board_info_t *bi);
