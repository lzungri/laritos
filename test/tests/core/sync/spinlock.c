/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <test/test.h>
#include <sync/spinlock.h>
#include <irq/core.h>
#include <test/utils/process.h>
#include <utils/utils.h>
#include <process/core.h>
#include <generated/autoconf.h>

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

T(spinlock_trylock_disables_irqs_when_it_grabs_the_lock) {
    irqctx_t ctx;
    spinlock_t s;
    spinlock_init(&s);
    tassert(!spinlock_is_locked(&s));

    tassert(spinlock_trylock(&s, &ctx));
    tassert(spinlock_owned_by_me(&s));
    tassert(spinlock_is_locked(&s));
    tassert(!irq_is_enabled());
    spinlock_release(&s, &ctx);
    tassert(!spinlock_is_locked(&s));
    tassert(irq_is_enabled());
TEND

#ifdef CONFIG_SMP
T(spinlock_trylock_doesnt_change_lock_status_and_returns_false_if_already_taken) {
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
#endif

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
