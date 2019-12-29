#pragma once

#include <log.h>

#include <process/core.h>
#include <sync/spinlock.h>
#include <dstruct/list.h>

typedef struct {
    struct list_head blocked;
} condition_t;

int condition_init(condition_t *cond);
void condition_wait_spinlock(condition_t *cond, spinlock_t *spin, irqctx_t *ctx);
pcb_t *condition_notify(condition_t *cond);


#define sleep_until(_expr, _cond, _spin, _ctx) \
    while (!(_expr)) { \
        condition_wait_spinlock(_cond, _spin, _ctx); \
    }
