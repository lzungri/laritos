#pragma once

#include <stdint.h>
#include <process/core.h>
#include <component/cpu.h>
#include <component/component.h>
#include <component/ticker.h>

struct sched_comp;
typedef struct {
    pcb_t *(*pick_ready_locked)(struct sched_comp *sched, cpu_t *cpu, pcb_t *curpcb);
} sched_comp_ops_t;

struct ticker_comp;
typedef struct sched_comp {
    component_t parent;

    ticker_comp_t *ticker;

    sched_comp_ops_t ops;
} sched_comp_t;
