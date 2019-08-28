#pragma once

#include <board.h>

#define COMPONENT_MAX_ID_LEN 32

typedef enum {
    COMP_TYPE_UNKNOWN = 0,
    COMP_TYPE_UART,
    COMP_TYPE_CHARDEV,
    COMP_TYPE_LOGGER,

    COMP_TYPE_LEN,
} component_type_t;

struct component;
typedef struct {
    int (*init)(struct component *c);
    int (*deinit)(struct component *c);

    // TODO Power manager stuff
} component_ops_t;

typedef struct component {
    char id[COMPONENT_MAX_ID_LEN];

    component_type_t type;
    component_ops_t ops;
} component_t;


#define for_each_component(_c) \
    for (int i = 0; _c = _laritos.components[i], i < ARRAYSIZE(_laritos.components); i++) \
        if (_c != NULL)

int component_init(component_t *comp, char *id, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
int component_register(component_t *comp);
int component_unregister(component_t *comp);
component_t *component_get_by_id(char *id);
void component_dump_registered_comps(void);
