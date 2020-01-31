#pragma once

#include <dstruct/list.h>
#include <generated/autoconf.h>

typedef struct module {
    char *id;
    struct list_head list;

    int (*init)(struct module *m);
    int (*deinit)(struct module *m);
} module_t;
