#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <loader/elf.h>
#include <process/types.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>

typedef struct {
    char *id;
    bool (*can_handle)(fs_file_t *executable);
    pcb_t *(*load)(fs_file_t *executable);
    list_head_t list;
} loader_type_t;

int loader_init_global_context(void);
int loader_register_loader_type(loader_type_t *loader);
int loader_unregister_loader_type(loader_type_t *loader);
pcb_t *loader_load_executable_from_file(char *path);
/**
 * Temporary api for loading apps
 */
pcb_t *loader_load_executable_from_memory(uint16_t appidx);


#define LOADER_MODULE(_id, _can_handle, _load) \
    static loader_type_t _loader_ ## _id = { \
        .id = #_id, \
        .list = LIST_HEAD_INIT(_loader_ ## _id.list), \
        .can_handle = (_can_handle), \
        .load = (_load), \
    }; \
    \
    static int _init_ ## _id(module_t *m) { \
        return loader_register_loader_type(&_loader_ ## _id); \
    } \
    \
    static int _deinit_ ## _id(module_t *m) { \
        return loader_unregister_loader_type(&_loader_ ## _id); \
    } \
    \
    MODULE(loader_ ## _id, _init_ ## _id, _deinit_ ## _id)
