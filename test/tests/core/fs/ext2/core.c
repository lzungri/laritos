#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/ext2.h>
#include <fs/file.h>
#include <utils/endian.h>

T(ext2_mount_adds_a_new_fs_under_mount_point) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
        (fs_param_t []) {
            { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
            { NULL },
        });
    tassert(fsm != NULL);
    tassert(fsm->flags == (FS_MOUNT_READ | FS_MOUNT_WRITE));
    tassert(strncmp(fsm->root->name, "test", sizeof(fsm->root->name)) == 0);
    tassert(file_is_dir("/test"));

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
TEND

T(ext2_reading_small_file_returns_the_right_data) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
        (fs_param_t []) {
            { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
            { NULL },
        });
    tassert(fsm != NULL);

    tassert(file_exist("/test/test/systemimg/small.txt"));
    fs_file_t *f = vfs_file_open("/test/test/systemimg/small.txt", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[128] = { 0 };
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, 0) >= 0);
    tassert(strncmp(buf, "This is a small file", sizeof(buf)) == 0);

    vfs_file_close(f);

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
TEND

T(ext2_reading_small_file_with_offset_returns_the_right_data) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
        (fs_param_t []) {
            { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
            { NULL },
        });
    tassert(fsm != NULL);

    tassert(file_exist("/test/test/systemimg/small.txt"));
    fs_file_t *f = vfs_file_open("/test/test/systemimg/small.txt", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[128] = { 0 };
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, 5) >= 0);
    tassert(strncmp(buf, "is a small file", sizeof(buf)) == 0);

    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, strlen("This is a small file")) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, strlen("This is a small file") + 1) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, strlen("This is a small file") + 100) == 0);

    vfs_file_close(f);

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
TEND

T(ext2_reading_medium_sized_file_returns_the_right_data) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
        (fs_param_t []) {
            { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
            { NULL },
        });
    tassert(fsm != NULL);

    tassert(file_exist("/test/test/systemimg/medium.txt"));
    fs_file_t *f = vfs_file_open("/test/test/systemimg/medium.txt", FS_ACCESS_MODE_READ);
    tassert(f != NULL);

    uint32_t pos = 0;
    uint32_t value;
    while (vfs_file_read(f, &value, sizeof(value), pos * sizeof(value)) == sizeof(value)) {
        value = le32_to_cpu(value);
        tassert(value == pos);
        pos++;
    }
    tassert(value == 99);

    vfs_file_close(f);

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
TEND

T(ext2_reading_big_file_returns_the_right_data) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
        (fs_param_t []) {
            { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
            { NULL },
        });
    tassert(fsm != NULL);

    tassert(file_exist("/test/test/systemimg/big.txt"));
    fs_file_t *f = vfs_file_open("/test/test/systemimg/big.txt", FS_ACCESS_MODE_READ);
    tassert(f != NULL);

    uint32_t pos = 0;
    uint32_t value;
    while (vfs_file_read(f, &value, sizeof(value), pos * sizeof(value)) == sizeof(value)) {
        value = le32_to_cpu(value);
        tassert(value == pos);
        pos++;
    }
    tassert(value == 9999);

    vfs_file_close(f);

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
TEND

T(ext2_reading_huge_file_returns_the_right_data) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
        (fs_param_t []) {
            { "mem-offset", TOSTRING(CONFIG_FS_SYSTEM_IMAGE_OFFSET) },
            { NULL },
        });
    tassert(fsm != NULL);

    tassert(file_exist("/test/test/systemimg/huge.txt"));
    fs_file_t *f = vfs_file_open("/test/test/systemimg/huge.txt", FS_ACCESS_MODE_READ);
    tassert(f != NULL);

    uint32_t pos = 0;
    uint32_t value;
    while (vfs_file_read(f, &value, sizeof(value), pos * sizeof(value)) == sizeof(value)) {
        value = le32_to_cpu(value);
        tassert(value == pos);
        pos++;
    }
    tassert(value == 999999);

    vfs_file_close(f);

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
TEND
