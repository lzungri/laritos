#pragma once

#include <process/types.h>
#include <sched/context.h>

/**
 * NOTE: Must be called with pcbs_data_lock held
 */
bool process_is_valid_kernel_exec_addr_locked(void *addr);
/**
 * NOTE: Must be called with pcbs_data_lock held
 */
bool process_is_valid_exec_addr_locked(pcb_t *pcb, void *addr);
/**
 * NOTE: Must be called with pcbs_data_lock held
 */
bool process_is_valid_context_locked(pcb_t *pcb, spctx_t *ctx);
