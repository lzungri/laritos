#pragma once

#include <board/board-types.h>
#include <module/types.h>
#include <module/core.h>
#include <driver/types.h>

int driver_init_global_context(void);
int driver_deinit_global_context(void);
int driver_register(driver_t *d, module_t *owner);
int driver_unregister(driver_t *d, module_t *owner);

int driver_process_board_components(board_info_t *bi);

#define DRIVER_MODULE(_id, _process_func) \
    static driver_t _driver_ ## _id = { \
        .id = #_id, \
        .list = LIST_HEAD_INIT(_driver_ ## _id.list), \
        .process = _process_func, \
    }; \
    \
    static int _init_ ## _id(module_t *m) { \
        return driver_register(&_driver_ ## _id, m); \
    } \
    \
    static int _deinit_ ## _id(module_t *m) { \
        return driver_unregister(&_driver_ ## _id, m); \
    } \
    \
    MODULE(_id, _init_ ## _id, _deinit_ ## _id)
