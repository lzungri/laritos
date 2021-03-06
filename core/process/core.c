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

#include <dstruct/list.h>
#include <cpu/core.h>
#include <cpu/cpu-local.h>
#include <core.h>
#include <mm/slab.h>
#include <mm/heap.h>
#include <mm/spprot.h>
#include <process/status.h>
#include <process/core.h>
#include <process/sysfs.h>
#include <sched/core.h>
#include <sched/context.h>
#include <sync/spinlock.h>
#include <sync/condition.h>
#include <sync/atomic.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <time/core.h>
#include <utils/utils.h>
#include <loader/loader.h>
#include <generated/autoconf.h>

int process_init_global_context(void) {
    INIT_LIST_HEAD(&_laritos.proc.pcbs);
    spinlock_init(&_laritos.proc.pcbs_lock);
    spinlock_init(&_laritos.proc.pcbs_data_lock);

    list_head_t *l;
    CPU_LOCAL_FOR_EACH_CPU_VAR(_laritos.sched.ready_pcbs, l) {
        INIT_LIST_HEAD(l);
    }

    _laritos.proc.pcb_slab = slab_create(CONFIG_PROCESS_MAX_CONCURRENT_PROCS, sizeof(pcb_t));
    return _laritos.proc.pcb_slab != NULL ? 0 : -1;
}

void process_assign_pid(pcb_t *pcb) {
    pcb->pid = (uint16_t) slab_get_slab_position(_laritos.proc.pcb_slab, pcb);
}

int process_free(pcb_t *pcb) {
    if (pcb->sched.status != PROC_STATUS_NOT_INIT && pcb->sched.status != PROC_STATUS_ZOMBIE) {
        error_async("You can only free a process in NOT_INIT or ZOMBIE state");
        return -1;
    }
    // Wipe memory region for security reasons
    memset(pcb, 0, sizeof(pcb));
    slab_free(_laritos.proc.pcb_slab, pcb);
    return 0;
}

/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held. To do so, make
 * sure you protect ref_dec(&pcb->refcnt) with the lock
 */
static void free_process_slab_locked(refcount_t *ref) {
    pcb_t *pcb = container_of(ref, pcb_t, refcnt);
    process_free(pcb);
}

pcb_t *process_alloc(void) {
    pcb_t *pcb = slab_alloc(_laritos.proc.pcb_slab);
    if (pcb == NULL) {
        error_async("Couldn't allocate memory for process");
        return NULL;
    }

    memset(pcb, 0, sizeof(pcb_t));
    INIT_LIST_HEAD(&pcb->sched.pcb_node);
    INIT_LIST_HEAD(&pcb->sched.sched_node);
    INIT_LIST_HEAD(&pcb->children);
    INIT_LIST_HEAD(&pcb->siblings);
    condition_init(&pcb->parent_waiting_cond);
    ref_init(&pcb->refcnt, free_process_slab_locked);
    int i;
    for (i = 0; i < ARRAYSIZE(pcb->stats.syscalls); i++) {
        atomic32_init(&pcb->stats.syscalls[i], 0);
    }
    pcb->sched.status = PROC_STATUS_NOT_INIT;
    process_set_name(pcb, "?");
    process_assign_pid(pcb);
    pcb->cwd = _laritos.fs.root;

    pcb->fs.fds_slab = slab_create(CONFIG_PROCESS_MAX_OPEN_FILES, sizeof(fs_file_t));
    if (pcb->fs.fds_slab == NULL) {
        error_async("Couldn't allocate memory for process fd slab");
        goto error_fds;
    }

    return pcb;

error_fds:
    slab_free(_laritos.proc.pcb_slab, pcb);
    return NULL;
}

int process_register(pcb_t *pcb) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_lock, &ctx);
    irqctx_t datactx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &datactx);

    debug_async("Registering process with pid=%u, priority=%u", pcb->pid, pcb->sched.priority);

    if (_laritos.process_mode) {
        pcb_t *parent = process_get_current();
        pcb->parent = parent;

        list_add_tail(&pcb->siblings, &parent->children);
        // Parent pcb is now referencing its child, increase ref counter
        ref_inc(&pcb->refcnt);
    }

    if (_laritos.components_loaded) {
        time_get_monotonic_time(&pcb->sched.start_time);
    }

    list_add_tail(&pcb->sched.pcb_node, &_laritos.proc.pcbs);

    sched_move_to_ready_locked(pcb);

    if (process_sysfs_create(pcb) < 0) {
        error("Error creating sysfs entries for pid=%u", pcb->pid);
    }

    spinlock_release(&_laritos.proc.pcbs_data_lock, &datactx);
    spinlock_release(&_laritos.proc.pcbs_lock, &ctx);

    return 0;
}

