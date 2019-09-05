#pragma once

#include <stdint.h>

typedef struct {
    int32_t p0;
    int32_t p1;
    int32_t p2;
    int32_t p3;
    int32_t p4;
    int32_t p5;
    int32_t p6;
} syscall_params_t;

int syscall(int sysno, syscall_params_t *params);
