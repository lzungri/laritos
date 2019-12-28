#pragma once

#include <log.h>

#include <stdint.h>
#include <string.h>

#include <dstruct/list.h>
#include <cpu.h>
#include <core.h>
#include <utils/assert.h>
#include <mm/slab.h>
#include <process/status.h>
#include <time/tick.h>
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
    struct list_head pcb_node;
    struct list_head sched_node;

    /**
     * List of processes blocked in a particular event.
     * A process can only be waiting for one event at a time.
     */
    struct list_head blockedlst;
} pcb_sched_t;

typedef struct {
    abstick_t last_status_change;
    tick_t ticks_spent[PROC_STATUS_LEN];
} pcb_stats_t;

typedef struct pcb {
    uint16_t pid;
    char name[CONFIG_PROCESS_MAX_NAME_LEN];
    bool kernel;

    char cmd[CONFIG_PROCESS_MAX_CMD_LEN];
    pcb_mm_t mm;
    pcb_sched_t sched;
    pcb_stats_t stats;

    int exit_status;

    struct pcb *parent;
    struct list_head children;
    struct list_head siblings;
} pcb_t;


/**
 * Kernel process main function type
 *
 * @param data: Pointer to any data you want to send to the kernel function
 */
typedef int (*kproc_main_t)(void *data);


int process_init_global_context(void);
int process_deinit_global_context(void);
void process_assign_pid(pcb_t *pcb);
pcb_t *process_alloc(void);
int process_free(pcb_t *pcb);
int process_register_locked(pcb_t *pcb);
int process_unregister_locked(pcb_t *pcb);
void process_unregister_zombie_children_locked(pcb_t *pcb);
void process_kill(pcb_t *pcb);
void process_kill_and_schedule(pcb_t *pcb);
int process_set_priority(pcb_t *pcb, uint8_t priority);
spctx_t *process_get_current_pcb_stack_context(void);
pcb_t *process_spawn_kernel_process(char *name, kproc_main_t main, void *data, uint32_t stacksize, uint8_t priority);

static inline pcb_t *process_get_current(void) {
    pcb_t *pcb = _laritos.sched.running[cpu_get_id()];
    assert(pcb != NULL, "Current pcb cannot be NULL, make sure you are running in process mode");
    return pcb;
}

static inline void process_set_current(pcb_t *pcb) {
    _laritos.sched.running[cpu_get_id()] = pcb;
}

static inline void process_set_current_pcb_stack_context(spctx_t *spctx) {
    pcb_t *pcb = process_get_current();
    verbose_async("Setting current context for pid=%u to 0x%p", pcb->pid, spctx);
    pcb->mm.sp_ctx = spctx;
}

static inline void process_set_name(pcb_t *pcb, char *name) {
    strncpy(pcb->name, name, sizeof(pcb->name));
}

#define for_each_process(_p) \
    list_for_each_entry(_p, &_laritos.proc.pcbs, sched.pcb_node)

#define for_each_child_process_safe(_parent, _child, _temp) \
    list_for_each_entry_safe(_child, _temp, &_parent->children, siblings)

#define for_each_ready_process(_p) \
    list_for_each_entry(_p, &_laritos.sched.ready_pcbs, sched.sched_node)

#define for_each_ready_process_safe(_p, _n) \
    list_for_each_entry_safe(_p, _n, &_laritos.sched.ready_pcbs, sched.sched_node)

#define for_each_blocked_process(_p) \
    list_for_each_entry(_p, &_laritos.sched.blocked_pcbs, sched.sched_node)

#define for_each_zombie_process(_p) \
    list_for_each_entry(_p, &_laritos.sched.zombie_pcbs, sched.sched_node)
