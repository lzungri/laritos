#define DEBUG
#include <log.h>

#include <process/core.h>
#include <arch/cpu.h>

int idle_main(void *data) {
    while (1) {
        info("IDLE");
        arch_wfi();
    }
    return 0;
}
