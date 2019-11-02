#pragma once

#include <log.h>

#include <stdint.h>
#include <string.h>

#include <dstruct/list.h>
#include <cpu.h>
#include <core.h>
#include <mm/slab.h>
#include <process/status.h>
#include <generated/autoconf.h>

typedef struct {
    uint16_t pid;

    char cmd[CONFIG_PROCESS_MAX_CMD_LEN];
    void *imgaddr;
    imgsize_t imgsize;
    regs_t regs;
    pcb_status_t status;

    struct list_head pcb_node;
    struct list_head sched_node;
} pcb_t;


int pcb_init_global_context(void);
int pcb_deinit_global_context(void);
void pcb_assign_pid(pcb_t *pcb);
pcb_t *pcb_alloc(void);
int pcb_free(pcb_t *pcb);
int pcb_register(pcb_t *pcb);
int pcb_unregister(pcb_t *pcb);

#define for_each_process(_p) \
    list_for_each_entry(_p, &_laritos.pcbs, pcb_node)

#define for_each_ready_process(_p) \
    list_for_each_entry(_p, &_laritos.ready_pcbs, sched_node)

#define for_each_blocked_process(_p) \
    list_for_each_entry(_p, &_laritos.blocked_pcbs, sched_node)

#define for_each_zombie_process(_p) \
    list_for_each_entry(_p, &_laritos.zombie_pcbs, sched_node)
