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

static int empty_mount(fs_type_t *fstype, fs_mount_t *fsm, fs_param_t *params) {
    return 0;
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
