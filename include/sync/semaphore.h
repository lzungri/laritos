#pragma once

#include <stdint.h>
#include <sync/spinlock.h>
#include <dstruct/list.h>

typedef struct {
    uint16_t count;
    spinlock_t lock;
    struct list_head blocked;
} sem_t;

int sem_init(sem_t *sem, uint16_t count);
int sem_acquire(sem_t *sem);
int sem_release(sem_t *sem);
