#pragma once

typedef enum {
    SYSCALL_EXIT = 0,
    SYSCALL_YIELD,
    SYSCALL_PUTS,
    SYSCALL_GETPID,

    SYSCALL_LEN,
} syscall_t;
