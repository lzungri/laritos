#pragma once

#include <log.h>
#include <stdbool.h>
#include <stdint.h>
#include <cpu/core.h>
#include <core.h>
#include <process/core.h>
#include <mm/exc-handlers.h>

#define CANARY_STACK 0xCCAADDBBL

static inline void spprot_setup(pcb_t *pcb) {
    uint32_t *bottom = pcb->mm.stack_bottom;
    *bottom = CANARY_STACK;
    uint32_t *top = (uint32_t *) pcb->mm.stack_top;
    *top = CANARY_STACK;
}

static inline bool spprot_is_stack_corrupted(pcb_t *pcb) {
    uint32_t *bottom = pcb->mm.stack_bottom;
    uint32_t *top = (uint32_t *) pcb->mm.stack_top;
    if (*bottom != CANARY_STACK || *top != CANARY_STACK) {
        error_async("Stack overflow pid=%u, stack=0x%p-0x%p", pcb->pid, pcb->mm.stack_bottom, (char *) pcb->mm.stack_bottom + pcb->mm.stack_size);
        error_async("Expected canaries head=0x%lX tail=0x%lX, got head=0x%lX tail=0x%lX", CANARY_STACK, CANARY_STACK, *top, *bottom);
        return true;
    }
    return false;
}
