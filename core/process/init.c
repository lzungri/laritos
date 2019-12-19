#include <log.h>

#include <process/core.h>
#include <arch/cpu.h>
#include <utils/assert.h>
#include <loader/loader.h>
#include <generated/autoconf.h>

#ifdef CONFIG_TEST_ENABLED
#include <test/test.h>
#endif

int init_main(void *data) {
    debug("Executing init process");

    info("Spawning idle process");
    int idle_main(void *data);
    pcb_t *idle = process_spawn_kernel_process("idle", idle_main, NULL,
                        CONFIG_PROCESS_IDLE_STACK_SIZE, CONFIG_SCHED_PRIORITY_LOWEST);
    assert(idle != NULL, "Could not create idle process");

#ifdef CONFIG_TEST_ENABLED
    log_always("***** Running in test mode *****");
    info("Spawning test process");
    pcb_t *test = process_spawn_kernel_process("test", test_main, __tests_start,
                        CONFIG_PROCESS_TEST_STACK_SIZE, CONFIG_SCHED_PRIORITY_MAX_USER - 2);
    assert(test != NULL, "Could not create TEST process");
#else
    // Launch a few processes for testing
    // TODO: This code will disappear once we implement a shell and file system

    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #0");
    }

    // Load the same program, just to have 2 processes for testing
    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #1");
    }

    // Load the same program, just to have 3 processes for testing
    if (loader_load_executable_from_memory(0) == NULL) {
        error("Failed to load app #2");
    }

    info("Spawning shell process");
    int shell_main(void *data);
    pcb_t *shell = process_spawn_kernel_process("shell", shell_main, NULL,
                        8196, CONFIG_SCHED_PRIORITY_LOWEST - 10);
    assert(shell != NULL, "Could not create shell process");
#endif

    // Wait forever
    while (1) {
        arch_wfi();

        pcb_t *child;
        pcb_t *temp;
        for_each_child_process_safe(process_get_current(), child, temp) {
            if (child->sched.status == PROC_STATUS_ZOMBIE) {
                verbose("Unregistering dead child process pid=%u", child->pid);
                process_unregister(child);
            }
        }
    }
    return 0;
}
