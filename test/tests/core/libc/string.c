#include <log.h>

#include <test/test.h>

// strstr()
DEF_TEST(strstr_returns_null_when_no_match_is_found) {
    assert_null(strstr("abc", "cba", 4));
DEF_TEST_END

DEF_TEST(strstr_returns_null_when_no_match_is_found_on_empty_str) {
    assert_null(strstr("", "abc", 4));
DEF_TEST_END

DEF_TEST(strstr_returns_ptr_when_match_is_found) {
    char *p = "abc";
    assert_eq(strstr(p, "a", 4), p);
    assert_eq(strstr(p, "ab", 4), p);
    assert_eq(strstr(p, "abc", 4), p);
    assert_eq(strstr(p, "bc", 4), p + 1);
    assert_eq(strstr(p, "c", 4), p + 2);
DEF_TEST_END


// strchr()
DEF_TEST(strchr_returns_null_when_no_match_is_found_on_empty_str) {
    assert_null(strchr("", 'a'));
DEF_TEST_END

DEF_TEST(strchr_returns_null_when_no_match_is_found) {
    assert_null(strchr("abc", 'd'));
DEF_TEST_END

DEF_TEST(strchr_returns_ptr_when_match_is_found) {
    char *p = "abc";
    assert_eq(strchr(p, 'a'), p);
    assert_eq(strchr(p, 'b'), p + 1);
    assert_eq(strchr(p, 'c'), p + 2);
DEF_TEST_END


// strrchr()
DEF_TEST(strrchr_returns_null_when_no_match_is_found_on_empty_str) {
    assert_null(strrchr("", 'a'));
DEF_TEST_END

DEF_TEST(strrchr_returns_null_when_no_match_is_found) {
    assert_null(strrchr("abc", 'd'));
DEF_TEST_END

DEF_TEST(strrchr_returns_ptr_when_match_is_found) {
    char *p = "abc";
    assert_eq(strrchr(p, 'a'), p);
    assert_eq(strrchr(p, 'b'), p + 1);
    assert_eq(strrchr(p, 'c'), p + 2);
DEF_TEST_END

DEF_TEST(strrchr_returns_last_ptr_when_match_is_found) {
    char *p = "abcabc";
    assert_eq(strrchr(p, 'a'), p + 3);
    assert_eq(strrchr(p, 'b'), p + 3 + 1);
    assert_eq(strrchr(p, 'c'), p + 3 + 2);
DEF_TEST_END


// strncpy()
DEF_TEST(strncpy_doesnt_add_null_char_if_not_enough_space) {
    char buf[4] = { 0 };
    strncpy(buf, "abc", sizeof(buf) - 1);
    assert_str_equals(buf, "abc", sizeof(buf));
    strncpy(buf, "123", sizeof(buf) - 2);
    assert_str_equals(buf, "12c", sizeof(buf));
    strncpy(buf, "456", sizeof(buf) - 3);
    assert_str_equals(buf, "42c", sizeof(buf));
DEF_TEST_END

DEF_TEST(strncpy_doesnt_overflow) {
    char buf[4] = { 0 };
    strncpy(buf, "abcdef", sizeof(buf) - 1);
    assert_str_equals(buf, "abc", sizeof(buf));
DEF_TEST_END


// strlen()
DEF_TEST(strnlen_returns_valid_len) {
    assert_eq(strnlen("abc", 100), 3);
    assert_eq(strnlen("abc", 5), 3);
    assert_eq(strnlen("abc", 4), 3);
DEF_TEST_END

DEF_TEST(strnlen_doesnt_overflow) {
    assert_eq(strnlen("abc", 3), 3);
    assert_eq(strnlen("abc", 2), 2);
    assert_eq(strnlen("abc", 1), 1);
    assert_eq(strnlen("abc", 0), 0);
DEF_TEST_END


// strncmp
DEF_TEST(strncmp_returns_0_for_equality) {
    assert_eq(strncmp("abc", "abc", 100), 0);
    assert_eq(strncmp("abc", "abc", 5), 0);
    assert_eq(strncmp("abc", "abc", 4), 0);
    assert_eq(strncmp("abc", "abc", 3), 0);
    assert_eq(strncmp("abc", "abc", 2), 0);
    assert_eq(strncmp("abc", "abc", 1), 0);
    assert_eq(strncmp("abc", "abc", 0), 0);
DEF_TEST_END

DEF_TEST(strncmp_returns_lt0_for_greater_right_str_operand) {
    assert_lt(strncmp("abc", "bbc", 100), 0);
    assert_lt(strncmp("abc", "bbc", 5), 0);
    assert_lt(strncmp("abc", "bbc", 4), 0);
    assert_lt(strncmp("abc", "bbc", 3), 0);
    assert_lt(strncmp("abc", "bbc", 2), 0);
    assert_lt(strncmp("abc", "bbc", 1), 0);
    assert_lt(strncmp("abc", "bbc", 0), 0);
DEF_TEST_END

DEF_TEST(strncmp_returns_gt0_for_greater_left_str_operand) {
    assert_gt(strncmp("bbc", "abc", 100), 0);
    assert_gt(strncmp("bbc", "abc", 5), 0);
    assert_gt(strncmp("bbc", "abc", 4), 0);
    assert_gt(strncmp("bbc", "abc", 3), 0);
    assert_gt(strncmp("bbc", "abc", 2), 0);
    assert_gt(strncmp("bbc", "abc", 1), 0);
    assert_gt(strncmp("bbc", "abc", 0), 0);
DEF_TEST_END

// memcmp
DEF_TEST(memcmp_returns_0_for_equality) {
    assert_eq(memcmp("abc", "abc", 4), 0);
    assert_eq(memcmp("abc", "abc", 3), 0);
    assert_eq(memcmp("abc", "abc", 2), 0);
    assert_eq(memcmp("abc", "abc", 1), 0);
    assert_eq(memcmp("abc", "abc", 0), 0);
DEF_TEST_END

DEF_TEST(memcmp_returns_lt0_for_greater_right_str_operand) {
    assert_lt(memcmp("abc", "bbc", 4), 0);
    assert_lt(memcmp("abc", "bbc", 3), 0);
    assert_lt(memcmp("abc", "bbc", 2), 0);
    assert_lt(memcmp("abc", "bbc", 1), 0);
    assert_lt(memcmp("abc", "bbc", 0), 0);
DEF_TEST_END

DEF_TEST(memcmp_returns_gt0_for_greater_left_str_operand) {
    assert_gt(memcmp("bbc", "abc", 4), 0);
    assert_gt(memcmp("bbc", "abc", 3), 0);
    assert_gt(memcmp("bbc", "abc", 2), 0);
    assert_gt(memcmp("bbc", "abc", 1), 0);
    assert_gt(memcmp("bbc", "abc", 0), 0);
DEF_TEST_END

// memset
DEF_TEST(memset_doesnt_overflow) {
    char buf[4] = { 0 };
    memset(buf, 'a', 0);
    memset(buf, 'b', sizeof(buf) - 3);
    assert_str_equals(buf, "b", sizeof(buf));
    memset(buf, 'c', sizeof(buf) - 2);
    assert_str_equals(buf, "cc", sizeof(buf));
    memset(buf, 'd', sizeof(buf) - 1);
    assert_str_equals(buf, "ddd", sizeof(buf));
DEF_TEST_END

// memcpy
DEF_TEST(memcpy_doesnt_overflow) {
    char from[] = { 'a', 'b', 'c'};
    char to[4] = { 0 };
    memcpy(to, from, sizeof(to) - 3);
    assert_str_equals(to, "a", sizeof(to));
    memcpy(to, from, sizeof(to) - 2);
    assert_str_equals(to, "ab", sizeof(to));
    memcpy(to, from, sizeof(to) - 1);
    assert_str_equals(to, "abc", sizeof(to));
DEF_TEST_END
