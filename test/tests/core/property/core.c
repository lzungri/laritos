#include <log.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <core.h>
#include <process/core.h>
#include <property/core.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/file.h>
#include <test/test.h>

T(prop_cannot_create_multiple_props_with_same_id) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) < 0);
    tassert(property_remove("test") >= 0);
TEND

T(prop_remove_fails_on_non_existant_property) {
    tassert(property_remove("test") < 0);
TEND

T(prop_set_assigns_the_right_value_to_the_prop) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_get("test", prop) < 0);
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_set("test", "hello") >= 0);
    tassert(property_get("test", prop) >= 0);
    tassert(strncmp(prop, "hello", sizeof(prop)) == 0);
    tassert(property_remove("test") >= 0);
TEND

T(prop_get_or_default_returns_def_if_prop_is_null_or_non_existent) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    property_get_or_def("test", prop, "default");
    tassert(strncmp(prop, "default", sizeof(prop)) == 0);

    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_set("test", "hello") >= 0);

    property_get_or_def("test", prop, "default2");
    tassert(strncmp(prop, "hello", sizeof(prop)) == 0);

    tassert(property_remove("test") >= 0);
TEND

T(prop_get_returns_prop_initial_value) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_create("test", "HELLO", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    property_get("test", prop);
    tassert(strncmp(prop, "HELLO", sizeof(prop)) == 0);
    tassert(property_remove("test") >= 0);
TEND

T(prop_get_fails_on_non_existent_prop) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_get("doesntexist", prop) < 0);
TEND

T(prop_get_int32_fails_on_non_existent_prop) {
    int32_t prop;
    tassert(property_get_int32("doesntexist", &prop) < 0);
TEND

T(prop_get_as_int32_returns_the_str_value_as_int32_t) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);

    int32_t prop;

    tassert(property_set("test", "123") >= 0);
    tassert(property_get_int32("test", &prop) >= 0);
    tassert(prop == 123);
    tassert(property_set("test", "-456") >= 0);
    tassert(property_get_int32("test", &prop) >= 0);
    tassert(prop == -456);
    tassert(property_set("test", "0") >= 0);
    tassert(property_get_int32("test", &prop) >= 0);
    tassert(prop == 0);
    tassert(property_set("test", "") >= 0);
    tassert(property_get_int32("test", &prop) >= 0);
    tassert(prop == 0);

    tassert(property_remove("test") >= 0);
TEND

T(prop_get_or_def_int32_returns_def_if_prop_is_null_or_non_existent) {
    tassert(property_get_or_def_int32("test", 123) == 123);

    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);

    tassert(property_get_or_def_int32("test", 123) == 0);

    tassert(property_set("test", "456") >= 0);
    tassert(property_get_or_def_int32("test", 123) == 456);

    tassert(property_remove("test") >= 0);
TEND

static int prop_read(void *data) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    return property_get("test", prop);
}

static int prop_write(void *data) {
    return property_set("test", "from child process");
}

T(prop_get_fails_if_read_comes_from_a_non_auth_process) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);
    tassert(property_set("test", "hello") >= 0);

    pcb_t *p0 = process_spawn_kernel_process("prop0", prop_read, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);

    int prop_result;
    process_wait_for(p0, &prop_result);
    tassert(prop_result < 0);

    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_get("test", prop) >= 0);
    tassert(strncmp(prop, "hello", sizeof(prop)) == 0);
    tassert(property_remove("test") >= 0);
TEND

T(prop_set_fails_if_write_comes_from_a_non_auth_process) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);
    tassert(property_set("test", "hello") >= 0);

    pcb_t *p0 = process_spawn_kernel_process("prop0", prop_write, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);

    int prop_result;
    process_wait_for(p0, &prop_result);
    tassert(prop_result < 0);

    tassert(property_remove("test") >= 0);
TEND

