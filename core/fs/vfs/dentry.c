//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <core.h>
#include <string.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <mm/heap.h>
#include <utils/file.h>
#include <process/core.h>
#include <generated/autoconf.h>


void vfs_dentry_add_child(fs_dentry_t *parent, fs_dentry_t *child) {
    list_add_tail(&child->siblings, &parent->children);
    child->parent = parent;
    verbose("New child of '%s': '%s'", parent->name, child->name);
}

void vfs_dentry_remove_as_child(fs_dentry_t *child) {
    verbose("Del child of '%s': '%s'", child->parent ? child->parent->name : "null", child->name);
    child->parent = NULL;
    list_del_init(&child->siblings);
}

void vfs_dentry_init(fs_dentry_t *d, char *name, fs_inode_t *inode, fs_dentry_t *parent) {
    INIT_LIST_HEAD(&d->children);
    INIT_LIST_HEAD(&d->siblings);
    strncpy(d->name, name, sizeof(d->name));
    d->inode = inode;

    if (parent != NULL) {
        vfs_dentry_add_child(parent, d);
    }
}

fs_dentry_t *vfs_dentry_alloc(char *name, fs_inode_t *inode, fs_dentry_t *parent) {
    fs_dentry_t *d = calloc(1, sizeof(fs_dentry_t));
    if (d == NULL) {
        return NULL;
    }
    vfs_dentry_init(d, name, inode, parent);
    return d;
}

void vfs_dentry_free(fs_dentry_t *d) {
    verbose("Freeing '%s'", d->name);
    vfs_dentry_remove_as_child(d);
    free(d);
}

static inline fs_dentry_t *find_children(fs_dentry_t *parent, char *relpath) {
    if (parent == NULL) {
        return NULL;
    }
    verbose_async("find_children('%s', '%s')", parent->name, relpath);

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

    fs_dentry_t *cwd = vfs_dentry_lookup(process_get_current()->cwd);
    return vfs_dentry_lookup_from(cwd, path);
}

fs_dentry_t *vfs_dentry_lookup_parent(char *path) {
    char abs_parent[CONFIG_FS_MAX_FILENAME_LEN];
    file_get_abs_dirname(path, abs_parent, sizeof(abs_parent));
    return vfs_dentry_lookup(abs_parent);
}

void vfs_dentry_free_tree(fs_dentry_t *root) {
    fs_dentry_t *d;
    fs_dentry_t *temp;
    list_for_each_entry_safe(d, temp, &root->children, siblings) {
        vfs_dentry_free_tree(d);
    }
    if (root->inode != NULL && root->inode->sb->ops.free_inode != NULL) {
        root->inode->sb->ops.free_inode(root->inode);
        root->inode = NULL;
    }
    vfs_dentry_free(root);
}

bool vfs_dentry_exist(char *path) {
    return vfs_dentry_lookup(path) != NULL;
}

bool vfs_dentry_is_dir(fs_dentry_t *d) {
    if (d == NULL || d->inode == NULL) {
        return false;
    }
    return d->inode->mode & FS_ACCESS_MODE_DIR;
}



#ifdef CONFIG_TEST_CORE_FS_VFS_DENTRY
#include __FILE__
#endif
