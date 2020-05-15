/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <printf.h>
#include <core.h>
#include <test/test.h>
#include <test/utils/fs.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <component/component.h>
#include <fs/ext2.h>
#include <fs/file.h>
#include <endian.h>
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
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/mkdir"));
DATAIMG_TEND

DATAIMG_T(ext2, ext2_mkdir_works_as_expected_on_dirs_with_max_filename_length) {
    char dname[CONFIG_FS_MAX_FILENAME_LEN] = { 0 };
    int i;
    for (i = 0; i < sizeof(dname) - 1; i++) {
        dname[i] = '0' + i % 10;
    }
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), dname,
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    tassert(dir != NULL);
    tassert(fs_file_in_listdir(DATA_TEST_DIR, dname));
DATAIMG_TEND

DATAIMG_T(ext2, ext2_creating_mult_dirs_spanning_from_mult_blks_doesnt_corrupt_fs) {
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), "multipledirs",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/multipledirs"));

    int i;
    for (i = 0; i < 50; i++) {
        char buf[EXT2_NAME_LEN] = { 0 };
        snprintf(buf, sizeof(buf), "long_directory_name_number_%d", i);
        fs_dentry_t *child = vfs_dir_create(dir, buf, FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
        tassert(child != NULL);
        tassert(fs_file_in_listdir(DATA_TEST_DIR "/multipledirs", buf));
    }
DATAIMG_TEND

DATAIMG_T(ext2, ext2_listdir_returns_new_direntry) {
    tassert(!fs_file_in_listdir(DATA_TEST_DIR, "newdir"));
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), "newdir",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/newdir"));
    tassert(fs_file_in_listdir(DATA_TEST_DIR, "newdir"));
DATAIMG_TEND

DATAIMG_T(ext2, ext2_mkdir_fails_on_readonly_parent) {
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), "rodir",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/rodir"));

    fs_dentry_t *child = vfs_dir_create(dir, "child",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(child == NULL);
    tassert(!file_is_dir(DATA_TEST_DIR "/rodir/child"));
    tassert(!fs_file_in_listdir(DATA_TEST_DIR "/rodir", "child"));
DATAIMG_TEND

DATAIMG_T(ext2, ext2_mkregfile_creates_a_new_file_with_size_0_and_1_data_block) {
    fs_dentry_t *f = vfs_file_create(fs_get_data_testdir(), "regfile",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);
    tassert(file_exist(DATA_TEST_DIR "/regfile"));
    tassert(!file_is_dir(DATA_TEST_DIR "/regfile"));
    tassert(fs_file_in_listdir(DATA_TEST_DIR, "regfile"));

    ext2_sb_t *sb = (ext2_sb_t *) f->inode->sb;
    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) f->inode->sb, f->inode->number, &physnode) >= 0);
    tassert(physnode.size == 0);
    tassert(inode_nblocks(sb, &physnode) == 1);
DATAIMG_TEND

