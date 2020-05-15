#pragma once

#include <log.h>

#include <stdint.h>
#include <string.h>

#include <core.h>
#include <process/types.h>
#include <dstruct/list.h>
#include <cpu/core.h>
#include <cpu/cpu-local.h>
#include <assert.h>
#include <time/tick.h>
#include <sync/spinlock.h>
#include <irq/core.h>

int process_init_global_context(void);
void process_assign_pid(pcb_t *pcb);
pcb_t *process_alloc(void);
/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held or the process to be freed
 * must not have been already registered via process_register().
 */
int process_free(pcb_t *pcb);
int process_register(pcb_t *pcb);
/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
int process_release_zombie_resources_locked(pcb_t *pcb);
void process_unregister_zombie_children(pcb_t *pcb);
void process_kill(pcb_t *pcb);
/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
void process_kill_locked(pcb_t *pcb);
void process_kill_and_schedule(pcb_t *pcb);
int process_set_priority(pcb_t *pcb, uint8_t priority);
spctx_t *process_get_current_pcb_stack_context(void);
pcb_t *process_spawn_kernel_process(char *name, kproc_main_t main, void *data, uint32_t stacksize, uint8_t priority);
void process_exit(int exit_status);
int process_wait_for(pcb_t *pcb, int *status);
int process_wait_pid(uint16_t pid, int *status);
/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
uint32_t process_get_avail_stack_locked(pcb_t *pcb);

static inline pcb_t *process_get_current(void) {
    pcb_t *pcb = CPU_LOCAL_GET(_laritos.sched.running);
    assert(pcb != NULL, "Current pcb cannot be NULL, make sure you are running in process mode");
    return pcb;
}

static inline void process_set_current(pcb_t *pcb) {
    CPU_LOCAL_SET(_laritos.sched.running, pcb);
}

static inline void process_set_current_pcb_stack_context(spctx_t *spctx) {
    pcb_t *pcb = process_get_current();
    insane_async("Setting current context for pid=%u to 0x%p", pcb->pid, spctx);
    pcb->mm.sp_ctx = spctx;
}

static inline void process_set_name(pcb_t *pcb, char *name) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    strncpy(pcb->name, name, sizeof(pcb->name));
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
}

/**
 * NOTE: Must be called with pcbs_lock held
 */
#define for_each_process_locked(_p) \
    list_for_each_entry(_p, &_laritos.proc.pcbs, sched.pcb_node)

/**
 * NOTE: Must be called with pcbs_data_lock held
 */
#define for_each_child_process_locked(_parent, _child) \
    list_for_each_entry(_child, &_parent->children, siblings)

/**
 * NOTE: Must be called with pcbs_data_lock held
 */
#define for_each_child_process_safe_locked(_parent, _child, _temp) \
    list_for_each_entry_safe(_child, _temp, &_parent->children, siblings)

/**
 * NOTE: Must be called with pcbs_data_lock held
 */
#define for_each_ready_process_locked(_p) \
    list_for_each_entry(_p, CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs), sched.sched_node)

/**
 * NOTE: Must be called with pcbs_data_lock held
 */
#define for_each_ready_process_safe_locked(_p, _n) \
    list_for_each_entry_safe(_p, _n, CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs), sched.sched_node)
