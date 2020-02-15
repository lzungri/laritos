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
    tassert(!spinlock_is_taken(&s));

    irqctx_t ctx;
    spinlock_acquire(&s, &ctx);
    tassert(spinlock_is_taken(&s));
    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_taken(&s));
TEND

T(spinlock_irqs_are_disabled_inside_the_critical_section) {
    tassert(irq_is_enabled());
    spinlock_t s;
    spinlock_init(&s);

    irqctx_t ctx;
    spinlock_acquire(&s, &ctx);
    tassert(spinlock_is_taken(&s));
    tassert(!irq_is_enabled());
    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_taken(&s));
TEND

T(spinlock_trylock_grabs_the_lock_and_returns_true_if_not_taken) {
    irqctx_t ctx;
    spinlock_t s;
    spinlock_init(&s);
    tassert(!spinlock_is_taken(&s));

    tassert(spinlock_trylock(&s, &ctx));
    tassert(spinlock_is_taken(&s));
    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_taken(&s));
TEND

T(spinlock_trylock_doesnt_change_lock_status_and_returns_false_if_taken) {
    irqctx_t ctx;
    spinlock_t s;
    spinlock_init(&s);

    spinlock_acquire(&s, &ctx);
    tassert(spinlock_is_taken(&s));

    tassert(!spinlock_trylock(&s, &ctx));
    tassert(spinlock_is_taken(&s));

    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_taken(&s));
TEND
