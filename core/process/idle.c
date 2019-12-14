#define DEBUG
#include <log.h>

#include <process/core.h>
#include <arch/cpu.h>

int idle_main(void *data) {
    while (1) {
        arch_wfi();
        info("IDLE");
    }
    return 0;
}
