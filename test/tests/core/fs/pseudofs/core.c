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

T(pseudofs_listdir_returns_double_dot_dir_if_no_children) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_file_t *d = vfs_file_open("/test/dir1", FS_ACCESS_MODE_READ);
    tassert(d != NULL);

    fs_listdir_t dirs[5] = { 0 };
    tassert(vfs_dir_listdir(d, 0, dirs, ARRAYSIZE(dirs)) == 1);
    tassert(strncmp(dirs[0].name, "..", sizeof(dirs[0].name)) == 0);
    tassert(dirs[0].isdir);

    vfs_file_close(d);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
TEND

T(pseudofs_listdir_returns_the_list_of_dirs_and_files) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
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
    tassert(vfs_dir_listdir(d, 0, dirs, ARRAYSIZE(dirs)) == 4);
    tassert(strncmp(dirs[0].name, "..", sizeof(dirs[0].name)) == 0);
    tassert(dirs[0].isdir);
    tassert(strncmp(dirs[1].name, "f1", sizeof(dirs[0].name)) == 0);
    tassert(!dirs[1].isdir);
    tassert(strncmp(dirs[2].name, "f2", sizeof(dirs[1].name)) == 0);
    tassert(!dirs[2].isdir);
    tassert(strncmp(dirs[3].name, "dir2", sizeof(dirs[2].name)) == 0);
    tassert(dirs[3].isdir);

    tassert(vfs_dir_listdir(d, 1, dirs, ARRAYSIZE(dirs)) == 3);
    tassert(strncmp(dirs[0].name, "f1", sizeof(dirs[0].name)) == 0);
    tassert(!dirs[0].isdir);
    tassert(strncmp(dirs[1].name, "f2", sizeof(dirs[1].name)) == 0);
    tassert(!dirs[1].isdir);
    tassert(strncmp(dirs[2].name, "dir2", sizeof(dirs[2].name)) == 0);
    tassert(dirs[2].isdir);

    tassert(vfs_dir_listdir(d, 2, dirs, ARRAYSIZE(dirs)) == 2);
    tassert(strncmp(dirs[0].name, "f2", sizeof(dirs[1].name)) == 0);
    tassert(!dirs[0].isdir);
    tassert(strncmp(dirs[1].name, "dir2", sizeof(dirs[2].name)) == 0);
    tassert(dirs[1].isdir);

    tassert(vfs_dir_listdir(d, 3, dirs, ARRAYSIZE(dirs)) == 1);
    tassert(strncmp(dirs[0].name, "dir2", sizeof(dirs[2].name)) == 0);
    tassert(dirs[0].isdir);

    tassert(vfs_dir_listdir(d, 0, dirs, 1) == 1);
    tassert(strncmp(dirs[0].name, "..", sizeof(dirs[2].name)) == 0);
    tassert(dirs[0].isdir);

    tassert(vfs_dir_listdir(d, 4, dirs, ARRAYSIZE(dirs)) == 0);
    tassert(vfs_dir_listdir(d, 100, dirs, ARRAYSIZE(dirs)) == 0);

    vfs_file_close(d);


    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
    tassert(!file_exist("/test/dir1/f1"));
    tassert(!file_exist("/test/dir1/f2"));
    tassert(!file_exist("/test/dir1/dir2"));
TEND

T(pseudofs_listdir_root_returns_all_its_children) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_file_t *d = vfs_file_open("/", FS_ACCESS_MODE_READ);
    tassert(d != NULL);

    fs_listdir_t dirs[10] = { 0 };
    int ndirs = vfs_dir_listdir(d, 0, dirs, ARRAYSIZE(dirs));
    int i;
    for (i = 0; i < ndirs; i++) {
        if (strncmp(dirs[i].name, "test", sizeof(dirs[i].name)) == 0) {
            break;
        }
    }
    tassert(i < ndirs);

    vfs_file_close(d);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
TEND

T(pseudofs_creating_dir_fails_on_readonly_mount) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    tassert(dir1 == NULL);
    tassert(!file_exist("/test/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
TEND

T(pseudofs_creating_file_fails_on_readonly_mount) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *f1 = pseudofs_create_file(fsm->root, "f1", FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 == NULL);
    tassert(!file_exist("/test/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/f1"));
TEND

T(pseudofs_creating_dir_fails_on_readonly_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir1 != NULL);
    tassert(file_exist("/test/dir1"));

    fs_dentry_t *dir2 = vfs_dir_create(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir2 == NULL);
    tassert(!file_exist("/test/dir1/dir2"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
TEND

T(pseudofs_creating_file_fails_on_readonly_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_EXEC);
    tassert(dir1 != NULL);
    tassert(file_exist("/test/dir1"));

    fs_dentry_t *f1 = pseudofs_create_file(dir1, "f1", FS_ACCESS_MODE_WRITE, &dummy_fop);
    tassert(f1 == NULL);
    tassert(!file_exist("/test/dir1/f1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
    tassert(!file_exist("/test/dir1/f1"));
TEND

T(pseudofs_listdir_fails_on_non_readable_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_EXEC);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_file_t *d = vfs_file_open("/test/dir1", FS_ACCESS_MODE_READ);
    tassert(d == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
TEND

static int openfiles(void *data) {
    vfs_file_open("/test/dir1/f1", FS_ACCESS_MODE_READ);
    vfs_file_open("/test/dir1/f1", FS_ACCESS_MODE_READ);
    vfs_file_open("/test/dir1/f1", FS_ACCESS_MODE_READ);
    vfs_file_open("/test/dir1/f1", FS_ACCESS_MODE_READ);
    vfs_file_open("/test/dir1/f1", FS_ACCESS_MODE_READ);
    sleep(1);
    return 0;
}

T(pseudofs_unclosed_files_are_closed_on_process_death) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(file_exist("/test"));

    fs_dentry_t *dir1 = vfs_dir_create(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC);
    tassert(dir1 != NULL);
    tassert(file_is_dir("/test/dir1"));

    fs_dentry_t *f1 = pseudofs_create_file(dir1, "f1", FS_ACCESS_MODE_READ, &dummy_fop);
    tassert(f1 != NULL);
    tassert(file_exist("/test/dir1/f1"));

    pcb_t *p = process_spawn_kernel_process("files1", openfiles, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    tassert(p != NULL);

    pcb_t *p2 = process_spawn_kernel_process("files2", openfiles, NULL,
                        8196,  process_get_current()->sched.priority - 1);
    tassert(p2 != NULL);

    process_wait_for(p, NULL);
    process_wait_for(p2, NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!file_exist("/test"));
    tassert(!file_exist("/test/dir1"));
    tassert(!file_exist("/test/dir1/f1"));
TEND
