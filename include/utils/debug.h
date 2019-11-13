#pragma once

#include <log.h>
#include <string.h>
#include <component/component.h>
#include <arch/debug.h>

static inline void message_delimiter(void) {
    error_async("*** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
}

/**
 * Print the current state of the OS
 *
 * Note: __attribute__((always_inline)) so that this function is always expanded and thus
 * we get a useful PC/LR, not just the PC inside the dump_cur_state() function (in case it
 * wasn't expanded by the compiler)
 */
__attribute__((always_inline)) static inline void dump_cur_state(void) {
    // IMPORTANT: Make sure you don't change any relevant registers before
    // calling dump_all_regs()
    arch_dump_all_regs();
    // TODO: Dump kernel info
    void heap_dump_info(void);
    heap_dump_info();
}

static inline void dump_registered_comps(void) {
    component_t *c;
    log_always("Components:");
    for_each_component(c) {
        log_always("   %s@0x%p, type: %d", c->id, c, c->type);
        if (strnlen(c->product, sizeof(c->product)) > 0) {
            log_always("      product: %s", c->product);
        }
        if (strnlen(c->vendor, sizeof(c->vendor)) > 0) {
            log_always("      vendor: %s", c->vendor);
        }
        if (strnlen(c->description, sizeof(c->description)) > 0) {
            log_always("      description: %s", c->description);
        }
    }
}
