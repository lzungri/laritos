#pragma once

#include <stdint.h>
#include <arch/context-types.h>

void exc_undef_handler(int32_t pc, spctx_t *ctx);
void exc_prefetch_handler(int32_t pc, spctx_t *ctx);
void exc_abort_handler(int32_t pc, spctx_t *ctx);
