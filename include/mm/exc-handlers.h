#pragma once

#include <stdint.h>
#include <arch/context-types.h>

void exc_dump_process_info(pcb_t *pcb);
/**
 * NOTE: Must be called with pcbs_data_lock held
 */
void exc_dump_process_info_locked(pcb_t *pcb);
void exc_handle_process_exception(pcb_t *pcb);
void exc_undef_handler(int32_t pc, spctx_t *ctx);
void exc_prefetch_handler(int32_t pc, spctx_t *ctx);
void exc_abort_handler(int32_t pc, spctx_t *ctx);
