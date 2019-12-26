#include <log.h>

#include <sched/context.h>

void asm_arch_context_restore(spctx_t *spctx) {
    arch_context_restore(spctx);
}
