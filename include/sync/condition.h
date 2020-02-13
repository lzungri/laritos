#pragma once

#include <log.h>

#include <stdbool.h>
#include <sync/spinlock.h>
#include <dstruct/list.h>

typedef struct {
    list_head_t blocked;
} condition_t;

int condition_init(condition_t *cond);
/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
void condition_wait_locked(condition_t *cond, spinlock_t *spin, irqctx_t *ctx);
/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
struct pcb *condition_notify_locked(condition_t *cond);
/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
bool condition_notify_all_locked(condition_t *cond);


#define CONDITION_STATIC_INIT(_cond) { .blocked = LIST_HEAD_INIT(_cond.blocked), }

/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
#define BLOCK_UNTIL(_expr, _cond, _spin, _ctx) \
    while (!(_expr)) { \
        condition_wait_locked(_cond, _spin, _ctx); \
    }
