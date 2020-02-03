#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <mm/heap.h>
#include <generated/autoconf.h>


int vfs_dentry_init_root() {
    INIT_LIST_HEAD(&_laritos.fs.root.children);
    INIT_LIST_HEAD(&_laritos.fs.root.siblings);
    strncpy(_laritos.fs.root.name, "/", sizeof(_laritos.fs.root.name));
    return 0;
}

void vfs_dentry_add_child(fs_dentry_t *parent, fs_dentry_t *child) {
    list_add_tail(&child->siblings, &parent->children);
    child->parent = parent;
}

void vfs_dentry_remove_as_child(fs_dentry_t *child) {
    child->parent = NULL;
    list_del_init(&child->siblings);
}

fs_dentry_t *vfs_dentry_alloc(char *name, fs_inode_t *inode, fs_dentry_t *parent) {
    fs_dentry_t *d = calloc(1, sizeof(fs_dentry_t));
    if (d == NULL) {
        return NULL;
    }
    INIT_LIST_HEAD(&d->children);
    INIT_LIST_HEAD(&d->siblings);
    strncpy(d->name, name, sizeof(d->name));
    d->inode = inode;

    if (parent != NULL) {
        vfs_dentry_add_child(parent, d);
    }
    return d;
}

void vfs_dentry_free(fs_dentry_t *d) {
    free(d);
}

static inline fs_dentry_t *find_children(fs_dentry_t *parent, char *relpath) {
    size_t namelen = strlen(relpath);
    char *nextpath = strchr(relpath, '/');
    if (nextpath != NULL) {
        namelen = min(namelen, (size_t) (nextpath - relpath));
    }

    fs_dentry_t *d;
    list_for_each_entry(d, &parent->children, siblings) {
        if (strncmp(d->name, relpath, namelen) == 0 && d->name[namelen] == '\0') {
            return d;
        }
    }
    return NULL;
}

fs_dentry_t *vfs_dentry_lookup_from(fs_dentry_t *parent, char *relpath) {
    if (relpath == NULL || strlen(relpath) == 0) {
        return NULL;
    }

    fs_dentry_t *d = parent;
    while (true) {
        d = find_children(d, relpath);
        if (d == NULL) {
            break;
        }
        relpath = strchr(relpath, '/');
        // If there are no more children to lookup or there is is a / with no
        // children, e.g. a/b/c/
        if (relpath == NULL || relpath[1] == '\0') {
            break;
        }
        relpath++;
    }
    return d;
}

fs_dentry_t *vfs_dentry_lookup(char *path) {
    if (path == NULL) {
        return NULL;
    }

    if (path[0] == '/') {
        if (path[1] == '\0') {
            return &_laritos.fs.root;
        }
        path++;
        return vfs_dentry_lookup_from(&_laritos.fs.root, path);
    }

    // TODO Use cwd dentry instead
    return vfs_dentry_lookup_from(&_laritos.fs.root, path);
}



#ifdef CONFIG_TEST_CORE_FS_VFS_DENTRY
#include __FILE__
#endif
