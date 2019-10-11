#pragma once

#include <log.h>
#include <cpu.h>
#include <utils/debug.h>

#define CANARY 0xAACCBBDDL

typedef struct {
    regpc_t pc;
    uint32_t canary;
    size_t size;
} bufprot_head_t;

typedef struct {
    uint32_t canary;
} bufprot_tail_t;

/**
 * Custom malloc() which adds buffer protection metadata to detect buffer overflows
 *
 * @param size: Amount to allocate
 * @return: Pointer to allocated chunk
 */
__attribute__((always_inline)) static inline void *malloc(size_t size) {
    bufprot_head_t *h = _malloc(sizeof(bufprot_head_t) + size + sizeof(bufprot_tail_t));
    if (h == NULL) {
        return NULL;
    }

    h->pc = regs_get_pc();
    h->canary = CANARY;
    h->size = size;
    bufprot_tail_t *t = (bufprot_tail_t *) ((char *) h + sizeof(bufprot_head_t) + size);
    t->canary = CANARY;
    return (char *) h + sizeof(bufprot_head_t);
}

/**
 * Custom free() which checks if the canaries were corrupted
 *
 * @param ptr: Pointer to mallocated chunk
 */
__attribute__((always_inline)) static inline void free(void *ptr) {
    bufprot_head_t *h = (bufprot_head_t *) ((char *) ptr - sizeof(bufprot_head_t));
    bufprot_tail_t *t = (bufprot_tail_t *) ((char *) ptr + h->size);
    if (h->canary != CANARY || t->canary != CANARY) {
        regpc_t pc = regs_get_pc();
        error("Buffer overflow on block with size %zu bytes:", h->size);
        error("  Expected canaries head=0x%lX tail=0x%lX, got head=0x%lX tail=0x%lX", CANARY, CANARY, h->canary, t->canary);
        error("  Allocation at pc=0x%p", h->pc);
        error("  Run gdb-multiarch -batch -n -ex 'file bin/laritos.elf' -ex 'disassemble /m 0x%p'", h->pc);
        error("  Deallocation at pc=0x%p", pc);
        error("  Run gdb-multiarch -batch -n -ex 'file bin/laritos.elf' -ex 'disassemble /m 0x%p'", pc);
        fatal("ABORT");
    }
    _free(h);
}
