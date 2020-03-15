#pragma once

#include <stdbool.h>
#include <board/types.h>
#include <dstruct/list.h>
#include <utils/assert.h>
#include <utils/utils.h>

#define COMPONENT_MAX_ID_LEN CONFIG_BOARD_INFO_MAX_TOKEN_LEN_BYTES

typedef enum {
    COMP_TYPE_UNKNOWN = 0,
    COMP_TYPE_CPU,
    COMP_TYPE_UART,
    COMP_TYPE_INTC,
    COMP_TYPE_RTC,
    COMP_TYPE_HRTIMER,
    COMP_TYPE_BYTESTREAM,
    COMP_TYPE_INPUTDEV,
    COMP_TYPE_LOGGER,
    COMP_TYPE_TICKER,
    COMP_TYPE_VRTIMER,
    COMP_TYPE_SCHED,
    COMP_TYPE_BLOCKDEV,

    COMP_TYPE_LEN,
} component_type_t;

static inline char *component_get_type_str(component_type_t t) {
    static char *str[] =
        { "unknown", "cpu", "uart", "intc", "rtc", "hrtimer", "bytestream", "inputdev", "logger",
          "ticker", "vrtimer", "sched", "blockdev" };
    cassert(ARRAYSIZE(str) >= COMP_TYPE_LEN, component_types_missing);
    return t < COMP_TYPE_LEN ? str[t] : "???";
}

struct component;
typedef struct {
    int (*init)(struct component *c);
    int (*deinit)(struct component *c);

    /**
     * Component deallocation callback
     * Note: Can be NULL (e.g. for statically allocated components)
     *
     * @param c: Component to deallocate
     */
    void *(*free)(struct component *c);

    // TODO Power manager stuff
} component_ops_t;

typedef struct component {
    char id[COMPONENT_MAX_ID_LEN];

    component_type_t type;
    char product[CONFIG_COMP_INFO_SIZE];
    char vendor[CONFIG_COMP_INFO_SIZE];
    char description[CONFIG_COMP_INFO_SIZE];

    /**
     * Indicates whether or not this component is the default one from a set of
     * components of the same type.
     * You can access the default component via component_get_default()
     */
    bool dflt;

    /**
     * List of components
     */
    list_head_t list;

    component_ops_t ops;
} component_t;

#define component_first_of_type(_t, _type) \
    ((_type *) list_first_entry_or_null(&_laritos.comps[_t], component_t, list))

#define component_get_default(_ct, _type) \
    component_first_of_type(_ct, _type)

#define for_each_component_type(_c, _t) \
    list_for_each_entry(_c, &_laritos.comps[_t], list)

#define for_each_component(_c) \
    for (int __i = 0; __i < COMP_TYPE_LEN; __i++) \
        list_for_each_entry(_c, &_laritos.comps[__i], list)

int component_init_global_context(void);
void *component_alloc(size_t size);
int component_init(component_t *comp, char *id, board_comp_t *bcomp, component_type_t type,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
int component_register(component_t *comp);
int component_unregister(component_t *comp);
component_t *component_get_by_id(char *id);
void component_dump_registered_comps(void);
int component_set_info(component_t *c, char *product, char *vendor, char *description);
bool component_any_of(component_type_t t);
bool component_are_mandatory_comps_present(void);


#define SYSFS_COMPONENT_TYPE_MODULE(_id) \
    static int create_root_sysfs(fs_sysfs_mod_t *sysfs) { \
        fs_dentry_t *root = vfs_dir_create(_laritos.fs.comp_type_root, #_id, \
                FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC); \
        if (root == NULL) { \
            error("Error creating component/" #_id " sysfs directory"); \
            return -1; \
        } \
    \
        return 0; \
    } \
    \
    static int remove_root_sysfs(fs_sysfs_mod_t *sysfs) { \
        return vfs_dir_remove(_laritos.fs.comp_type_root, #_id); \
    } \
    \
    SYSFS_MODULE(_id, create_root_sysfs, remove_root_sysfs)
