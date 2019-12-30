#pragma once

#include <log.h>

#include <stdbool.h>
#include <sync/spinlock.h>
#include <dstruct/list.h>

typedef struct {
    struct list_head blocked;
} condition_t;

int condition_init(condition_t *cond);

void condition_wait_locked(condition_t *cond, spinlock_t *spin, irqctx_t *ctx);
struct pcb *condition_notify_locked(condition_t *cond);
bool condition_notify_all_locked(condition_t *cond);

/**
 * Condition holding the _laritos.proclock spinlock
 */
void condition_wait_proclocked(condition_t *cond, irqctx_t *ctx);
/**
 * Notification holding the _laritos.proclock spinlock
 */
struct pcb *condition_notify_proclocked(condition_t *cond);
/**
 * Notification holding the _laritos.proclock spinlock
 */
bool condition_notify_all_proclocked(condition_t *cond);


#define CONDITION_STATIC_INIT(_cond) { .blocked = LIST_HEAD_INIT(_cond.blocked), }

#define SLEEP_UNTIL(_expr, _cond, _spin, _ctx) \
    while (!(_expr)) { \
        condition_wait_locked(_cond, _spin, _ctx); \
    }

#define SLEEP_UNTIL_PROCLOCKED(_expr, _cond, _ctx) \
    while (!(_expr)) { \
        condition_wait_proclocked(_cond, _ctx); \
    }
