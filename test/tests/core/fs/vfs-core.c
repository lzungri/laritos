#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <test/test.h>
#include <fs/vfs-core.h>
#include <fs/vfs-types.h>

static bool is_fstype_registered(fs_type_t *fst) {
    return false;
}

static bool is_fs_mounted(char *mount_point) {
    return false;
}

fs_mount_t *empty_mount(fs_type_t *type, char *mount_point, uint16_t flags, void *params) {
    return NULL;
}

T(vfs_register_fs_type_adds_a_new_fstype_to_the_supported_systems) {
    fs_type_t fst = {
        .id = "newfs",
        .mount = empty_mount,
    };
    tassert(vfs_register_fs_type(&fst));
    tassert(is_fstype_registered(&fst));

    vfs_unregister_fs_type(&fst);
    tassert(!is_fstype_registered(&fst));
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
    tassert(vfs_register_fs_type(&fst));
    tassert(is_fstype_registered(&fst));

    fs_type_t fst2 = {
        .id = "testfs",
        .mount = empty_mount,
    };
    tassert(vfs_register_fs_type(&fst2) < 0);

    vfs_unregister_fs_type(&fst);
    tassert(!is_fstype_registered(&fst));
TEND

T(vfs_mount_fails_on_unsupported_fs_type) {
    tassert(vfs_mount_fs("unsopported", "xxx", 0, NULL) == NULL);
TEND

T(vfs_unmount_fs_fails_on_non_mounted_fs) {
    tassert(vfs_unmount_fs("unmounted") < 0);
TEND

T(vfs_mount_adds_a_new_fs_under_mount_point) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(is_fs_mounted("/test"));

    vfs_unmount_fs("/test");
    tassert(!is_fs_mounted("/test"));
TEND
