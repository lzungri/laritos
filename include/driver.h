#pragma once

#include <board.h>

#define DRIVER_NAME_MAX_LEN 10

typedef enum {
    DRIVER_STATE_NOT_INITIALIZED,
    DRIVER_STATE_INITIALIZED,

    DRIVER_STATE_LEN,
} driver_state_t;

typedef struct {
    char *name;
    int (*init)(board_comp_t *comp);
    int (*deinit)(void);
    driver_state_t state;
} driver_t;

// Null-terminated array of pointers to drivers
extern driver_t *__drivers_start[];

int driver_initialize_all(board_info_t *bi);
int driver_deinit_all(void);