int process_release_zombie_resources_locked(pcb_t *pcb) {
    if (pcb->sched.status != PROC_STATUS_ZOMBIE) {
        error_async("You can only release resources of a ZOMBIE process");
        return -1;
    }

    debug_async("Releasing zombie resources for pid=%u, exit status=%d", pcb->pid, pcb->exit_status);

    if (process_sysfs_remove(pcb) < 0) {
        error("Error removing sysfs entries for pid=%u", pcb->pid);
    }

    list_del(&pcb->sched.pcb_node);
    list_del(&pcb->sched.sched_node);

    free(pcb->mm.imgaddr);
    pcb->mm.imgaddr = NULL;

    slab_destroy(pcb->fs.fds_slab);
    pcb->fs.fds_slab = NULL;

    return 0;
}

/**
 * Note: Must be called with _laritos.proc.pcbs_data_lock held
 */
static inline void handle_dead_child_locked(pcb_t *pcb, int *status) {
    if (status != NULL) {
        *status = pcb->exit_status;
    }
    // Remove from the children list
    list_del(&pcb->siblings);
    // Parent is no longer referencing its child
    ref_dec(&pcb->refcnt);
}

void process_unregister_zombie_children(pcb_t *pcb) {
    pcb_t *child;
    pcb_t *temp;

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    for_each_child_process_safe_locked(pcb, child, temp) {
        if (child->sched.status == PROC_STATUS_ZOMBIE) {
            handle_dead_child_locked(child, NULL);
        }
    }

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
}

spctx_t *asm_process_get_current_pcb_stack_context(void) {
    return process_get_current()->mm.sp_ctx;
}

pcb_t *asm_process_get_current(void) {
    return process_get_current();
}

void process_kill_locked(pcb_t *pcb) {
    verbose_async("Killing process pid=%u", pcb->pid);
    sched_move_to_zombie_locked(pcb);
}

void process_kill(pcb_t *pcb) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);
    process_kill_locked(pcb);
    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
}

void process_kill_and_schedule(pcb_t *pcb) {
    process_kill(pcb);
    schedule();
}

int process_set_priority(pcb_t *pcb, uint8_t priority) {
    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    if (!pcb->kernel && priority < CONFIG_SCHED_PRIORITY_MAX_USER) {
        error_async("Invalid priority for a user process, max priority = %u", CONFIG_SCHED_PRIORITY_MAX_USER);
        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
        return -1;
    }

    debug_async("Setting priority for process at 0x%p to %u", pcb, priority);
    uint8_t prev_prio = pcb->sched.priority;
    pcb->sched.priority = priority;

    // If the process is in the ready queue, then reorder it based on the new priority
    if (pcb->sched.status == PROC_STATUS_READY) {
        sched_move_to_ready_locked(pcb);
    } else if (pcb->sched.status == PROC_STATUS_RUNNING && priority > prev_prio) {
        // If the process is running and it now has a lower priority (i.e. higher number), then re-schedule.
        // There may be now a higher priority ready process
        _laritos.sched.need_sched = true;
    }

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    return 0;
}

void process_exit(int exit_status) {
    pcb_t *pcb = process_get_current();
    irqctx_t pcbdatalock_ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &pcbdatalock_ctx);
    pcb->exit_status = exit_status;
    spinlock_release(&_laritos.proc.pcbs_data_lock, &pcbdatalock_ctx);

    debug_async("Exiting process pid=%u, exitcode=%d", pcb->pid, pcb->exit_status);

    vfs_file_close_all_for_cur_process();
    process_kill_and_schedule(pcb);
}

static void kernel_main_wrapper(kproc_main_t main, void *data) {
    verbose_async("Starting kernel_main_wrapper(main=0x%p, data=0x%p)", main, data);
    int status = main(data);
    process_exit(status);
    // Execution will never reach this point
}

