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
    },
};

static int dummy_mount(fs_type_t *fstype, fs_mount_t *fsm) {
    dummy_sb.fstype = fstype;
    fsm->sb = &dummy_sb;
    return 0;
}

T(vfs_creates_a_slash_root_dentry) {
    tassert(vfs_dentry_lookup("/") == &_laritos.fs.root);
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
    tassert(vfs_dentry_exist("/dummymnt"));

    tassert(vfs_unmount_fs("/dummymnt") >= 0);
    tassert(!vfs_dentry_exist("/dummymnt"));

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

    vfs_dentry_free(d2);
    vfs_dentry_free(d1);
TEND

T(vfs_dentry_lookup_returns_null_when_relpath_not_found) {
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", NULL, NULL);
    tassert(d1 != NULL);
    tassert(vfs_dentry_lookup_from(d1, "d2") == NULL);
    vfs_dentry_free(d1);
TEND

T(vfs_dentry_lookup_returns_child_dentry_when_relpath_found) {
    fs_dentry_t *d1 = vfs_dentry_alloc("d1", NULL, NULL);
    tassert(d1 != NULL);
    fs_dentry_t *d2 = vfs_dentry_alloc("d2", NULL, d1);
    tassert(d2 != NULL);
    fs_dentry_t *d3 = vfs_dentry_alloc("d3", NULL, d2);
    tassert(d3 != NULL);
    fs_dentry_t *d4 = vfs_dentry_alloc("d4", NULL, d3);
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
