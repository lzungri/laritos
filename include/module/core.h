#pragma once

#include <module/types.h>
#include <generated/autoconf.h>

int module_init_global_context(void);
int module_deinit_global_context(void);
int module_load_static_modules(void);

#define MODULE(_id, _init, _deinit) \
    static module_t _module_ ## _id = { \
        .id = #_id, \
        .list = LIST_HEAD_INIT(_module_ ## _id.list), \
        .init = (_init), \
        .deinit = (_deinit), \
    }; \
    module_t *_module_ ## _id ## _ptr __attribute__ ((section (".modules"))) = &_module_ ## _id;
