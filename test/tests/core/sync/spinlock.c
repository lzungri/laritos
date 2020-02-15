#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <test/test.h>
#include <sync/spinlock.h>
#include <irq/core.h>
#include <test/utils.h>
#include <utils/utils.h>
#include <process/core.h>

T(spinlock_acquire_grabs_the_lock_and_release_restores_it) {
    spinlock_t s;
    spinlock_init(&s);
    tassert(!spinlock_is_locked(&s));

    irqctx_t ctx;
    spinlock_acquire(&s, &ctx);
    tassert(spinlock_owned_by_me(&s));
    tassert(spinlock_is_locked(&s));
    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_locked(&s));
    tassert(!spinlock_owned_by_me(&s));
TEND

T(spinlock_irqs_are_disabled_inside_the_critical_section) {
    tassert(irq_is_enabled());
    spinlock_t s;
    spinlock_init(&s);

    irqctx_t ctx;
    spinlock_acquire(&s, &ctx);
    tassert(spinlock_is_locked(&s));
    tassert(spinlock_owned_by_me(&s));
    tassert(!irq_is_enabled());
    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_locked(&s));
TEND

T(spinlock_trylock_grabs_the_lock_and_returns_true_if_not_taken) {
    irqctx_t ctx;
    spinlock_t s;
    spinlock_init(&s);
    tassert(!spinlock_is_locked(&s));

    tassert(spinlock_trylock(&s, &ctx));
    tassert(spinlock_owned_by_me(&s));
    tassert(spinlock_is_locked(&s));
    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_locked(&s));
TEND

T(spinlock_trylock_doesnt_change_lock_status_and_returns_false_if_taken) {
    irqctx_t ctx;
    spinlock_t s;
    spinlock_init(&s);

    spinlock_acquire(&s, &ctx);
    tassert(spinlock_is_locked(&s));

    tassert(!spinlock_trylock(&s, &ctx));
    tassert(spinlock_is_locked(&s));

    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_locked(&s));
TEND

static int spinowner(void *data) {
    irqctx_t ctx;
    spinlock_t s;
    spinlock_init(&s);
    spinlock_acquire(&s, &ctx);
    bool owned_by_me = spinlock_owned_by_me(&s);
    spinlock_release(&s, &ctx);
    return (int) owned_by_me;
}

T(spinlock_locked_by_me_returns_true_to_the_lock_process_owner) {
    pcb_t *p1 = process_spawn_kernel_process("owner", spinowner, NULL,
                        8196, process_get_current()->sched.priority - 1);
    tassert(p1 != NULL);

    int own_by_spinowner;
    process_wait_for(p1, &own_by_spinowner);
    tassert(own_by_spinowner);
TEND
