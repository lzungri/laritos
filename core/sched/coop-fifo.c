/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <board/core.h>
#include <process/types.h>
#include <driver/core.h>
#include <sched/core.h>
#include <component/sched.h>
#include <mm/heap.h>
#include <cpu/cpu-local.h>

static inline pcb_t *pick_ready_locked(sched_comp_t *sched, struct cpu *cpu, pcb_t *curpcb) {
    return list_first_entry_or_null(CPU_LOCAL_GET_PTR_LOCKED(_laritos.sched.ready_pcbs), pcb_t, sched.sched_node);
}

static int process(board_comp_t *comp) {
    sched_comp_t *s = component_alloc(sizeof(sched_comp_t));
    if (s == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    s->ops.pick_ready_locked = pick_ready_locked;

    if (component_init((component_t *) s, comp->id, comp, COMP_TYPE_SCHED, NULL, NULL) < 0) {
        error("Failed to initialize '%s' scheduler component", comp->id);
        goto fail;
    }

    component_set_info((component_t *) s, "FIFO Scheduler", "lzungri", "FIFO cooperative scheduler");

    if (component_register((component_t *) s) < 0) {
        error("Couldn't register scheduler '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(s);
    return -1;
}

DRIVER_MODULE(coop_fifo, process);
