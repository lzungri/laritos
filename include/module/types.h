#pragma once

#include <dstruct/list.h>
#include <generated/autoconf.h>

typedef struct module {
    char *id;
    list_head_t list;

    int (*init)(struct module *m);
    int (*deinit)(struct module *m);
} module_t;