DATAIMG_T(ext2, ext2_mkregfile_fails_on_readonly_parent) {
    fs_dentry_t *dir = vfs_dir_create(fs_get_data_testdir(), "rodir2",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir != NULL);
    tassert(file_is_dir(DATA_TEST_DIR "/rodir2"));

    fs_dentry_t *f = vfs_file_create(dir, "child",
            FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(f == NULL);
    tassert(!file_is_dir(DATA_TEST_DIR "/rodir2/child"));
    tassert(!fs_file_in_listdir(DATA_TEST_DIR "/rodir2", "child"));
DATAIMG_TEND

DATAIMG_T(ext2, ext2_writing_n_bytes_change_file_size_to_n) {
    fs_dentry_t *d = vfs_file_create(fs_get_data_testdir(), "nbytes", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(d != NULL);

    fs_file_t *f = vfs_file_open(DATA_TEST_DIR "/nbytes", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    tassert(vfs_file_write(f, buf, sizeof(buf), 0) == sizeof(buf));
    vfs_file_close(f);

    ext2_sb_t *sb = (ext2_sb_t *) d->inode->sb;
    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sizeof(buf));
    tassert(inode_nblocks(sb, &physnode) == 1);
DATAIMG_TEND

DATAIMG_T(ext2, ext2_writing_data_updates_corresponding_block) {
    fs_dentry_t *d = vfs_file_create(fs_get_data_testdir(), "flushblock", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(d != NULL);

    fs_file_t *f = vfs_file_open(DATA_TEST_DIR "/flushblock", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char wbuf[] = "HELLO";
    tassert(vfs_file_write(f, wbuf, sizeof(wbuf), 0) == sizeof(wbuf));
    vfs_file_close(f);

    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sizeof(wbuf));

    f = vfs_file_open(DATA_TEST_DIR "/flushblock", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char rbuf[16];
    tassert(vfs_file_read(f, rbuf, sizeof(rbuf), 0) == sizeof(wbuf));
    vfs_file_close(f);
    tassert(strncmp(rbuf, "HELLO", sizeof(rbuf)) == 0);
DATAIMG_TEND

DATAIMG_T(ext2, ext2_overwriting_data_doesnt_change_file_size) {
    fs_dentry_t *d = vfs_file_create(fs_get_data_testdir(), "overwrite", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(d != NULL);

    fs_file_t *f = vfs_file_open(DATA_TEST_DIR "/overwrite", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char wbuf[] = "HELLO";
    tassert(vfs_file_write(f, wbuf, sizeof(wbuf), 0) == sizeof(wbuf));
    vfs_file_close(f);
    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sizeof(wbuf));

    f = vfs_file_open(DATA_TEST_DIR "/overwrite", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char ovbuf = 'A';
    tassert(vfs_file_write(f, &ovbuf, sizeof(ovbuf), 0) == sizeof(ovbuf));
    vfs_file_close(f);
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sizeof(wbuf));

    f = vfs_file_open(DATA_TEST_DIR "/overwrite", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char rbuf[16];
    tassert(vfs_file_read(f, rbuf, sizeof(rbuf), 0) == sizeof(wbuf));
    vfs_file_close(f);
    tassert(strncmp(rbuf, "AELLO", sizeof(rbuf)) == 0);
DATAIMG_TEND

DATAIMG_T(ext2, ext2_writing_blocksize_plus1_bytes_allocates_new_block) {
    fs_dentry_t *d = vfs_file_create(fs_get_data_testdir(), "2blocks", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(d != NULL);

    ext2_sb_t *sb = (ext2_sb_t *) d->inode->sb;

    fs_file_t *f = vfs_file_open(DATA_TEST_DIR "/2blocks", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    int i;
    for (i = 0; i < sb->block_size; i += sizeof(buf)) {
        tassert(vfs_file_write(f, buf, sizeof(buf), i) == sizeof(buf));
    }
    tassert(vfs_file_write(f, buf, 1, i) == 1);
    vfs_file_close(f);

    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sb->block_size + 1);
    tassert(inode_nblocks(sb, &physnode) == 2);

    f = vfs_file_open(DATA_TEST_DIR "/2blocks", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    for (i = 0; i < sb->block_size; i += sizeof(buf)) {
        tassert(vfs_file_read(f, buf, sizeof(buf), i) == sizeof(buf));
        tassert(memcmp(buf, (char []) { 0, 1, 2, 3, 4, 5, 6, 7 }, sizeof(buf)) == 0);
    }
    tassert(vfs_file_read(f, buf, 1, i) == 1);
    tassert(buf[0] == 0);
    vfs_file_close(f);
DATAIMG_TEND

DATAIMG_T(ext2, ext2_writing_data_at_blocksize_offset_allocates_new_block) {
    fs_dentry_t *d = vfs_file_create(fs_get_data_testdir(), "bsoffset", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(d != NULL);

    ext2_sb_t *sb = (ext2_sb_t *) d->inode->sb;
    fs_file_t *f = vfs_file_open(DATA_TEST_DIR "/bsoffset", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char wbuf[] = "HELLO";
    tassert(vfs_file_write(f, wbuf, sizeof(wbuf), sb->block_size) == sizeof(wbuf));
    vfs_file_close(f);

    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sb->block_size + sizeof(wbuf));
    tassert(inode_nblocks(sb, &physnode) == 2);

    f = vfs_file_open(DATA_TEST_DIR "/bsoffset", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char rbuf[16];
    tassert(vfs_file_read(f, rbuf, sizeof(rbuf), sb->block_size) == sizeof(wbuf));
    vfs_file_close(f);
    tassert(strncmp(rbuf, "HELLO", sizeof(rbuf)) == 0);
DATAIMG_TEND

DATAIMG_T(ext2, ext2_writing_data_at_blocksize_boundaries_allocates_new_block) {
    fs_dentry_t *d = vfs_file_create(fs_get_data_testdir(), "bsoffset2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(d != NULL);

    ext2_sb_t *sb = (ext2_sb_t *) d->inode->sb;
    fs_file_t *f = vfs_file_open(DATA_TEST_DIR "/bsoffset2", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char wbuf[] = "HELLO";
    tassert(vfs_file_write(f, wbuf, sizeof(wbuf), sb->block_size - 1) == sizeof(wbuf));
    vfs_file_close(f);

    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sb->block_size - 1 + sizeof(wbuf));
    tassert(inode_nblocks(sb, &physnode) == 2);

    f = vfs_file_open(DATA_TEST_DIR "/bsoffset2", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char rbuf[16];
    tassert(vfs_file_read(f, rbuf, sizeof(rbuf), sb->block_size - 1) == sizeof(wbuf));
    vfs_file_close(f);
    tassert(strncmp(rbuf, "HELLO", sizeof(rbuf)) == 0);
DATAIMG_TEND

DATAIMG_T(ext2, ext2_writing_data_at_logblock_num10_allocates_logblocks_from1to10) {
    fs_dentry_t *d = vfs_file_create(fs_get_data_testdir(), "bsoffset3", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(d != NULL);

    ext2_sb_t *sb = (ext2_sb_t *) d->inode->sb;
    fs_file_t *f = vfs_file_open(DATA_TEST_DIR "/bsoffset3", FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char wbuf[] = "HELLO";
    tassert(vfs_file_write(f, wbuf, sizeof(wbuf), sb->block_size * 10 - 1) == sizeof(wbuf));
    vfs_file_close(f);

    ext2_inode_data_t physnode;
    tassert(read_inode_from_dev((ext2_sb_t *) d->inode->sb, d->inode->number, &physnode) >= 0);
    tassert(physnode.size == sb->block_size * 10 - 1 + sizeof(wbuf));
    tassert(inode_nblocks(sb, &physnode) == 11);

    f = vfs_file_open(DATA_TEST_DIR "/bsoffset3", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char rbuf[16];
    tassert(vfs_file_read(f, rbuf, sizeof(rbuf), sb->block_size * 10 - 1) == sizeof(wbuf));
    vfs_file_close(f);
    tassert(strncmp(rbuf, "HELLO", sizeof(rbuf)) == 0);
DATAIMG_TEND
