#pragma once

#include <log.h>

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
