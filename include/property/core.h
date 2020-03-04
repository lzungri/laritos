#pragma once

#include <stdint.h>

#define PROPERTY_ID_MAX_LEN 16
#define PROPERTY_VALUE_MAX_LEN 128

typedef enum {
    PROPERTY_MODE_READ_BY_ALL = 1,
    PROPERTY_MODE_WRITE_BY_ALL = 2,
    PROPERTY_MODE_READ_BY_OWNER = 4,
    PROPERTY_MODE_WRITE_BY_OWNER = 8,
} prop_mode_t;

int property_create(char *id, prop_mode_t mode);
int property_remove(char *id);
int property_set(char *id, char *value);
int property_get(char *id, char *buf);
int property_get_int32(char *id, int32_t *buf);
