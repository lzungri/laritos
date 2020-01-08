#pragma once

#include <stdint.h>
#include <sync/spinlock.h>
#include <sync/condition.h>

typedef struct {
    uint16_t count;
    spinlock_t lock;
    condition_t cond;
} sem_t;

int sem_init(sem_t *sem, uint16_t count);
int sem_acquire(sem_t *sem);
int sem_release(sem_t *sem);
