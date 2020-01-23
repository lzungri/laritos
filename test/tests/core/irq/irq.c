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

    irq_restore_context(&ctx);
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
        arch_irq_restore_context(&ctx2);

        tassert(!irq_is_enabled());
    }
    arch_irq_restore_context(&ctx);

    tassert(irq_is_enabled() == irq_orig_enabled);
TEND
