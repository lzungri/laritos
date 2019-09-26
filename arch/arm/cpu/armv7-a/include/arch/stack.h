#pragma once

#include <stdint.h>

/**
 * Structure mapping the set of registers pushed into the
 * stack by the handlers
 */
typedef struct {
    int32_t r[13];
    int32_t lr;
} spregs_t;
