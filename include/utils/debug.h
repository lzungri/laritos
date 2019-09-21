#pragma once

#include <log.h>
#include <component/component.h>
#include <component/hwcomp.h>

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
    log(false, "I", "Components:");
    for_each_component(c) {
        log(false, "I", "   %s, type: %d, subtype: %d", c->id, c->type, c->stype);
        if (c->type == COMP_TYPE_HW) {
            hwcomp_t *hw = (hwcomp_t *) c;
            log(false, "I", "      product: %s", hw->product);
            log(false, "I", "      vendor: %s", hw->vendor);
            log(false, "I", "      description: %s", hw->description);
        }
    }
}