pcb_t *process_spawn_kernel_process(char *name, kproc_main_t main, void *data, uint32_t stacksize, uint8_t priority) {
    debug_async("Spawning kernel process (name=%-7.7s, main=0x%p, data=0x%p, prio=%u)", name, main, data, priority);

    // Allocate PCB structure for this new process
    pcb_t *pcb = process_alloc();
    if (pcb == NULL) {
        error_async("Could not allocate space for PCB");
        goto error_pcb;
    }

    process_set_name(pcb, name);
    pcb->kernel = true;
    strncpy(pcb->cmd, "<kernel>", sizeof(pcb->cmd));

    verbose_async("Allocating %lu bytes for kernel process stack", stacksize);
    // Since we only allocate space for the stack (for kernel processes), imgaddr is
    // just a pointer to the stack
    if ((pcb->mm.imgaddr = malloc(stacksize)) == NULL) {
        error_async("Couldn't allocate memory for kernel process at 0x%p", pcb);
        goto error_imgalloc;
    }
    pcb->mm.imgsize = stacksize;

    pcb->mm.text_start = (void *) __text_start;
    pcb->mm.text_size = (secsize_t) __text_size;
    pcb->mm.data_start = (void *) __data_start;
    pcb->mm.data_size = (secsize_t) __data_size;
    pcb->mm.bss_start = (void *) __bss_start;
    pcb->mm.bss_size = (secsize_t) __bss_size;
    pcb->mm.heap_start = (void *) __heap_start;
    pcb->mm.heap_size = (secsize_t) ((char *) __heap_end - (char *) __heap_start);

    pcb->mm.stack_bottom = pcb->mm.imgaddr;
    pcb->mm.stack_size = stacksize;
    pcb->mm.stack_top = ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size - 4);
    pcb->mm.sp_ctx = (spctx_t *) ((char *) pcb->mm.stack_top - 4);

    // Setup the stack protector
    spprot_setup(pcb);

    debug_async("Process image: 0x%p-0x%p", pcb->mm.imgaddr, (char *) pcb->mm.imgaddr + pcb->mm.imgsize);
    debug_async("text:          0x%p-0x%p", pcb->mm.text_start, (char *) pcb->mm.text_start + pcb->mm.text_size);
    debug_async("stack:         0x%p-0x%p", pcb->mm.stack_bottom, (char *) pcb->mm.stack_bottom + pcb->mm.stack_size);

    context_init(pcb, kernel_main_wrapper, CPU_MODE_SUPERVISOR);

    // Set arguments for the kernel_main_wrapper
    arch_context_set_first_arg(pcb->mm.sp_ctx, main);
    arch_context_set_second_arg(pcb->mm.sp_ctx, data);

    process_set_priority(pcb, priority);

    if (process_register(pcb) < 0) {
        error_async("Could not register process loaded at 0x%p", pcb);
        goto error_pcbreg;
    }

    return pcb;

error_pcbreg:
    free(pcb->mm.imgaddr);
    pcb->mm.imgaddr = NULL;
error_imgalloc:
    process_free(pcb);
error_pcb:
    return NULL;
}

int process_wait_for(pcb_t *pcb, int *status) {
    verbose_async("Waiting for pid=%u", pcb->pid);

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proc.pcbs_data_lock, &ctx);

    if (pcb->parent != process_get_current()) {
        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
        return -1;
    }

    if (pcb->sched.status == PROC_STATUS_ZOMBIE) {
        handle_dead_child_locked(pcb, status);
        spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);
        return 0;
    }

    BLOCK_UNTIL(pcb->sched.status == PROC_STATUS_ZOMBIE, &pcb->parent_waiting_cond,
                &_laritos.proc.pcbs_data_lock, &ctx);
    handle_dead_child_locked(pcb, status);

    spinlock_release(&_laritos.proc.pcbs_data_lock, &ctx);

    return 0;
}

int process_wait_pid(uint16_t pid, int *status) {
    pcb_t *pcb = slab_get_ptr_from_position(_laritos.proc.pcb_slab, pid);
    if (pcb == NULL) {
        error_async("Invalid pid=%u", pid);
        return -1;
    }
    return process_wait_for(pcb, status);
}

uint32_t process_get_avail_stack_locked(pcb_t *pcb) {
    char *sp = pcb->sched.status == PROC_STATUS_RUNNING ? arch_cpu_get_sp() : pcb->mm.sp_ctx;
    return sp - (char *) pcb->mm.stack_bottom;
}



#ifdef CONFIG_TEST_CORE_PROCESS_CORE
#include __FILE__
#endif
