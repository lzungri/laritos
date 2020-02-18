#pragma once

#include <stdbool.h>
#include <process/types.h>
#include <dstruct/list.h>
#include <component/component.h>
#include <component/vrtimer.h>
#include <component/ticker.h>

static inline bool is_process_in(list_head_t *pcb, list_head_t *list) {
    list_head_t *pos;
    list_for_each(pos, list) {
        if (pos == pcb) {
            return true;
        }
    }
    return false;
}

static inline vrtimer_comp_t *get_vrtimer(void) {
    return component_get_default(COMP_TYPE_VRTIMER, vrtimer_comp_t);
}

static inline ticker_comp_t *get_ticker(void) {
    return component_get_default(COMP_TYPE_TICKER, ticker_comp_t);
}
