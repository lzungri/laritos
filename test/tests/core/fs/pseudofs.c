#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <utils/file.h>

static char rdata[32];
static char wdata[32];
static int file_dummy_open(fs_inode_t *inode, fs_file_t *f) {
    memset(rdata, 0, sizeof(rdata));
    memset(wdata, 0, sizeof(wdata));
    return 0;
}

static int file_dummy_close(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static int file_dummy_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    if (offset >= strlen(rdata) + 1) {
        return 0;
    }
    size_t len = min(strlen(rdata) - offset + 1, blen);
    memcpy(buf, rdata + offset, len);
    return len;
}

static int file_dummy_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    if (offset >= sizeof(wdata)) {
        return 0;
    }
    size_t len = min(sizeof(wdata) - offset, blen);
    memcpy(wdata + offset, buf, len);
    return len;
}

static fs_file_ops_t dummy_fop = {
    .open = file_dummy_open,
    .close = file_dummy_close,
    .read = file_dummy_read,
    .write = file_dummy_write,
};

T(pseudofs_fstype_is_supported_by_default) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));
    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
TEND

T(pseudofs_mount_populates_superblock) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));
    tassert(fsm->sb != NULL);
    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
TEND

T(pseudofs_dir_creation_instantiates_inode_and_dentry) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_dir_create(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(file_is_dir("/test/dir1/dir2"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
    tassert(!file_exist("/test/dir1/dir2"));
TEND

T(pseudofs_cannot_remove_non_existent_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    tassert(vfs_dir_remove(fsm->root, "hola") < 0);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
TEND

T(pseudofs_dir_removal_deletes_inodes_and_dentries) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_dir_create(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(file_is_dir("/test/dir1/dir2"));

    tassert(vfs_dir_remove(fsm->root, "dir1") >= 0);
    tassert(!file_exist("/test/dir1/dir2"));
    tassert(!file_exist("/test/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
    tassert(!file_exist("/test/dir1/dir2"));
TEND

T(pseudofs_dir_removal_deletes_inodes_and_dentries2) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_dir_create(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(file_is_dir("/test/dir1/dir2"));

    tassert(vfs_dir_remove(dir1, "dir2") >= 0);
    tassert(!file_exist("/test/dir1/dir2"));
    tassert(file_exist("/test/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
    tassert(!file_exist("/test/dir1/dir2"));
TEND

T(pseudofs_cannot_create_dir_if_file_or_dir_already_existing) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    tassert(vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE) == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
TEND

T(pseudofs_cannot_create_dir_if_parent_is_not_a_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_dentry_t *dir1 = vfs_dir_create(f1, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 == NULL);
    tassert(!file_exist("/test/f1/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
    tassert(!file_exist("/test/f1/dir1"));
TEND

T(pseudofs_file_creation_instantiates_dentry_and_inode) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(f1->inode != NULL);
    tassert(strncmp(f1->name, "f1", sizeof(f1->name)) == 0);

    tassert(file_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_file_creation_set_the_right_file_operations) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(f1->inode != NULL);
    tassert(f1->inode->fops.open == dummy_fop.open);
    tassert(f1->inode->fops.read == dummy_fop.read);
    tassert(f1->inode->fops.write == dummy_fop.write);

    fs_dentry_t *f2 = pseudofs_create_file(fsm->root, "f2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, NULL);
    tassert(f2 != NULL);
    tassert(f2->inode != NULL);
    tassert(f2->inode->fops.open == NULL);
    tassert(f2->inode->fops.read == NULL);
    tassert(f2->inode->fops.write == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
    tassert(!file_exist("/test/f2"));
TEND

T(pseudofs_file_cannot_create_duplicate_files) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    fs_dentry_t *f2 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f2 == NULL);
    tassert(file_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_file_removal_deletes_inode_and_dentry) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    tassert(vfs_file_remove(fsm->root, "f1") >= 0);
    tassert(!file_exist("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_create_file_if_parent_is_not_a_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    fs_dentry_t *f2 = pseudofs_create_file(f1, "f2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f2 == NULL);
    tassert(!file_exist("/test/f2"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_file_cannot_remove_non_existent_file) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    tassert(!file_exist("/test/f1"));
    tassert(vfs_file_remove(fsm->root, "f1") < 0);
    tassert(!file_exist("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
TEND

T(pseudofs_cannot_open_file_with_null_open_fops) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_file_ops_t null_open_fops = {
        .open = NULL,
    };

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &null_open_fops);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ);
    tassert(f == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_open_file_for_writing_if_it_doesnt_have_write_perms) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(f == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_open_file_for_reading_if_it_doesnt_have_read_perms) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ);
    tassert(f == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_open_non_existent_file) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_file_t *f = vfs_file_open("/test/hola", FS_ACCESS_MODE_READ);
    tassert(f == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_read_file_if_it_is_already_closed) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[10];
    tassert(vfs_file_read(f, buf, sizeof(buf), 0) >= 0);

    vfs_file_close(f);
    tassert(!f->opened);

    tassert(vfs_file_read(f, buf, sizeof(buf), 0) < 0);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_read_file_with_null_read_fops) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_file_ops_t null_fops = {
        .open = file_dummy_open,
        .read = NULL,
    };

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &null_fops);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[10];
    tassert(vfs_file_read(f, buf, sizeof(buf), 0) < 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_read_file_with_no_read_perms) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);
    char buf[10];
    tassert(vfs_file_read(f, buf, sizeof(buf), 0) < 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_read_file_if_it_was_opened_for_writing_only) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);
    char buf[10];
    tassert(vfs_file_read(f, buf, sizeof(buf), 0) < 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_read_file_reads_the_expected_data) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ);
    tassert(f != NULL);

    strncpy(rdata, "12345", sizeof(rdata));

    char buf[64];
    tassert(vfs_file_read(f, buf, sizeof(buf), 0) == 6);
    tassert(strncmp(buf, "12345", sizeof(buf)) == 0);

    tassert(vfs_file_read(f, buf, sizeof(buf), 2) == 4);
    tassert(strncmp(buf, "345", sizeof(buf)) == 0);

    tassert(vfs_file_read(f, buf, sizeof(buf), 4) == 2);
    tassert(strncmp(buf, "5", sizeof(buf)) == 0);

    tassert(vfs_file_read(f, buf, sizeof(buf), 6) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf), 7) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf), 100) == 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_read_bin_file_reads_the_expected_data) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    char bin[5] = { 0, 1, 2 };

    tassert(pseudofs_create_bin_file(fsm->root, "bin", FS_ACCESS_MODE_READ,
            bin, sizeof(bin)) != NULL);

    tassert(file_exist("/test/bin"));

    fs_file_t *f = vfs_file_open("/test/bin", FS_ACCESS_MODE_READ);
    tassert(f != NULL);

    char buf[64];
    tassert(vfs_file_read(f, buf, sizeof(buf), 0) == 5);
    tassert(buf[0] == 0);
    tassert(buf[1] == 1);
    tassert(buf[2] == 2);
    tassert(buf[3] == 0);
    tassert(buf[4] == 0);

    tassert(vfs_file_read(f, buf, sizeof(buf), 2) == 3);
    tassert(buf[0] == 2);
    tassert(buf[1] == 0);
    tassert(buf[2] == 0);

    tassert(vfs_file_read(f, buf, sizeof(buf), 5) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf), 7) == 0);
    tassert(vfs_file_read(f, buf, sizeof(buf), 100) == 0);

    vfs_file_close(f);

    tassert(vfs_file_open("/test/bin", FS_ACCESS_MODE_WRITE) == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/bin"));
TEND

T(pseudofs_cannot_write_file_if_it_is_already_closed) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);
    char buf[10] = { 0 };
    tassert(vfs_file_write(f, buf, sizeof(buf), 0) >= 0);

    vfs_file_close(f);
    tassert(!f->opened);

    tassert(vfs_file_write(f, buf, sizeof(buf), 0) < 0);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_write_file_with_no_write_perms) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[10] = { 0 };
    tassert(vfs_file_write(f, buf, sizeof(buf), 0) < 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_cannot_write_file_with_null_write_fops) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_file_ops_t null_fops = {
        .open = file_dummy_open,
        .write = NULL,
    };

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &null_fops);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);
    char buf[10] = { 0 };
    tassert(vfs_file_write(f, buf, sizeof(buf), 0) < 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_write_file_writes_the_expected_data) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);

    char buf[64];
    strncpy(buf, "12345", sizeof(buf));

    tassert(vfs_file_write(f, buf, strlen(buf) + 1, 0) == 6);
    tassert(strncmp(wdata, "12345", sizeof(wdata)) == 0);

    tassert(vfs_file_write(f, buf, strlen(buf) + 1, 2) == 6);
    tassert(strncmp(wdata, "1212345", sizeof(wdata)) == 0);

    tassert(vfs_file_write(f, buf, strlen(buf) + 1, 4) == 6);
    tassert(strncmp(wdata, "121212345", sizeof(wdata)) == 0);

    tassert(vfs_file_write(f, buf, strlen(buf) + 1, sizeof(wdata) - 1) == 1);
    tassert(wdata[sizeof(wdata) - 1] == '1');

    tassert(vfs_file_write(f, buf, strlen(buf) + 1, sizeof(wdata)) == 0);
    tassert(vfs_file_write(f, buf, strlen(buf) + 1, sizeof(wdata) + 1) == 0);
    tassert(vfs_file_write(f, buf, strlen(buf) + 1, sizeof(wdata) + 100) == 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_write_bin_file_reads_the_expected_data) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    char bin[5] = { 0 };

    tassert(pseudofs_create_bin_file(fsm->root, "bin", FS_ACCESS_MODE_WRITE,
            bin, sizeof(bin)) != NULL);

    tassert(file_exist("/test/bin"));

    fs_file_t *f = vfs_file_open("/test/bin", FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);

    char buf[64] = { 0, 1, 2, 3, 4};
    tassert(vfs_file_write(f, buf, sizeof(buf), 0) == 5);
    tassert(bin[0] == 0);
    tassert(bin[1] == 1);
    tassert(bin[2] == 2);
    tassert(bin[3] == 3);
    tassert(bin[4] == 4);

    tassert(vfs_file_write(f, buf, sizeof(buf), 2) == 3);
    tassert(bin[0] == 0);
    tassert(bin[1] == 1);
    tassert(bin[2] == 0);
    tassert(bin[3] == 1);
    tassert(bin[4] == 2);

    tassert(vfs_file_write(f, buf, sizeof(buf), 5) == 0);
    tassert(vfs_file_write(f, buf, sizeof(buf), 7) == 0);
    tassert(vfs_file_write(f, buf, sizeof(buf), 100) == 0);

    vfs_file_close(f);

    tassert(vfs_file_open("/test/bin", FS_ACCESS_MODE_READ) == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/bin"));
TEND

T(pseudofs_listdir_only_works_on_directories) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/f1"));

    fs_file_t *f = vfs_file_open("/test/f1", FS_ACCESS_MODE_READ);
    tassert(f != NULL);

    fs_listdir_t dirs[5] = { 0 };
    tassert(vfs_dir_listdir(f, 0, dirs, ARRAYSIZE(dirs)) < 0);

    vfs_file_close(f);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_listdir_returns_0_if_no_children) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_file_t *d = vfs_file_open("/test/dir1", FS_ACCESS_MODE_READ);
    tassert(d != NULL);

    fs_listdir_t dirs[5] = { 0 };
    tassert(vfs_dir_listdir(d, 0, dirs, ARRAYSIZE(dirs)) == 0);

    vfs_file_close(d);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
