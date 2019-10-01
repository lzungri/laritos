#pragma once

#include <log.h>
#include <string.h>
#include <component/component.h>

static inline void message_delimiter(void) {
    log(false, "I", "*** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
}

static inline void dump_cur_state(void) {
    // IMPORTANT: Make sure you don't change any relevant registers before
    // calling dump_all_regs()
    extern void dump_all_regs(void);
    dump_all_regs();
    // TODO: Dump kernel info
}

static inline void dump_registered_comps(void) {
    component_t *c;
    log_always("Components:");
    for_each_component(c) {
        log_always("   %s, type: %d", c->id, c->type);
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
