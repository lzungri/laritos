#include <log.h>

#include <stdint.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>
#include <utils/math.h>
#include <fs/pseudofs.h>
#include <sync/atomic.h>

fs_inode_t *pseudofs_def_lookup(fs_inode_t *parent, char *name) {
    return NULL;
}

int pseudofs_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

int pseudofs_def_rmdir(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

int pseudofs_def_mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

int pseudofs_def_rmregfile(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

int pseudofs_def_open(fs_inode_t *inode, fs_file_t *f) {
    f->data0 = inode->file_data0;
    f->data1 = inode->file_data1;
    return 0;
}

int pseudofs_def_close(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static int listdir(fs_file_t *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen) {
    int i = 0;
    fs_dentry_t *d;
    list_for_each_entry(d, &f->dentry->children, siblings) {
        if (i >= offset) {
            if (i - offset >= listlen) {
                break;
            }

            fs_listdir_t *dir = &dirlist[i - offset];
            strncpy(dir->name, d->name, sizeof(dir->name));
            dir->isdir = d->inode->mode & FS_ACCESS_MODE_DIR;
        }

        i++;
    }
    return i < offset ? 0 : i - offset;
}

static fs_inode_t *alloc_inode(fs_superblock_t *sb) {
    fs_inode_t *inode = calloc(1, sizeof(fs_inode_t));
    if (inode == NULL) {
        error("No memory available for pseudofs_inode_t structure");
        return NULL;
    }

    inode->sb = sb;

    inode->number = atomic32_inc(&((pseudofs_sb_t *) sb)->next_inode_number);

    inode->ops.lookup = pseudofs_def_lookup;
    inode->ops.mkdir = pseudofs_def_mkdir;
    inode->ops.rmdir = pseudofs_def_rmdir;
    inode->ops.mkregfile = pseudofs_def_mkregfile;
    inode->ops.rmregfile = pseudofs_def_rmregfile;

    inode->fops.open = pseudofs_def_open;
    inode->fops.close = pseudofs_def_close;
    inode->fops.listdir = listdir;
    return (fs_inode_t *) inode;
}

static void free_inode(fs_inode_t *inode) {
    verbose("Freeing inode #%lu", inode != NULL ? inode->number : 0);
    free(inode);
}

static int unmount(fs_mount_t *fsm) {
    free(fsm->sb);
    return 0;
}

static int mount(fs_type_t *fstype, fs_mount_t *m) {
    pseudofs_sb_t *psb = calloc(1, sizeof(pseudofs_sb_t));
    if (psb == NULL) {
        error("No memory available for pseudofs_sb_t structure");
        goto error_sb;
    }
    m->sb = (fs_superblock_t *) psb;
    m->sb->fstype = fstype;
    m->sb->ops.alloc_inode = alloc_inode;
    m->sb->ops.free_inode = free_inode;

    m->ops.unmount = unmount;

    atomic32_init(&psb->next_inode_number, 0);

    m->sb->root = alloc_inode(m->sb);

    return 0;

error_sb:
    return -1;
}

FILESYSTEM_MODULE(pseudofs, mount);



#ifdef CONFIG_TEST_CORE_FS_PSEUDOFS_CORE
#include __FILE__
#endif
