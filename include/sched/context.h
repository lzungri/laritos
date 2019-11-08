#pragma once

#include <process/pcb.h>
#include <arch/context.h>

static inline void context_restore(pcb_t * pcb) {
    arch_context_restore(pcb);
}