TEND

T(pseudofs_listdir_returns_the_list_of_dirs_and_files) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *f1 = pseudofs_create_file(dir1, "f1", FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/dir1/f1"));

    fs_dentry_t *f2 = pseudofs_create_file(dir1, "f2", FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f2 != NULL);
    tassert(file_exist("/test/dir1/f2"));

    fs_dentry_t *dir2 = vfs_dir_create(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir2 != NULL);
    tassert(file_is_dir("/test/dir1/dir2"));


    fs_file_t *d = vfs_file_open("/test/dir1", FS_ACCESS_MODE_READ);
    tassert(d != NULL);

    fs_listdir_t dirs[5] = { 0 };
    tassert(vfs_dir_listdir(d, 0, dirs, ARRAYSIZE(dirs)) == 3);
    tassert(strncmp(dirs[0].name, "f1", sizeof(dirs[0].name)) == 0);
    tassert(!dirs[0].isdir);
    tassert(strncmp(dirs[1].name, "f2", sizeof(dirs[1].name)) == 0);
    tassert(!dirs[1].isdir);
    tassert(strncmp(dirs[2].name, "dir2", sizeof(dirs[2].name)) == 0);
    tassert(dirs[2].isdir);

    tassert(vfs_dir_listdir(d, 1, dirs, ARRAYSIZE(dirs)) == 2);
    tassert(strncmp(dirs[0].name, "f2", sizeof(dirs[1].name)) == 0);
    tassert(!dirs[0].isdir);
    tassert(strncmp(dirs[1].name, "dir2", sizeof(dirs[2].name)) == 0);
    tassert(dirs[1].isdir);

    tassert(vfs_dir_listdir(d, 2, dirs, ARRAYSIZE(dirs)) == 1);
    tassert(strncmp(dirs[0].name, "dir2", sizeof(dirs[2].name)) == 0);
    tassert(dirs[0].isdir);

    tassert(vfs_dir_listdir(d, 0, dirs, 1) == 1);
    tassert(strncmp(dirs[0].name, "f1", sizeof(dirs[2].name)) == 0);
    tassert(!dirs[0].isdir);

    tassert(vfs_dir_listdir(d, 3, dirs, ARRAYSIZE(dirs)) == 0);
    tassert(vfs_dir_listdir(d, 100, dirs, ARRAYSIZE(dirs)) == 0);

    vfs_file_close(d);


    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
    tassert(!file_exist("/test/dir1/f1"));
    tassert(!file_exist("/test/dir1/f2"));
    tassert(!file_exist("/test/dir1/dir2"));
TEND
