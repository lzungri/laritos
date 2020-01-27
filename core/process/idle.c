#include <log.h>

#include <process/core.h>
#include <arch/cpu.h>

int idle_main(void *data) {
    while (1) {
        insane("IDLE");
        arch_cpu_wfi();
    }
    return 0;
}
