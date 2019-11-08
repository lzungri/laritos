#pragma once

#include <cpu.h>
#include <process/pcb.h>
#include <arch/context.h>

static inline void context_init(struct pcb *pcb, void *retaddr, cpu_mode_t mode) {
    arch_context_init(pcb, retaddr, mode);
}

static inline void context_restore(pcb_t * pcb) {
    arch_context_restore(pcb);
}
