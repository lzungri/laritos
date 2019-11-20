#include <log.h>

#include <stdbool.h>
#include <component/intc.h>
#include <irq.h>
#include <cpu.h>
#include <board-types.h>
#include <board.h>
#include <driver/driver.h>
#include <driver/gicv2.h>
#include <utils/math.h>
#include <mm/heap.h>
#include <generated/autoconf.h>


#define CHECK_IRQ_NUMBER(_irq) \
    if (_irq > min(gic->num_irqs, CONFIG_INT_MAX_IRQS)) { \
        error("Invalid irq %u, max_supported: %u", _irq, min(gic->num_irqs, CONFIG_INT_MAX_IRQS)); \
        return -1; \
    }

static int set_irq_enable(intc_t *intc, irq_t irq, bool enabled){
    verbose("Setting irq %u enabled state to %u", irq, enabled);
    gic_t *gic = (gic_t *) intc;
    CHECK_IRQ_NUMBER(irq);
    if (enabled) {
        // Note: Writing zeros has no effect on the other irqs (there is another register
        // for clearing irqs)
        gic->dist->set_enable[irq >> 3] = 1 << irq % 8;
    } else {
        // Note: Writing zeros has no effect on the other irqs (there is another register
        // for enabling irqs)
        gic->dist->clear_enable[irq >> 3] = 1 << irq % 8;
    }
    return 0;
}

static int set_irq_trigger_mode(intc_t *intc, irq_t irq, irq_trigger_mode_t mode) {
    verbose("Set irq %u trigger mode to %u", irq, mode);
    gic_t *gic = (gic_t *) intc;
    CHECK_IRQ_NUMBER(irq);

    // ARM GIC v2 only supports edge and level configuration (no high <-> low granularity)
    uint8_t m = (mode == IRQ_TRIGGER_EDGE_HIGH_LOW || mode == IRQ_TRIGGER_EDGE_LOW_HIGH) ? 1 : 0;
    uint8_t current = gic->dist->cfg[irq >> 2];
    uint8_t irq_subpos = (irq % 4) * 2;
    gic->dist->cfg[irq >> 2] = (current & ~(2 << irq_subpos)) | (m * 2 << irq_subpos);
    return 0;
}

static int set_irq_target_cpus(intc_t *intc, irq_t irq, cpubits_t bits) {
    verbose("Set irq %u target cpus to 0x%lx", irq, bits);
    gic_t *gic = (gic_t *) intc;
    CHECK_IRQ_NUMBER(irq);
    gic->dist->targets[irq] = (uint8_t) bits;
    return 0;
}

static int set_irqs_enable_for_this_cpu(intc_t *intc, bool enabled) {
    verbose("Setting interrupts signaling to cpu %d to %u", cpu_get_id(), enabled);
    gic_t *gic = (gic_t *) intc;
    gic->cpu->ctrl.b.enable_group0 = enabled;
    gic->cpu->ctrl.b.enable_group1 = enabled;
    return 0;
}

static int set_priority_filter(intc_t *intc, uint8_t lowest_prio) {
    verbose("Set priority filter to %u", lowest_prio);
    gic_t *gic = (gic_t *) intc;
    gic->cpu->prio_mask.b.priority = lowest_prio;
    return 0;
}

static irqret_t dispatch_irq(intc_t *intc) {
    gic_t *gic = (gic_t *) intc;

    // Acknowledge the GIC (pending->active) and grab the irq id
    gic_cpu_int_ack_t ack = gic->cpu->int_ack;
    if (ack.b.id == GICV2_SPURIOUS_INT_ID) {
        // No pending interrupt right now or the int was raised by a
        // different interrupt controller
        return IRQ_RET_NOT_HANDLED;
    }

    irqret_t ret = intc->ops.handle_irq(intc, (irq_t) ack.b.id);
    if (ret == IRQ_RET_ERROR) {
        error_async("Error while handling irq %u", ack.b.id);
    }

    // Notify the GIC about the completion of the interrupt (active->inactive)
    gic->cpu->end_int = ack;

    return ret;
}

static int init(component_t *c) {
    gic_t *gic = (gic_t *) c;

    // Check gic architecture version
    if (gic->dist->perip_id2.b.arch_rev != ARCH_REV_GICV2) {
        error("Invalid GIC architecture revision, expected gicv2");
        return -1;
    }

    gic->num_irqs = (gic->dist->type.b.nlines + 1) * 32;
    if (gic->num_irqs == 0) {
        error("Invalid number of supported irqs");
        return -1;
    }
    debug("Max number of supported IRQs by GIC: %u", gic->num_irqs);

    /**
     * From GIC specification:
     *  On power-up, or after a reset, a GIC implementation that supports interrupt
     *  grouping is configured with:
     *      * all interrupts assigned to Group 0
     *      * the FIQ exception request disabled
     */
    info("Enabling global interrupts for groups 0 and 1");
    gic->dist->ctrl.b.enable_group0 = 1;
    gic->dist->ctrl.b.enable_group1 = 1;

    info("Disabling irq priority filtering (prio=0xff)");
    gic->parent.ops.set_priority_filter((intc_t *) gic, 0xff);

    return 0;
}

static int deinit(component_t *c) {
    gic_t *gic = (gic_t *) c;

    info("Resetting irq priority filtering (prio=0)");
    gic->parent.ops.set_priority_filter((intc_t *) gic, 0);

    info("Disabling global interrupts for groups 0 and 1");
    gic->dist->ctrl.b.enable_group0 = 0;
    gic->dist->ctrl.b.enable_group1 = 0;

    return 0;
}

static int process(board_comp_t *comp) {
    gic_t *gic = component_alloc(sizeof(gic_t));
    if (gic == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }
    intc_t *intc = (intc_t *) gic;

    if (intc_component_init(intc, comp->id, comp, init, deinit) < 0) {
        error("Failed to initialize gic '%s'", comp->id);
        goto fail;
    }
    intc->ops.dispatch_irq = dispatch_irq;
    intc->ops.set_irq_enable = set_irq_enable;
    intc->ops.set_irq_trigger_mode = set_irq_trigger_mode;
    intc->ops.set_irq_target_cpus = set_irq_target_cpus;
    intc->ops.set_irqs_enable_for_this_cpu = set_irqs_enable_for_this_cpu;
    intc->ops.set_priority_filter = set_priority_filter;

    component_set_info((component_t *) intc, "GICv2", "ARM", "Generic Interrupt Controller v2");

    board_get_ptr_attr_def(comp, "distaddr", (void **) &gic->dist, NULL);
    if (gic->dist == NULL) {
        error("No distributor address was specified in the board information for '%s'", comp->id);
        goto fail;
    }

    board_get_ptr_attr_def(comp, "cpuaddr", (void **) &gic->cpu, NULL);
    if (gic->cpu == NULL) {
        error("No cpu address was specified in the board information for '%s'", comp->id);
        goto fail;
    }

    if (component_register((component_t *) gic) < 0) {
        error("Couldn't register gic '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(cpu);
    return -1;
}

DEF_DRIVER_MANAGER(gicv2, process);
