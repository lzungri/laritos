#pragma once

#include <stdint.h>
#include <dstruct/list.h>

#define PROPERTY_ID_MAX_LEN 32
#define PROPERTY_VALUE_MAX_LEN 128

typedef enum {
    PROPERTY_MODE_READ_BY_ALL = 1,
    PROPERTY_MODE_WRITE_BY_ALL = 2,
    PROPERTY_MODE_READ_BY_OWNER = 4,
    PROPERTY_MODE_WRITE_BY_OWNER = 8,
} prop_mode_t;

struct pcb;
typedef struct {
    char id[PROPERTY_ID_MAX_LEN];
    char value[PROPERTY_VALUE_MAX_LEN];
    struct pcb *owner;
    prop_mode_t mode;

    list_head_t list;
} property_t;

int property_init_global_context(void);
int property_create(char *id, prop_mode_t mode);
int property_remove(char *id);
int property_set(char *id, char *value);
int property_get(char *id, char *buf);
void property_get_or_def(char *id, char *buf, char *def);
int property_get_int32(char *id, int32_t *buf);
int32_t property_get_or_def_int32(char *id, int32_t def);
