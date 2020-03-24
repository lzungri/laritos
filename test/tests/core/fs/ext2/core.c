#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <core.h>
#include <test/test.h>
#include <test/utils/fs.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <component/component.h>
#include <fs/ext2.h>
#include <fs/file.h>
#include <utils/endian.h>
#include <generated/autoconf.h>

SYSIMG_T(ext2, ext2_mount_fails_if_not_given_the_right_params) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
            (fs_param_t []) { { NULL } });
    tassert(fsm == NULL);
SYSIMG_TEND

SYSIMG_T(ext2, ext2_mount_fails_if_dev_is_null) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
            (fs_param_t []) { { "dev", NULL }, { NULL } });
    tassert(fsm == NULL);
SYSIMG_TEND

SYSIMG_T(ext2, ext2_mount_adds_a_new_fs_under_mount_point) {
    fs_mount_t *fsm = vfs_mount_fs("ext2", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE,
            (fs_param_t []) { { "dev", component_get_by_id("flash1") }, { NULL } });
    tassert(fsm != NULL);
    tassert(fsm->flags == (FS_MOUNT_READ | FS_MOUNT_WRITE));
    tassert(strncmp(fsm->root->name, "test", sizeof(fsm->root->name)) == 0);
    tassert(file_is_dir("/test"));

    vfs_unmount_fs("/test");
    tassert(!file_exist("/test"));
SYSIMG_TEND

SYSIMG_T(ext2, ext2_reading_small_file_returns_the_right_data) {
    tassert(file_exist("/sys/test/systemimg/small.txt"));
    fs_file_t *f = vfs_file_open("/sys/test/systemimg/small.txt", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[128] = { 0 };
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, 0) >= 0);
    tassert(strncmp(buf, "This is a small file", sizeof(buf)) == 0);
    vfs_file_close(f);
SYSIMG_TEND

SYSIMG_T(ext2, ext2_reading_small_file_with_offset_returns_the_right_data) {
    tassert(file_exist("/sys/test/systemimg/small.txt"));
    fs_file_t *f = vfs_file_open("/sys/test/systemimg/small.txt", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[128] = { 0 };
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, 5) >= 0);
    tassert(strncmp(buf, "is a small file", sizeof(buf)) == 0);

    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, strlen("This is a small file")) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, strlen("This is a small file") + 1) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, strlen("This is a small file") + 100) == 0);

    vfs_file_close(f);
SYSIMG_TEND

SYSIMG_T(ext2, ext2_reading_medium_sized_file_returns_the_right_data) {
    tassert(file_exist("/sys/test/systemimg/medium.txt"));
    fs_file_t *f = vfs_file_open("/sys/test/systemimg/medium.txt", FS_ACCESS_MODE_READ);
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
SYSIMG_TEND

SYSIMG_T(ext2, ext2_reading_big_file_returns_the_right_data) {
    tassert(file_exist("/sys/test/systemimg/big.txt"));
    fs_file_t *f = vfs_file_open("/sys/test/systemimg/big.txt", FS_ACCESS_MODE_READ);
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
SYSIMG_TEND

SYSIMG_T(ext2, ext2_reading_huge_file_returns_the_right_data) {
    tassert(file_exist("/sys/test/systemimg/huge.txt"));
    fs_file_t *f = vfs_file_open("/sys/test/systemimg/huge.txt", FS_ACCESS_MODE_READ);
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
SYSIMG_TEND

DATAIMG_T(ext2, ext2_mkdir_creates_a_new_directory) {
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), "mkdir",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/mkdir"));
DATAIMG_TEND

DATAIMG_T(ext2, ext2_listdir_returns_new_direntry) {
    tassert(!fs_file_in_listdir(DATA_TEST_DIR, "rodir"));
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), "mkdir",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/mkdir"));
    tassert(fs_file_in_listdir(DATA_TEST_DIR, "rodir"));
DATAIMG_TEND

DATAIMG_T(ext2, ext2_mkdir_fails_on_readonly_parent) {
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), "rodir", FS_ACCESS_MODE_READ);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/rodir"));

    fs_dentry_t *dir2 = vfs_dir_create(dir, "child", FS_ACCESS_MODE_READ);
    tassert(dir2 == NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/rodir/dir2"));
    tassert(!fs_file_in_listdir(DATA_TEST_DIR "/rodir", "dir2"));
DATAIMG_TEND
