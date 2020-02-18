#pragma once

#include <stdint.h>
#include <dstruct/list.h>
#include <board/types.h>

typedef struct {
    char *id;
    list_head_t list;

    int (*process)(board_comp_t *comp);
} driver_t;
