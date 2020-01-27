#include <log.h>

#include <stdint.h>
#include <cpu/cpu.h>
#include <test/test.h>
#include <utils/random.h>
#include <utils/math.h>

T(random_randint_gen_numbers_in_the_given_range) {
    int j;
    for (j = -1000; j < 1000; j++) {
        int i;
        for (i = 0; i < 1000; i++) {
            int32_t rnd = random_int32(j, j + abs(j) * 3);
            tassert(rnd >= j && rnd <= j + abs(j) * 3);
        }
    }
TEND
