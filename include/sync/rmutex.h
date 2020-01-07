#pragma once

#include <stdint.h>
#include <sync/spinlock.h>
#include <sync/condition.h>
#include <process/core.h>

typedef struct {
    uint16_t lock_count;
    spinlock_t lock;
    condition_t cond;
    pcb_t *owner;
} rmutex_t;

int rmutex_init(rmutex_t *mutex);
int rmutex_acquire(rmutex_t *mutex);
int rmutex_release(rmutex_t *mutex);
