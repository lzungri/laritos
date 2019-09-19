#pragma once

#include <board.h>
#include <component/component.h>

#include <generated/autoconf.h>

typedef struct {
    component_t parent;

    char product[CONFIG_HWCOMP_HWINFO_SIZE];
    char vendor[CONFIG_HWCOMP_HWINFO_SIZE];
    char description[CONFIG_HWCOMP_HWINFO_SIZE];
} hwcomp_t;

int hwcomp_init(hwcomp_t *comp, char *id, board_comp_t *bcomp, component_subtype_t stype,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
int hwcomp_set_info(hwcomp_t *comp, char *product, char *vendor, char *description);
