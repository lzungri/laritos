#include <log.h>

#include <stdint.h>
#include <test/test.h>
#include <vfs/core.h>
#include <vfs/types.h>

T(vfs_cannot_register_invalid_fs_types) {
    fs_type_t fst = {
        .id = "",
        .mount = NULL,
    };
    tassert(vfs_register_fs_type(&fst) < 0);

    fs_type_t fst2 = {
        .id = "123",
        .mount = NULL,
    };
    tassert(vfs_register_fs_type(&fst2) < 0);
TEND

T(vfs_register_fs_type_adds_a_new_fstype_to_the_supported_systems) {
    fs_type_t fst = {
        .id = "",
        .mount = NULL,
    };
    tassert(vfs_register_fs_type(&fst) < 0);





TEND

T(vfs_) {
//    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "sysinfo", flags, mem);

TEND
