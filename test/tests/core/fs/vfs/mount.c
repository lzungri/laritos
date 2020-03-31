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
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>

static fs_superblock_t dummy_sb = {
    .ops = {
        .alloc_inode = vfs_inode_def_alloc,
        .free_inode = vfs_inode_def_free,
    },
};

static int dummy_mount(fs_type_t *fstype, fs_mount_t *fsm, fs_param_t *params) {
    dummy_sb.fstype = fstype;
    fsm->sb = &dummy_sb;
    fsm->sb->root = fsm->sb->ops.alloc_inode(fsm->sb);
    return 0;
}

int mount_error(fs_type_t *fstype, fs_mount_t *fsm, fs_param_t *params) {
    return -1;
}

T(vfs_mount_make_sure_root_filesystem_is_mounted) {
    tassert(file_is_dir("/"));
    tassert(vfs_dentry_lookup("/") == _laritos.fs.root);
TEND

T(vfs_mount_make_sure_procfs_filesystem_is_mounted) {
    tassert(file_is_dir("/proc"));
    tassert(vfs_dentry_lookup("/proc") == _laritos.fs.proc_root);
TEND

T(vfs_mount_fails_on_unsupported_fs_type) {
    tassert(vfs_mount_fs("unsopported", "/xxx", 0, NULL) == NULL);
    tassert(!file_exist("/xxx"));
TEND

T(vfs_mount_fails_on_mount_error) {
    fs_type_t fst = {
        .id = "testfs",
        .mount = mount_error,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    tassert(vfs_mount_fs("testfs", "/xxx", 0, NULL) == NULL);
    tassert(!file_exist("/xxx"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_unmount_fs_fails_on_non_mounted_fs) {
    tassert(vfs_unmount_fs("unmounted") < 0);
TEND

T(vfs_mount_adds_a_new_fs_under_mount_point) {
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
    tassert(file_exist("/dummymnt"));

    vfs_unmount_fs("/dummymnt");
    tassert(!file_exist("/dummymnt"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

int nosb_mount(fs_type_t *fstype, fs_mount_t *fsm, fs_param_t *params) {
    fsm->sb = NULL;
    return 0;
}

T(vfs_mount_fails_when_no_superblock_is_instantiated) {
    fs_type_t fst = {
        .id = "testnosb",
        .mount = nosb_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    tassert(vfs_mount_fs("testnosb", "/testnosb", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL) == NULL);
    tassert(!file_exist("/testnosb"));

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
    tassert(file_exist("/mnt_used"));
    tassert(vfs_mount_fs("mnt_used", "/mnt_used", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL) == NULL);
    tassert(file_exist("/mnt_used"));

    vfs_unmount_fs("/mnt_used");
    tassert(!file_exist("/mnt_used"));

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_mount_fails_if_mount_point_one_or_more_dirs_are_missing) {
    fs_type_t fst = {
        .id = "nodirs",
        .mount = dummy_mount,
    };
    vfs_register_fs_type(&fst);
    tassert(vfs_is_fs_type_supported(fst.id));

    tassert(vfs_mount_fs("nodirs", "/abc/mpoint", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL) == NULL);

    vfs_unregister_fs_type(&fst);
    tassert(!vfs_is_fs_type_supported(fst.id));
TEND

T(vfs_cannot_unmount_inexistent_mount_point) {
    tassert(vfs_unmount_fs("/non_existent_mount_point") < 0);
TEND
