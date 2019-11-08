#pragma once

#include <log.h>

#include <cpu.h>
#include <process/pcb.h>
#include <arch/context.h>

static inline void context_init(struct pcb *pcb, void *retaddr, cpu_mode_t mode) {
    verbose_async("Initializing context for pid=%u, retaddr=0x%p, mode=%u", pcb->pid, retaddr, mode);
    arch_context_init(pcb, retaddr, mode);
}

static inline void context_restore(pcb_t * pcb) {
    verbose_async("Restoring context for pid=%u", pcb->pid);
    arch_context_restore(pcb);
}

static inline void context_save(pcb_t *pcb, void *retaddr) {
    verbose_async("Saving context for pid=%u, retaddr=0x%p", pcb->pid, retaddr);
    arch_context_save(pcb, retaddr);
}
