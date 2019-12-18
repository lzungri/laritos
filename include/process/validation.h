#pragma once

#include <process/core.h>
#include <sched/context.h>

bool process_is_valid_kernel_exec_addr(void *addr);
bool process_is_valid_exec_addr(pcb_t *pcb, void *addr);
bool process_is_valid_context(pcb_t *pcb, spctx_t *ctx);
