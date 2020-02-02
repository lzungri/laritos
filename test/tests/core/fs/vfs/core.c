#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

static fs_mount_t *empty_mount(fs_type_t *fstype, char *mount_point, uint16_t flags, void *params) {
    return NULL;
}

T(vfs_register_fs_type_adds_a_new_fstype_to_the_supported_systems) {
    fs_type_t fst = {
        .id = "newfs",
        .mount = empty_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_unregister_fstype_fails_on_nonregistered_fstype) {
    fs_type_t fst = {
        .id = "non_existent",
        .mount = empty_mount,
    };
    tassert(vfs_unregister_fs_type(NULL) < 0);
    tassert(vfs_unregister_fs_type(&fst) < 0);
TEND

T(vfs_cannot_register_invalid_fs_types) {
    fs_type_t fst = {
        .id = "",
        .mount = empty_mount,
    };
    tassert(vfs_register_fs_type(&fst) < 0);

    fs_type_t fst2 = {
        .id = "123",
        .mount = NULL,
    };
    tassert(vfs_register_fs_type(&fst2) < 0);
TEND

T(vfs_cannot_register_two_or_more_fstypes_with_the_same_id) {
    fs_type_t fst = {
        .id = "testfs",
        .mount = empty_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    fs_type_t fst2 = {
        .id = "testfs",
        .mount = empty_mount,
    };
    tassert(vfs_register_fs_type(&fst2) < 0);

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_mount_fails_on_unsupported_fs_type) {
    tassert(vfs_mount_fs("unsopported", "/xxx", 0, NULL) == NULL);
    tassert(!vfs_is_fs_mounted("/xxx"));
TEND

T(vfs_mount_fails_on_mount_error) {
    fs_type_t fst = {
        .id = "testfs",
        .mount = empty_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    tassert(vfs_mount_fs("testfs", "/xxx", 0, NULL) == NULL);
    tassert(!vfs_is_fs_mounted("/xxx"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_unmount_fs_fails_on_non_mounted_fs) {
    tassert(vfs_unmount_fs("unmounted") < 0);
TEND

fs_mount_t dummy_mnt = {
    .sb = &(fs_superblock_t) {
    }
};

static fs_mount_t *dummy_mount(fs_type_t *fstype, char *mount_point, uint16_t flags, void *params) {
    vfs_initialize_mount_struct(&dummy_mnt, fstype, mount_point, flags, params);
    return &dummy_mnt;
}

T(vfs_mount_adds_a_new_fs_under_mount_point) {
    fs_type_t fst = {
        .id = "dummymnt",
        .mount = dummy_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    fs_mount_t *fsm = vfs_mount_fs("dummymnt", "/dummymnt", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm == &dummy_mnt);
    tassert(fsm->flags == (FS_MOUNT_READ | FS_MOUNT_WRITE));
    tassert(strncmp(fsm->root.name, "/dummymnt", sizeof(fsm->root.name)) == 0);
    tassert(fsm->sb->fstype == &fst);
    tassert(vfs_is_fs_mounted("/dummymnt"));

    vfs_unmount_fs("/dummymnt");
    tassert(!vfs_is_fs_mounted("/dummymnt"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

fs_mount_t nosb_mnt = {
    .sb = NULL,
};

static fs_mount_t *nosb_mount(fs_type_t *fstype, char *mount_point, uint16_t flags, void *params) {
    vfs_initialize_mount_struct(&nosb_mnt, fstype, mount_point, flags, params);
    return &nosb_mnt;
}

T(vfs_mount_fails_when_no_superblock_is_instantiated) {
    fs_type_t fst = {
        .id = "testnosb",
        .mount = nosb_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    tassert(vfs_mount_fs("testnosb", "/testnosb", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL) == NULL);
    tassert(!vfs_is_fs_mounted("/testnosb"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_mount_fails_if_mount_point_is_already_used) {
    fs_type_t fst = {
        .id = "mnt_used",
        .mount = dummy_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    fs_mount_t *fsm = vfs_mount_fs("mnt_used", "/mnt_used", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_is_fs_mounted("/mnt_used"));
    tassert(vfs_mount_fs("mnt_used", "/mnt_used", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL) == NULL);
    tassert(vfs_is_fs_mounted("/mnt_used"));

    vfs_unmount_fs("/mnt_used");
    tassert(!vfs_is_fs_mounted("/mnt_used"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_cannot_unmount_inexistent_mount_point) {
    tassert(vfs_unmount_fs("/non_existent_mount_point") < 0);
TEND
