#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>


int vfs_dentry_init_root() {
    INIT_LIST_HEAD(&_laritos.fs.root.children);
    INIT_LIST_HEAD(&_laritos.fs.root.siblings);
    strncpy(_laritos.fs.root.name, "/", sizeof(_laritos.fs.root.name));
    return 0;
}



#ifdef CONFIG_TEST_CORE_FS_VFS_DENTRY
#include __FILE__
#endif
