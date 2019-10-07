#include <log.h>

#include <test/test.h>

DEF_TEST(strstr_returns_null_when_no_match_is_found) {
    ASSERT_NULL(strstr("abc", "cba", 4));
    return TEST_PASS;
}

DEF_TEST(strstr_returns_null_when_no_match_is_found2) {
    ASSERT_NULL(strstr("abc", "abc", 4));
    return TEST_PASS;
}

DEF_TEST(strstr_returns_null_when_no_match_is_found6) {
    ASSERT_NULL((void *)0xff);
    return TEST_PASS;
}

DEF_TEST(strstr_returns_null_when_no_match_is_found4) {
    ASSERT_NULL(NULL);
    return TEST_PASS;
}

DEF_TEST(strstr_returns_null_when_no_match_is_found5) {
    ASSERT_NULL(strstr("abc", "cba", 4));
    return TEST_PASS;
}
