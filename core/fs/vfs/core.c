#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <mm/heap.h>


int vfs_init_global_context() {
    INIT_LIST_HEAD(&_laritos.fs.fstypes);
    INIT_LIST_HEAD(&_laritos.fs.mounts);
    return 0;
}

fs_type_t *vfs_get_fstype(char *fstype) {
    fs_type_t *fst;
    list_for_each_entry(fst, &_laritos.fs.fstypes, list) {
        if (strncmp(fst->id, fstype, strlen(fst->id)) == 0) {
            return fst;
        }
    }
    return NULL;
}

int vfs_register_fs_type(fs_type_t *fst) {
    if (vfs_get_fstype(fst->id) != NULL) {
        error("%s filesystem type already registered", fst->id);
        return -1;
    }

    if (fst->id == NULL || fst->id[0] == '\0') {
        error("File system type cannot have a null or empty id");
        return -1;
    }

    if (fst->mount == NULL) {
        error("File system type cannot have a null mount function");
        return -1;
    }

    debug("Registering FS type '%s'", fst->id);
    list_add_tail(&fst->list, &_laritos.fs.fstypes);
    return 0;
}

int vfs_unregister_fs_type(fs_type_t *fst) {
    if (fst == NULL) {
        return -1;
    }

    if (vfs_get_fstype(fst->id) == NULL) {
        error("%s filesystem type not registered", fst->id);
        return -1;
    }

    debug("Un-registering FS type '%s'", fst->id);
    list_del_init(&fst->list);
    return -1;
}

bool vfs_is_fs_type_supported(char *fstype) {
    return vfs_get_fstype(fstype) != NULL;
}



#ifdef CONFIG_TEST_CORE_FS_VFS_CORE
#include __FILE__
#endif
