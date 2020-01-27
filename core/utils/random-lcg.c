#include <log.h>

#include <core.h>
#include <utils/random.h>
#include <utils/math.h>

void random_seed(uint32_t seed) {
    _laritos.rndseed = seed;
}

/**
 * Same LCG (Linear Congruential Generator) used by glicb, basic and exploitable.
 */
int32_t random_int32(int32_t min, int32_t max) {
    /**
     * X(n+1) = (X(n) * a + c) % m
     *
     * multiplier a = 1103515245
     * increment c  = 12345
     * modulus m    = 2^31
     */
    _laritos.rndseed = (_laritos.rndseed * 1103515245 + 12345) & 0x7fffffff;
    return _laritos.rndseed % (max - min + 1) + min;
}



#ifdef CONFIG_TEST_CORE_UTILS_RANDOM_LCG
#include __FILE__
#endif
