#pragma once

#include <stdint.h>
#include <dstruct/list.h>
#include <board/types.h>

typedef struct {
    char *id;
    struct list_head list;

    int (*process)(board_comp_t *comp);
} driver_t;
