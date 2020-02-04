#include <log.h>

#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>

typedef struct {
    fs_inode_t parent;

} pseudofs_inode_t;

static fs_inode_t *lookup(fs_inode_t *parent, char *name) {
    return NULL;
}

static int mkdir(fs_inode_t *parent, char *name, fs_access_mode_t mode) {
    return -1;
}

static int rmdir(fs_inode_t *parent, char *name) {
    return -1;
}


static fs_inode_t *alloc_inode(fs_superblock_t *sb) {
    pseudofs_inode_t *inode = calloc(1, sizeof(pseudofs_inode_t));
    if (inode == NULL) {
        error("No memory available for pseudofs_inode_t structure");
        return NULL;
    }
    inode->parent.ops.lookup = lookup;
    inode->parent.ops.mkdir = mkdir;
    inode->parent.ops.rmdir = rmdir;
    return (fs_inode_t *) inode;
}

static void free_inode(fs_inode_t *inode) {
    free(inode);
}

static int unmount(fs_mount_t *fsm) {
    free(fsm->sb);
    return 0;
}

int mount(fs_type_t *fstype, fs_mount_t *m) {
    m->sb = calloc(1, sizeof(fs_superblock_t));
    if (m->sb == NULL) {
        error("No memory available for fs_superblock_t structure");
        goto error_sb;
    }
    m->sb->fstype = fstype;
    m->sb->ops.alloc_inode = alloc_inode;
    m->sb->ops.free_inode = free_inode;

    m->ops.unmount = unmount;

    return 0;

error_sb:
    return -1;
}

FILESYSTEM_MODULE(pseudofs, mount);



#ifdef CONFIG_TEST_CORE_FS_PSEUDOFS
#include __FILE__
#endif
