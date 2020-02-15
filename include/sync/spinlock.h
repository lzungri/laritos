#pragma once

#include <stdbool.h>
#include <irq/core.h>
#include <arch/spinlock.h>
#include <generated/autoconf.h>

/**
 * Note regarding spinlocks:
 *    If the OS is compiled with the CONFIG_SMP option (i.e. multiprocessing support), then
 *    we need to use a lock to guarantee no other processor is running inside the critical section.
 *    If the OS was compiled without the CONFIG_SMP option (i.e. uniprocessor), then
 *    there is no need to use lock variables, we just need to disable local interrupts, which will
 *    prevent local context switches.
 */

struct pcb;

typedef struct {
#ifdef CONFIG_SMP
    arch_spinlock_t lock;
#endif
    struct pcb *owner;
} spinlock_t;


int spinlock_init(spinlock_t *lock);
int spinlock_acquire(spinlock_t *lock, irqctx_t *ctx);
int spinlock_trylock(spinlock_t *lock, irqctx_t *ctx);
int spinlock_release(spinlock_t *lock, irqctx_t *ctx);
bool spinlock_is_locked(spinlock_t *lock);
bool spinlock_owned_by_me(spinlock_t *lock);
