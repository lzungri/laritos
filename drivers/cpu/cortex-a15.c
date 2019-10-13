#include <log.h>

#include <cpu.h>
#include <component/cpu.h>
#include <driver/driver.h>
#include <component/intc.h>
#include <mm/heap.h>

static int set_irqs_enable(cpu_t *c, bool enabled) {
    if (cpu_get_id() != c->id) {
        error("Cannot enable/disable irqs on behalf of other processor");
        return -1;
    }
    return c->intc->ops.set_irqs_enable_for_this_cpu(c->intc, enabled);
}

static int process(board_comp_t *comp) {
    cpu_t *cpu = component_alloc(sizeof(cpu_t));
    if (cpu == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (cpu_component_init(cpu, comp, NULL, NULL) < 0){
        error("Failed to register '%s'", comp->id);
        goto fail;
    }
    cpu->ops.set_irqs_enable = set_irqs_enable;

    component_set_info((component_t *) cpu, "Cortex-A15", "ARM", "Cortex-A15 armv7-a processor");

    if (component_register((component_t *) cpu) < 0) {
        error("Couldn't register cpu '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(cpu);
    return -1;
}

DEF_DRIVER_MANAGER(cortex_a15, process);
