#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>
#include <utils/file.h>

static int file_dummy_open(fs_inode_t *inode, fs_file_t *f) {
    return -1;
}

static int file_dummy_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    return -1;
}

static int file_dummy_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    return -1;
}

static fs_file_ops_t dummy_fop = {
    .open = file_dummy_open,
    .read = file_dummy_read,
    .write = file_dummy_write,
};

T(pseudofs_fstype_is_supported_by_default) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));
    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_mount_populates_superblock) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));
    tassert(fsm->sb != NULL);
    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_dir_creation_instantiates_inode_and_dentry) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_create_dir(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(file_is_dir("/test/dir1/dir2"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
TEND

T(pseudofs_cannot_remove_non_existent_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    tassert(vfs_remove_dir(fsm->root, "hola") < 0);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_dir_removal_deletes_inodes_and_dentries) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_create_dir(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(file_is_dir("/test/dir1/dir2"));

    tassert(vfs_remove_dir(fsm->root, "dir1") >= 0);
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
    tassert(!vfs_dentry_exist("/test/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
TEND

T(pseudofs_dir_removal_deletes_inodes_and_dentries2) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_create_dir(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(file_is_dir("/test/dir1/dir2"));

    tassert(vfs_remove_dir(dir1, "dir2") >= 0);
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
    tassert(vfs_dentry_exist("/test/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
TEND

T(pseudofs_cannot_create_dir_if_file_or_dir_already_existing) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    tassert(vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE) == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
TEND

T(pseudofs_cannot_create_dir_if_parent_is_not_a_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(vfs_dentry_exist("/test/f1"));

    fs_dentry_t *dir1 = vfs_create_dir(f1, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 == NULL);
    tassert(!vfs_dentry_exist("/test/f1/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/f1"));
    tassert(!vfs_dentry_exist("/test/f1/dir1"));
TEND

T(pseudofs_file_creation_instantiates_dentry_and_inode) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(f1->inode != NULL);
    tassert(strncmp(f1->name, "f1", sizeof(f1->name)) == 0);

    tassert(vfs_dentry_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/f1"));
TEND

T(pseudofs_file_creation_set_the_right_file_operations) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

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
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/f1"));
    tassert(!vfs_dentry_exist("/test/f2"));
TEND

T(pseudofs_file_cannot_create_duplicate_files) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(vfs_dentry_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    fs_dentry_t *f2 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f2 == NULL);
    tassert(vfs_dentry_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/f1"));
TEND

T(pseudofs_file_removal_deletes_inode_and_dentry) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(vfs_dentry_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    tassert(vfs_remove_file(fsm->root, "f1") >= 0);
    tassert(!vfs_dentry_exist("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/f1"));
TEND

T(pseudofs_cannot_create_file_if_parent_is_not_a_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 != NULL);
    tassert(vfs_dentry_exist("/test/f1"));
    tassert(!file_is_dir("/test/f1"));

    fs_dentry_t *f2 = pseudofs_create_file(f1, "f2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f2 == NULL);
    tassert(!vfs_dentry_exist("/test/f2"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/f1"));
TEND

T(pseudofs_file_cannot_remove_non_existent_file) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    tassert(!vfs_dentry_exist("/test/f1"));
    tassert(vfs_remove_file(fsm->root, "f1") < 0);
    tassert(!vfs_dentry_exist("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
TEND
