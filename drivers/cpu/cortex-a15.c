#include <log.h>

#include <cpu.h>
#include <component/cpu.h>
#include <driver/driver.h>
#include <component/intc.h>

#define MAX_CPUS 4

// TODO Use dynamic memory instead
static cpu_t cpus[MAX_CPUS];
static uint8_t curcpu;


static int set_irqs_enable(cpu_t *c, bool enabled) {
    if (cpu_get_id() != c->id) {
        error("Cannot enable/disable irqs on behalf of other processor");
        return -1;
    }
    return c->intc->ops.set_irqs_enable_for_this_cpu(c->intc, enabled);
}

static int process(board_comp_t *comp) {
    if (curcpu > ARRAYSIZE(cpus)) {
        error("Max number of cpu components reached");
        return -1;
    }

    cpu_t *cpu = &cpus[curcpu];
    if (cpu_component_init(cpu, comp, NULL, NULL) < 0){
        error("Failed to register '%s'", comp->id);
        return -1;
    }
    cpu->ops.set_irqs_enable = set_irqs_enable;

    component_set_info((component_t *) cpu, "Cortex-A15", "ARM", "Cortex-A15 armv7-a processor");

    if (component_register((component_t *) cpu) < 0) {
        error("Couldn't register cpu '%s'", comp->id);
        return -1;
    }

    curcpu++;

    return 0;
}

DEF_DRIVER_MANAGER(cortex_a15, process);
