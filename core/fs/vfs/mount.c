#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <fs/file.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <mm/heap.h>

static inline bool is_valid_mount_struct(fs_mount_t *fsm) {
    if (fsm->sb == NULL) {
        error("No superblock was instantiated");
        return false;
    }

    if (fsm->sb->ops.alloc_inode == NULL) {
        error("No alloc_inode() function declared in superblock");
        return false;
    }
    return true;
}

static inline fs_mount_t *get_fsmount(char *mount_point) {
    fs_dentry_t *mount_de = vfs_dentry_lookup(mount_point);
    if (mount_de == NULL) {
        error("'%s' is not a valid directory", mount_point);
        return NULL;
    }

    fs_mount_t *fsm;
    list_for_each_entry(fsm, &_laritos.fs.mounts, list) {
        if (mount_de == fsm->root) {
            return fsm;
        }
    }
    return NULL;
}

fs_mount_t *vfs_mount_fs(char *fstype, char *mount_point, fs_mount_flags_t flags, void *params) {
    info("Mounting %s filesystem at %s with flags=0x%0x", fstype, mount_point, flags);

    fs_type_t *fst = vfs_get_fstype(fstype);
    if (fst == NULL) {
        error("File system '%s' not supported", fstype);
        return NULL;
    }

    if (file_exist(mount_point)) {
        error("Cannot mount at '%s', already in use", mount_point);
        return NULL;
    }

    fs_dentry_t *parent = vfs_dentry_lookup_parent(mount_point);
    // Root filesystem has no parent
    if (parent == NULL && strncmp(mount_point, "/", 2) != 0) {
        error("Not all directories from '%s' exist", mount_point);
        return NULL;
    }

    fs_mount_t *fsm = calloc(1, sizeof(fs_mount_t));
    if (fsm == NULL) {
        error("No memory available for fs_mount_t structure");
        return NULL;
    }
    fsm->flags = flags;
    INIT_LIST_HEAD(&fsm->list);

    if (fst->mount(fst, fsm) < 0) {
        error("Could not mount file system '%s' at %s", fst->id, mount_point);
        goto error_mount;
    }

    if (!is_valid_mount_struct(fsm)) {
        error("Mount structure is not valid or is missing essential components");
        goto error_validation;
    }

    fsm->sb->mount = fsm;

    fsm->root = vfs_dentry_alloc(file_get_basename(mount_point), NULL, NULL);
    if (fsm->root == NULL) {
        error("No memory for %s root entry", mount_point);
        goto error_dentry;
    }

    fsm->root->inode = fsm->sb->root;
    if (fsm->root->inode == NULL) {
        error("No root inode for '%s'", mount_point);
        goto error_inode;
    }
    fsm->root->inode->mode = FS_ACCESS_MODE_DIR | FS_ACCESS_MODE_EXEC;
    if (flags & FS_MOUNT_READ) {
        fsm->root->inode->mode |= FS_ACCESS_MODE_READ;
    }
    if (flags & FS_MOUNT_WRITE) {
        fsm->root->inode->mode |= FS_ACCESS_MODE_WRITE;
    }

    list_add_tail(&fsm->list, &_laritos.fs.mounts);

    if (parent != NULL) {
        vfs_dentry_add_child(parent, fsm->root);
    }

    return fsm;

error_inode:
    vfs_dentry_free_tree(fsm->root);
error_dentry:
error_validation:
    if (fsm->ops.unmount != NULL) {
        fsm->ops.unmount(fsm);
    }
error_mount:
    free(fsm);

    return NULL;
}

int vfs_unmount_fs(char *mount_point) {
    fs_mount_t *fsm = get_fsmount(mount_point);
    if (fsm == NULL) {
        error("Cannot unmount: '%s' not mounted", mount_point);
        return -1;
    }

    info("Unmounting filesystem %s", mount_point);

    list_del_init(&fsm->list);
    vfs_dentry_free_tree(fsm->root);
    if (fsm->ops.unmount != NULL && fsm->ops.unmount(fsm) < 0) {
        error("Error while unmounting filesystem at %s", mount_point);
        return -1;
    }
    free(fsm);
    return 0;
}



#ifdef CONFIG_TEST_CORE_FS_VFS_MOUNT
#include __FILE__
#endif
