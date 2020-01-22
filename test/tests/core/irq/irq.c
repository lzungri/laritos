#include <log.h>

#include <stdint.h>
#include <test/test.h>
#include <irq/core.h>

T(irq_disable_enable_works_as_expected) {
    irq_disable_local();
    tassert(!irq_is_enabled());
    irq_enable_local();
    tassert(irq_is_enabled());
TEND

T(irq_enable_disable_calls_should_match_to_actually_change_irqs) {
    irq_disable_local();
    tassert(!irq_is_enabled());
    irq_disable_local();
    tassert(!irq_is_enabled());

    irq_enable_local();
    tassert(!irq_is_enabled());

    irq_disable_local();
    tassert(!irq_is_enabled());

    irq_enable_local();
    tassert(!irq_is_enabled());
    irq_enable_local();
    tassert(irq_is_enabled());
TEND
