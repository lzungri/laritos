#include <log.h>

#include <test/test.h>

// strstr()
DEF_TEST(strstr_returns_null_when_no_match_is_found) {
    tassert(strstr("abc", "cba", 4) == NULL);
DEF_TEST_END

DEF_TEST(strstr_returns_null_when_no_match_is_found_on_empty_str) {
    tassert(strstr("", "abc", 4) == NULL);
DEF_TEST_END

DEF_TEST(strstr_returns_ptr_when_match_is_found) {
    char *p = "abc";
    tassert(strstr(p, "a", 4) == p);
    tassert(strstr(p, "ab", 4) == p);
    tassert(strstr(p, "abc", 4) == p);
    tassert(strstr(p, "bc", 4) == p + 1);
    tassert(strstr(p, "c", 4) == p + 2);
DEF_TEST_END


// strchr()
DEF_TEST(strchr_returns_null_when_no_match_is_found_on_empty_str) {
    tassert(strchr("", 'a') == NULL);
DEF_TEST_END

DEF_TEST(strchr_returns_null_when_no_match_is_found) {
    tassert(strchr("abc", 'd') == NULL);
DEF_TEST_END

DEF_TEST(strchr_returns_ptr_when_match_is_found) {
    char *p = "abc";
    tassert(strchr(p, 'a') == p);
    tassert(strchr(p, 'b') == p + 1);
    tassert(strchr(p, 'c') == p + 2);
DEF_TEST_END


// strrchr()
DEF_TEST(strrchr_returns_null_when_no_match_is_found_on_empty_str) {
    tassert(strrchr("", 'a') == NULL);
DEF_TEST_END

DEF_TEST(strrchr_returns_null_when_no_match_is_found) {
    tassert(strrchr("abc", 'd') == NULL);
DEF_TEST_END

DEF_TEST(strrchr_returns_ptr_when_match_is_found) {
    char *p = "abc";
    tassert(strrchr(p, 'a') == p);
    tassert(strrchr(p, 'b') == p + 1);
    tassert(strrchr(p, 'c') == p + 2);
DEF_TEST_END

DEF_TEST(strrchr_returns_last_ptr_when_match_is_found) {
    char *p = "abcabc";
    tassert(strrchr(p, 'a') == p + 3);
    tassert(strrchr(p, 'b') == p + 3 + 1);
    tassert(strrchr(p, 'c') == p + 3 + 2);
DEF_TEST_END


// strncpy()
DEF_TEST(strncpy_doesnt_add_null_char_if_not_enough_space) {
    char buf[4] = { 0 };
    strncpy(buf, "abc", sizeof(buf) - 1);
    tassert(strncmp(buf, "abc", sizeof(buf)) == 0);
    strncpy(buf, "123", sizeof(buf) - 2);
    tassert(strncmp(buf, "12c", sizeof(buf)) == 0);
    strncpy(buf, "456", sizeof(buf) - 3);
    tassert(strncmp(buf, "42c", sizeof(buf)) == 0);
DEF_TEST_END

DEF_TEST(strncpy_doesnt_overflow) {
    char buf[4] = { 0 };
    strncpy(buf, "abcdef", sizeof(buf) - 1);
    tassert(strncmp(buf, "abc", sizeof(buf)) == 0);
DEF_TEST_END


// strlen()
DEF_TEST(strnlen_returns_valid_len) {
    tassert(strnlen("abc", 100) == 3);
    tassert(strnlen("abc", 5) == 3);
    tassert(strnlen("abc", 4) == 3);
DEF_TEST_END

DEF_TEST(strnlen_doesnt_overflow) {
    tassert(strnlen("abc", 3) == 3);
    tassert(strnlen("abc", 2) == 2);
    tassert(strnlen("abc", 1) == 1);
    tassert(strnlen("abc", 0) == 0);
DEF_TEST_END


// strncmp
DEF_TEST(strncmp_returns_0_for_equality) {
    tassert(strncmp("abc", "abc", 100) == 0);
    tassert(strncmp("abc", "abc", 5) == 0);
    tassert(strncmp("abc", "abc", 4) == 0);
    tassert(strncmp("abc", "abc", 3) == 0);
    tassert(strncmp("abc", "abc", 2) == 0);
    tassert(strncmp("abc", "abc", 1) == 0);
    tassert(strncmp("abc", "abc", 0) == 0);
DEF_TEST_END

DEF_TEST(strncmp_returns_lt0_for_greater_right_str_operand) {
    tassert(strncmp("abc", "bbc", 100) < 0);
    tassert(strncmp("abc", "bbc", 5) < 0);
    tassert(strncmp("abc", "bbc", 4) < 0);
    tassert(strncmp("abc", "bbc", 3) < 0);
    tassert(strncmp("abc", "bbc", 2) < 0);
    tassert(strncmp("abc", "bbc", 1) < 0);
DEF_TEST_END

DEF_TEST(strncmp_returns_gt0_for_greater_left_str_operand) {
    tassert(strncmp("bbc", "abc", 100) > 0);
    tassert(strncmp("bbc", "abc", 5) > 0);
    tassert(strncmp("bbc", "abc", 4) > 0);
    tassert(strncmp("bbc", "abc", 3) > 0);
    tassert(strncmp("bbc", "abc", 2) > 0);
    tassert(strncmp("bbc", "abc", 1) > 0);
DEF_TEST_END

// memcmp
DEF_TEST(memcmp_returns_0_for_equality) {
    tassert(memcmp("abc", "abc", 4) == 0);
    tassert(memcmp("abc", "abc", 3) == 0);
    tassert(memcmp("abc", "abc", 2) == 0);
    tassert(memcmp("abc", "abc", 1) == 0);
    tassert(memcmp("abc", "abc", 0) == 0);
DEF_TEST_END

DEF_TEST(memcmp_returns_lt0_for_greater_right_str_operand) {
    tassert(memcmp("abc", "bbc", 4) < 0);
    tassert(memcmp("abc", "bbc", 3) < 0);
    tassert(memcmp("abc", "bbc", 2) < 0);
    tassert(memcmp("abc", "bbc", 1) < 0);
DEF_TEST_END

DEF_TEST(memcmp_returns_gt0_for_greater_left_str_operand) {
    tassert(memcmp("bbc", "abc", 4) > 0);
    tassert(memcmp("bbc", "abc", 3) > 0);
    tassert(memcmp("bbc", "abc", 2) > 0);
    tassert(memcmp("bbc", "abc", 1) > 0);
DEF_TEST_END

// memset
DEF_TEST(memset_doesnt_overflow) {
    char buf[4] = { 0 };
    memset(buf, 'a', 0);
    memset(buf, 'b', sizeof(buf) - 3);
    tassert(strncmp(buf, "b", sizeof(buf)) == 0);
    memset(buf, 'c', sizeof(buf) - 2);
    tassert(strncmp(buf, "cc", sizeof(buf)) == 0);
    memset(buf, 'd', sizeof(buf) - 1);
    tassert(strncmp(buf, "ddd", sizeof(buf)) == 0);
DEF_TEST_END

// memcpy
DEF_TEST(memcpy_doesnt_overflow) {
    char from[] = { 'a', 'b', 'c'};
    char to[4] = { 0 };
    memcpy(to, from, sizeof(to) - 3);
    tassert(strncmp(to, "a", sizeof(to)) == 0);
    memcpy(to, from, sizeof(to) - 2);
    tassert(strncmp(to, "ab", sizeof(to)) == 0);
    memcpy(to, from, sizeof(to) - 1);
    tassert(strncmp(to, "abc", sizeof(to)) == 0);
DEF_TEST_END
