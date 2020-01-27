#include <log.h>

#include <stdint.h>
#include <test/test.h>
#include <utils/math.h>

T(math_sign_extend_16_32_preserves_sign) {
    tassert(sign_extend_32(0xFFFF, 16) == 0xFFFFFFFF);
    tassert(sign_extend_32(0x8001, 16) == 0xFFFF8001);
    tassert(sign_extend_32(0x1, 16) == 0x1);
    tassert(sign_extend_32(0x7FFF, 16) == 0x00007FFF);
    tassert(sign_extend_32(0x8FFF, 16) == 0xFFFF8FFF);
TEND

T(math_sign_extend_24_32_preserves_sign) {
    tassert(sign_extend_32(0xFFFFFF, 24) == 0xFFFFFFFF);
    tassert(sign_extend_32(0x800001, 24) == 0xFF800001);
    tassert(sign_extend_32(0x1, 24) == 0x1);
    tassert(sign_extend_32(0x7FFFFF, 24) == 0x007FFFFF);
    tassert(sign_extend_32(0x8FFFFF, 24) == 0xFF8FFFFF);
TEND
