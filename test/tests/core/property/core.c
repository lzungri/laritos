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
    tassert(property_create("test", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_create("test", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) < 0);
    tassert(property_remove("test") >= 0);
TEND

T(prop_remove_fails_on_non_existant_property) {
    tassert(property_remove("test") < 0);
TEND

T(prop_set_assigns_the_right_value_to_the_prop) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    tassert(property_get("test", prop) < 0);
    tassert(property_create("test", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(property_set("test", "hello") >= 0);
    tassert(property_get("test", prop) >= 0);
    tassert(strncmp(prop, "hello", sizeof(prop)) == 0);
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
    tassert(property_create("test", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);

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

static int prop_read(void *data) {
    char prop[PROPERTY_VALUE_MAX_LEN];
    return property_get("test", prop);
}

static int prop_write(void *data) {
    return property_set("test", "from child process");
}

T(prop_get_fails_if_read_comes_from_a_non_auth_process) {
    tassert(property_create("test", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);
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
    tassert(property_create("test", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);
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
    tassert(property_create("test", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);
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
    tassert(property_create("test", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
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
    tassert(property_create("test", PROPERTY_MODE_READ_BY_OWNER | PROPERTY_MODE_WRITE_BY_OWNER) >= 0);

    pcb_t *p0 = process_spawn_kernel_process("prop0", prop_remove, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p0 != NULL);

    int prop_result;
    process_wait_for(p0, &prop_result);
    tassert(prop_result < 0);

    tassert(property_remove("test") >= 0);
TEND

T(prop_create_adds_a_new_entry_in_the_kernelfs) {
    tassert(!file_exist("/kernel/prop/test"));
    tassert(property_create("test", PROPERTY_MODE_READ_BY_ALL | PROPERTY_MODE_WRITE_BY_ALL) >= 0);
    tassert(file_exist("/kernel/prop/test"));
    tassert(property_remove("test") >= 0);
    tassert(!file_exist("/kernel/prop/test"));
TEND
