#include <log.h>

#include <sched/context.h>

void asm_arch_context_restore(pcb_t *pcb) {
    arch_context_restore(pcb);
}