T(prop_read_succeeds_if_read_permission_for_all_is_enabled) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);
    tassert(property_set("test", "hello") >= 0);

    pcb_t *p0 = process_spawn_kernel_process("prop0", prop_read, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);

    int prop_result;
    process_wait_for(p0, &prop_result);
    tassert(prop_result >= 0);

    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_get("test", prop) >= 0);
    tassert(strncmp(prop, "hello", sizeof(prop)) == 0);
    tassert(property_remove("test") >= 0);
TEND

T(prop_set_succeeds_if_write_permission_for_all_is_enabled) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_set("test", "hello") >= 0);

    pcb_t *p0 = process_spawn_kernel_process("prop0", prop_write, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);

    int prop_result;
    process_wait_for(p0, &prop_result);
    tassert(prop_result >= 0);

    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_get("test", prop) >= 0);
    tassert(strncmp(prop, "from child process", sizeof(prop)) == 0);
    tassert(property_remove("test") >= 0);
TEND

static int prop_remove(void *data) {
    return property_remove("test");
}

T(prop_remove_fails_if_deletion_comes_from_a_non_auth_process) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);

    pcb_t *p0 = process_spawn_kernel_process("prop0", prop_remove, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);

    int prop_result;
    process_wait_for(p0, &prop_result);
    tassert(prop_result < 0);

    tassert(property_remove("test") >= 0);
TEND

T(prop_create_adds_a_new_entry_in_the_kernelfs) {
    tassert(!file_exist("/property/test"));
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(file_exist("/property/test"));
    tassert(property_remove("test") >= 0);
    tassert(!file_exist("/property/test"));
TEND

T(prop_set_fails_on_read_only_props) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL) >= 0);
    tassert(property_set("test", "fail") < 0);
    tassert(property_remove("test") >= 0);
TEND

T(prop_read_from_sysfs_returns_the_right_data) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_set("test", "data") >= 0);

    fs_file_t *f = vfs_file_open("/property/test", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[32] = { 0 };
    tassert(vfs_file_read(f, buf, sizeof(buf) - 1, 0) >= 0);
    tassert(strncmp(buf, "data", sizeof(buf)) == 0);
    vfs_file_close(f);

    tassert(property_remove("test") >= 0);
TEND

T(prop_write_to_sysfs_updates_the_property_accordingly) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_set("test", "data") >= 0);

    fs_file_t *f = vfs_file_open("/property/test", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(f != NULL);
    char buf[32] = "from sysfs";
    tassert(vfs_file_write(f, buf, sizeof(buf), 0) >= 0);
    vfs_file_close(f);

    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_get("test", prop) >= 0);
    tassert(strncmp(prop, "from sysfs", sizeof(prop)) == 0);

    tassert(property_remove("test") >= 0);
TEND

T(prop_write_to_sysfs_fails_on_readonly_props) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL) >= 0);

    fs_file_t *f = vfs_file_open("/property/test", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(f == NULL);

    tassert(property_remove("test") >= 0);
TEND


static int prop_sysfs_read(void *data) {
    fs_file_t *f = vfs_file_open("/property/test", FS_ACCESS_MODE_READ);
    if (f == NULL) {
        return -1;
    }
    char buf[32] = { 0 };
    if (vfs_file_read(f, buf, sizeof(buf) - 1, 0) < 0) {
        vfs_file_close(f);
        return -1;
    }
    vfs_file_close(f);
    return 0;
}

T(prop_read_from_sysfs_fails_if_read_comes_from_a_non_auth_process) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_ALL) >= 0);

    pcb_t *p0 = process_spawn_kernel_process("prop0", prop_sysfs_read, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);

    int read_status;
    process_wait_for(p0, &read_status);
    tassert(read_status < 0);

    tassert(property_remove("test") >= 0);
TEND

T(prop_write_to_sysfs_fails_on_readonly_props2) {
    tassert(property_create("test", "", PROPERTY_MODE_READ_BY_ALL) >= 0);

    fs_file_t *f = vfs_file_open("/property/test", FS_ACCESS_MODE_READ);
    tassert(f != NULL);
    char buf[32];
    tassert(vfs_file_write(f, buf, sizeof(buf), 0) < 0);
    vfs_file_close(f);

    tassert(property_remove("test") >= 0);
TEND
