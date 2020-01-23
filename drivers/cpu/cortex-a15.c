#include <log.h>

#include <core.h>
#include <cpu/cpu.h>
#include <board/board.h>
#include <driver/driver.h>
#include <mm/heap.h>
#include <component/intc.h>
#include <component/cpu.h>
#include <arch/cpu-types.h>

static int set_irqs_enable(cpu_t *c, bool enabled) {
    if (cpu_get_id() != c->id) {
        error("Cannot enable/disable irqs on behalf of other processor");
        return -1;
    }
    return c->intc->ops.set_irqs_enable_for_this_cpu(c->intc, enabled);
}

static int custom_initialization(cpu_t *c) {
    // Enable cpu cycle counter
    return cpu_set_cycle_count_enable(true);
}

static int process(board_comp_t *comp) {
    arm_cpu_t *cpu = component_alloc(sizeof(arm_cpu_t));
    if (cpu == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (cpu_component_init((cpu_t *) cpu, comp, NULL, NULL) < 0){
        error("Failed to register '%s'", comp->id);
        goto fail;
    }
    cpu->parent.ops.set_irqs_enable = set_irqs_enable;
    cpu->parent.ops.custom_initialization = custom_initialization;

    int irq;
    if (board_get_int_attr(comp, "pmu_irq", &irq) < 0 || irq < 0) {
        error("Invalid or no PMU irq was specified in the board info");
        goto fail;
    }
    cpu->pmu_irq = irq;
    board_get_irq_trigger_attr_def(comp, "pmu_trigger", &cpu->pmu_irq_trigger, IRQ_TRIGGER_LEVEL_HIGH);

    component_set_info((component_t *) cpu, "Cortex-A15", "ARM", "Cortex-A15 armv7-a processor");

    if (component_register((component_t *) cpu) < 0) {
        error("Couldn't register cpu '%s'", comp->id);
        goto fail;
    }

    // Save CPU shortcut
    _laritos.cpu[cpu->parent.id] = (cpu_t *) cpu;

    return 0;

fail:
    free(cpu);
    return -1;
}

DEF_DRIVER_MANAGER(cortex_a15, process);
