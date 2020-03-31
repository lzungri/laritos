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

#include <log.h>

#include <stdbool.h>
#include <stdint.h>
#include <test/test.h>
#include <irq/core.h>
#include <irq/types.h>

T(irq_disable_enable_works_as_expected) {
    irq_disable_local();
    tassert(!irq_is_enabled());
    irq_enable_local();
    tassert(irq_is_enabled());
TEND

T(irq_enable_disable_calls_work_as_expected) {
    irqctx_t ctx;
    irq_save_context(&ctx);

    irq_disable_local();
    tassert(!irq_is_enabled());
    irq_disable_local();
    tassert(!irq_is_enabled());

    irq_enable_local();
    tassert(irq_is_enabled());

    irq_local_restore_ctx(&ctx);
TEND

T(irq_enable_disable_with_ctx_calls_work_as_expected) {
    bool irq_orig_enabled = irq_is_enabled();

    irqctx_t ctx;
    irq_disable_local_and_save_ctx(&ctx);
    {
        tassert(!irq_is_enabled());
        tassert(irq_is_enabled_in_ctx(&ctx) == irq_orig_enabled);

        irqctx_t ctx2;
        irq_disable_local_and_save_ctx(&ctx2);
        {
            tassert(!irq_is_enabled());
            tassert(!irq_is_enabled_in_ctx(&ctx2));
        }
        irq_local_restore_ctx(&ctx2);

        tassert(!irq_is_enabled());
    }
    irq_local_restore_ctx(&ctx);

    tassert(irq_is_enabled() == irq_orig_enabled);
TEND
