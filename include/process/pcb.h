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
#include <generated/autoconf.h>

typedef struct {
    void *imgaddr;
    secsize_t imgsize;
    void *stack_bottom;
    secsize_t stack_size;
    void *heap_start;
    secsize_t heap_size;
    void *got_start;
    secsize_t got_size;

    spctx_t *sp_ctx;
    spctx_t *sp_ctx_prev;
} pcb_mm_t;

typedef struct {
    pcb_status_t status;
    uint8_t priority;
    struct list_head pcb_node;
    struct list_head sched_node;
} pcb_sched_t;

typedef struct pcb {
    uint16_t pid;
    char name[CONFIG_PROCESS_MAX_NAME_LEN];
    bool kernel;

    char cmd[CONFIG_PROCESS_MAX_CMD_LEN];
    pcb_mm_t mm;
    pcb_sched_t sched;

    int exit_status;
} pcb_t;


/**
 * Kernel process main function type
 *
 * @param data: Pointer to any data you want to send to the kernel function
 */
typedef int (*kproc_main_t)(void *data);


int pcb_init_global_context(void);
int pcb_deinit_global_context(void);
void pcb_assign_pid(pcb_t *pcb);
pcb_t *pcb_alloc(void);
int pcb_free(pcb_t *pcb);
int pcb_register(pcb_t *pcb);
int pcb_unregister(pcb_t *pcb);
void pcb_kill(pcb_t *pcb);
void pcb_kill_and_schedule(pcb_t *pcb);
int pcb_set_priority(pcb_t *pcb, uint8_t priority);
spctx_t *pcb_get_current_pcb_stack_context(void);
pcb_t *pcb_spawn_kernel_process(char *name, kproc_main_t main, void *data, uint32_t stacksize, uint8_t priority);

static inline pcb_t *pcb_get_current(void) {
    pcb_t *pcb = _laritos.sched.running[cpu_get_id()];
    assert(pcb != NULL, "Current pcb cannot be NULL, make sure you are running in process mode");
    return pcb;
}

static inline void pcb_set_current(pcb_t *pcb) {
    _laritos.sched.running[cpu_get_id()] = pcb;
}

static inline void pcb_set_current_pcb_stack_context(spctx_t *spctx) {
    pcb_t *pcb = pcb_get_current();
    verbose_async("Setting current context for pid=%u to 0x%p", pcb->pid, spctx);
    pcb->mm.sp_ctx = spctx;
}

static inline void pcb_set_name(pcb_t *pcb, char *name) {
    strncpy(pcb->name, name, sizeof(pcb->name));
}

#define for_each_process(_p) \
    list_for_each_entry(_p, &_laritos.proc.pcbs, sched.pcb_node)

#define for_each_ready_process(_p) \
    list_for_each_entry(_p, &_laritos.sched.ready_pcbs, sched.sched_node)

#define for_each_ready_process_safe(_p, _n) \
    list_for_each_entry_safe(_p, _n, &_laritos.sched.ready_pcbs, sched.sched_node)

#define for_each_blocked_process(_p) \
    list_for_each_entry(_p, &_laritos.sched.blocked_pcbs, sched.sched_node)

#define for_each_zombie_process(_p) \
    list_for_each_entry(_p, &_laritos.sched.zombie_pcbs, sched.sched_node)
