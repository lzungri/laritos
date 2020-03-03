#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <core.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>


static fs_superblock_t dummy_sb = {
    .ops = {
        .alloc_inode = vfs_inode_def_alloc,
        .free_inode = vfs_inode_def_free,
    },
};

static int dummy_mount(fs_type_t *fstype, fs_mount_t *fsm) {
    dummy_sb.fstype = fstype;
    fsm->sb = &dummy_sb;
    fsm->sb->root = fsm->sb->ops.alloc_inode(fsm->sb);
    return 0;
}

fs_inode_t *dummy_inode_lookup(fs_inode_t *parent, char *name) {
    return NULL;
}

static fs_superblock_t minimal_sb = {
    .ops = {
        .alloc_inode = NULL,
        .free_inode = NULL,
    },
};

fs_inode_t minimal_inode = {
    .sb = &minimal_sb,
    .ops = {
        .lookup = dummy_inode_lookup,
    }
};

T(vfs_creates_a_slash_root_dentry) {
    tassert(vfs_dentry_lookup("/") == _laritos.fs.root);
TEND

T(vfs_mount_creates_a_new_dentry_for_the_mounting_point) {
    fs_type_t fst = {
        .id = "dummymnt",
        .mount = dummy_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    fs_mount_t *fsm = vfs_mount_fs("dummymnt", "/dummymnt", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(fsm->flags == (FS_MOUNT_READ | FS_MOUNT_WRITE));
    tassert(strncmp(fsm->root->name, "dummymnt", sizeof(fsm->root->name)) == 0);
    tassert(fsm->sb->fstype == &fst);
    tassert(file_exist("/dummymnt"));

    tassert(vfs_unmount_fs("/dummymnt") >= 0);
    tassert(!file_exist("/dummymnt"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_dentry_initializes_parent_and_child_properly) {
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", NULL, NULL);
    tassert(d1 != NULL);
    tassert(strncmp(d1->name, "d1", sizeof(d1->name)) == 0);
    tassert(d1->inode == NULL);
    tassert(d1->parent == NULL);
    tassert(list_empty(&d1->children));
    tassert(list_empty(&d1->siblings));

    fs_dentry_t *d2 = vfs_dentry_alloc("d2", NULL, d1);
    tassert(d2 != NULL);
    tassert(strncmp(d2->name, "d2", sizeof(d2->name)) == 0);
    tassert(d2->inode == NULL);
    tassert(d2->parent == d1);
    tassert(list_empty(&d2->children));
    tassert(!list_empty(&d2->siblings));
    tassert(!list_empty(&d1->children));

    vfs_dentry_remove_as_child(d2);
    vfs_dentry_remove_as_child(d1);

    vfs_dentry_free(d2);
    vfs_dentry_free(d1);
TEND

T(vfs_dentry_lookup_returns_null_when_relpath_not_found) {
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", &minimal_inode, NULL);
    tassert(d1 != NULL);
    tassert(vfs_dentry_lookup_from(d1, "d2") == NULL);
    vfs_dentry_free(d1);
TEND

T(vfs_dentry_lookup_returns_child_dentry_when_relpath_found) {
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", &minimal_inode, NULL);
    tassert(d1 != NULL);
    fs_dentry_t *d2 = vfs_dentry_alloc("d2", &minimal_inode, d1);
    tassert(d2 != NULL);
    fs_dentry_t *d3 = vfs_dentry_alloc("d3", &minimal_inode, d2);
    tassert(d3 != NULL);
    fs_dentry_t *d4 = vfs_dentry_alloc("d4", &minimal_inode, d3);
    tassert(d4 != NULL);

    tassert(vfs_dentry_lookup_from(d1, "d2") == d2);
    tassert(vfs_dentry_lookup_from(d1, "d2/") == d2);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3") == d3);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3/") == d3);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3/d4") == d4);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3/d4/") == d4);

    tassert(vfs_dentry_lookup_from(d2, "d3") == d3);
    tassert(vfs_dentry_lookup_from(d2, "d3/d4") == d4);

    vfs_dentry_remove_as_child(d2);

    tassert(vfs_dentry_lookup_from(d1, "d2") == NULL);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3/d4/") == NULL);
    tassert(vfs_dentry_lookup_from(d2, "d3/d4") == d4);

    vfs_dentry_remove_as_child(d4);

    tassert(vfs_dentry_lookup_from(d2, "d3/d4") == NULL);

    vfs_dentry_remove_as_child(d3);

    tassert(vfs_dentry_lookup_from(d2, "d3") == NULL);

    tassert(list_empty(&d1->children));
    tassert(list_empty(&d1->siblings));
    tassert(list_empty(&d2->children));
    tassert(list_empty(&d2->siblings));
    tassert(list_empty(&d3->children));
    tassert(list_empty(&d3->siblings));
    tassert(list_empty(&d4->children));
    tassert(list_empty(&d4->siblings));

    vfs_dentry_free(d4);
    vfs_dentry_free(d3);
    vfs_dentry_free(d2);
    vfs_dentry_free(d1);
TEND

T(vfs_dentry_lookup_works_properly_with_double_dots_dir) {
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", &minimal_inode, NULL);
    tassert(d1 != NULL);
    fs_dentry_t *d2 = vfs_dentry_alloc("d2", &minimal_inode, d1);
    tassert(d2 != NULL);
    fs_dentry_t *d3 = vfs_dentry_alloc("d3", &minimal_inode, d2);
    tassert(d3 != NULL);
    fs_dentry_t *d4 = vfs_dentry_alloc("d4", &minimal_inode, d3);
    tassert(d4 != NULL);

    tassert(vfs_dentry_lookup_from(d1, "d2") == d2);
    tassert(vfs_dentry_lookup_from(d1, "d2/..") == d1);
    tassert(vfs_dentry_lookup_from(d1, "d2/../") == d1);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3/../") == d2);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3/../../") == d1);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3/d4/../../../") == d1);

    tassert(vfs_dentry_lookup_from(d2, "") == d2);
    tassert(vfs_dentry_lookup_from(d2, "..") == d1);
    tassert(vfs_dentry_lookup_from(d2, "../d2/d3/d4/") == d4);

    vfs_dentry_free(d4);
    vfs_dentry_free(d3);
    vfs_dentry_free(d2);
    vfs_dentry_free(d1);
TEND

T(vfs_dentry_lookup_works_properly_with_multiple_consecutive_slashes) {
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", &minimal_inode, NULL);
    tassert(d1 != NULL);
    fs_dentry_t *d2 = vfs_dentry_alloc("d2", &minimal_inode, d1);
    tassert(d2 != NULL);
    fs_dentry_t *d3 = vfs_dentry_alloc("d3", &minimal_inode, d2);
    tassert(d3 != NULL);
    fs_dentry_t *d4 = vfs_dentry_alloc("d4", &minimal_inode, d3);
    tassert(d4 != NULL);

    tassert(vfs_dentry_lookup_from(d1, "d2") == d2);
    tassert(vfs_dentry_lookup_from(d1, "d2//") == d2);
    tassert(vfs_dentry_lookup_from(d1, "d2///d3") == d3);
    tassert(vfs_dentry_lookup_from(d1, "d2/d3////") == d3);
    tassert(vfs_dentry_lookup_from(d1, "d2//////d3////d4") == d4);
    tassert(vfs_dentry_lookup_from(d1, "d2///d3/d4//") == d4);

    vfs_dentry_free(d4);
    vfs_dentry_free(d3);
    vfs_dentry_free(d2);
    vfs_dentry_free(d1);
TEND

T(vfs_dentry_get_fullpath_returns_all_dentries_until_parent) {
    fs_dentry_t *root = vfs_dentry_alloc("", &minimal_inode, NULL);
    tassert(root != NULL);
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", &minimal_inode, root);
    tassert(d1 != NULL);
    fs_dentry_t *d2 = vfs_dentry_alloc("d2", &minimal_inode, d1);
    tassert(d2 != NULL);
    fs_dentry_t *d3 = vfs_dentry_alloc("d3", &minimal_inode, d2);
    tassert(d3 != NULL);
    fs_dentry_t *d4 = vfs_dentry_alloc("d4", &minimal_inode, d3);
    tassert(d4 != NULL);

    char path[128] = { 0 };
    vfs_dentry_get_fullpath(root, path, sizeof(path));
    tassert(strncmp("/", path, sizeof(path)) == 0);
    vfs_dentry_get_fullpath(d1, path, sizeof(path));
    tassert(strncmp("/d1", path, sizeof(path)) == 0);
    vfs_dentry_get_fullpath(d2, path, sizeof(path));
    tassert(strncmp("/d1/d2", path, sizeof(path)) == 0);
    vfs_dentry_get_fullpath(d3, path, sizeof(path));
    tassert(strncmp("/d1/d2/d3", path, sizeof(path)) == 0);
    vfs_dentry_get_fullpath(d4, path, sizeof(path));
    tassert(strncmp("/d1/d2/d3/d4", path, sizeof(path)) == 0);

    vfs_dentry_free(d4);
    vfs_dentry_free(d3);
    vfs_dentry_free(d2);
    vfs_dentry_free(d1);
    vfs_dentry_free(root);
TEND
