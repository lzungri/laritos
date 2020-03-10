#include <log.h>

#include <process/types.h>
#include <process/core.h>
#include <arch/cpu.h>
#include <generated/autoconf.h>

static int idle_main(void *data) {
    while (1) {
        insane("IDLE");
        arch_cpu_wfi();
    }
    return 0;
}

pcb_t *idle_launcher(void) {
    return process_spawn_kernel_process("idle", idle_main, NULL,
            CONFIG_PROCESS_IDLE_STACK_SIZE, CONFIG_SCHED_PRIORITY_LOWEST);
}
