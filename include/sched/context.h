#pragma once

#include <process/pcb.h>
#include <arch/context.h>

static inline void restore_context(pcb_t * pcb) {
    arch_restore_context(pcb);
}
