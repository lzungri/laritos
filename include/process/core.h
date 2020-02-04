#pragma once

#include <log.h>

#include <stdint.h>
#include <string.h>

#include <dstruct/list.h>
#include <cpu/cpu.h>
#include <cpu/cpu-local.h>
#include <core.h>
#include <refcount.h>
#include <utils/assert.h>
#include <mm/slab.h>
#include <process/status.h>
#include <time/tick.h>
#include <sync/atomic.h>
#include <sync/condition.h>
#include <syscall/syscall-no.h>
#include <generated/autoconf.h>

typedef struct {
    void *imgaddr;
    secsize_t imgsize;

    void *text_start;
    secsize_t text_size;
    void *data_start;
    secsize_t data_size;
    void *got_start;
    secsize_t got_size;
    void *bss_start;
    secsize_t bss_size;
    void *heap_start;
    secsize_t heap_size;
    void *stack_bottom;
    secsize_t stack_size;

    spctx_t *sp_ctx;
} pcb_mm_t;

typedef struct {
    process_status_t status;
    uint8_t priority;

    /**
     * Node used to link a process to the _laritos.proc.pcbs list
     */
    struct list_head pcb_node;

    /**
     * Node used to link a process to a particular scheduling list. So far,
     * it could be a per-cpu READY list or a blocked list (if the blocking
     * event has any).
     */
    struct list_head sched_node;
} pcb_sched_t;

typedef struct {
    abstick_t last_status_change;
    tick_t ticks_spent[PROC_STATUS_LEN];
    atomic32_t syscalls[SYSCALL_LEN];
} pcb_stats_t;

typedef struct pcb {
    uint16_t pid;
    char name[CONFIG_PROCESS_MAX_NAME_LEN];
    bool kernel;

    char cmd[CONFIG_PROCESS_MAX_CMD_LEN];
    char cwd[CONFIG_FS_MAX_FILENAME_LEN];
    pcb_mm_t mm;
    pcb_sched_t sched;
    pcb_stats_t stats;

    int exit_status;

    struct pcb *parent;
    condition_t parent_waiting_cond;
    struct list_head children;
    struct list_head siblings;

    refcount_t refcnt;
} pcb_t;


/**
 * Kernel process main function type
 *
 * @param data: Pointer to any data you want to send to the kernel function
 */
typedef int (*kproc_main_t)(void *data);


int process_init_global_context(void);
void process_assign_pid(pcb_t *pcb);
pcb_t *process_alloc(void);
int process_free(pcb_t *pcb);
int process_register_locked(pcb_t *pcb);
int process_release_zombie_resources_locked(pcb_t *pcb);
void process_unregister_zombie_children_locked(pcb_t *pcb);
void process_kill(pcb_t *pcb);
void process_kill_locked(pcb_t *pcb);
void process_kill_and_schedule(pcb_t *pcb);
int process_set_priority(pcb_t *pcb, uint8_t priority);
spctx_t *process_get_current_pcb_stack_context(void);
pcb_t *process_spawn_kernel_process(char *name, kproc_main_t main, void *data, uint32_t stacksize, uint8_t priority);
int process_wait_for(pcb_t *pcb, int *status);
int process_wait_pid(uint16_t pid, int *status);

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
    strncpy(pcb->name, name, sizeof(pcb->name));
}

#define for_each_process_locked(_p) \
    list_for_each_entry(_p, &_laritos.proc.pcbs, sched.pcb_node)

#define for_each_child_process_locked(_parent, _child) \
    list_for_each_entry(_child, &_parent->children, siblings)

#define for_each_child_process_safe_locked(_parent, _child, _temp) \
    list_for_each_entry_safe(_child, _temp, &_parent->children, siblings)

#define for_each_ready_process_locked(_p) \
    list_for_each_entry(_p, CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs), sched.sched_node)

#define for_each_ready_process_safe_locked(_p, _n) \
    list_for_each_entry_safe(_p, _n, CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs), sched.sched_node)
