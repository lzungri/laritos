#define DEBUG
#include <log.h>
#include <driver.h>
#include <board.h>
#include <intc.h>
#include <irq.h>
#include <cpu.h>
#include <drivers/gicv2.h>

#define MAX_GICS 1

// TODO Use dynamic memory instead
static gic_t gics[MAX_GICS];
static uint8_t cur_gic;


#define CHECK_IRQ_NUMBER(_irq) \
    if (_irq > gic->num_irqs) { \
        error("Invalid irq %u, max_supported: %u", _irq, gic->num_irqs); \
        return -1; \
    }

static int enable_irq(struct intc *intc, irq_t irq){
    verbose("Enable irq %u", irq);
    gic_t *gic = (gic_t *) intc;
    CHECK_IRQ_NUMBER(irq);
    // Note: Writing zeros has no effect on the other irqs (there is another register
    // for clearing irqs)
    gic->dist->set_enable[irq >> 3] = 1 << irq % 8;
    return 0;
}

static int disable_irq(struct intc *intc, irq_t irq) {
    verbose("Disable irq %u", irq);
    gic_t *gic = (gic_t *) intc;
    CHECK_IRQ_NUMBER(irq);
    // Note: Writing zeros has no effect on the other irqs (there is another register
    // for enabling irqs)
    gic->dist->clear_enable[irq >> 3] = 1 << irq % 8;
    return 0;
}

static int set_irq_trigger_mode(struct intc *intc, irq_t irq, irq_trigger_mode_t mode) {
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

static int set_irq_target_cpus(struct intc *intc, irq_t irq, cpubits_t bits) {
    verbose("Set irq %u target cpus to 0x%lx", irq, bits);
    gic_t *gic = (gic_t *) intc;
    CHECK_IRQ_NUMBER(irq);
    gic->dist->targets[irq] = (uint8_t) bits;
    return 0;
}

static int enable_int_signaling_to_this_cpu(struct intc *intc) {
    verbose("Enable interrupt signaling to this (=%d) cpu", get_cpu_id());
    gic_t *gic = (gic_t *) intc;
    gic->cpu->ctrl.b.enable_group0 = 1;
    gic->cpu->ctrl.b.enable_group1 = 1;
    return 0;
}

static int set_priority_filter(struct intc *intc, uint8_t lowest_prio) {
    verbose("Set priority filter to %u", lowest_prio);
    gic_t *gic = (gic_t *) intc;
    gic->cpu->prio_mask.b.priority = lowest_prio;
    return 0;
}

static irqret_t dispatch_irq(struct intc *intc) {
    if (intc->parent.stype != COMP_SUBTYPE_GIC) {
        return IRQ_RET_NOT_HANDLED;
    }

    gic_t *gic = (gic_t *) intc;

    // Acknowledge the GIC (pending->active) and grab the irq id
    gic_cpu_int_ack_t ack = gic->cpu->int_ack;

    irqret_t ret = intc->ops.handle_irq(intc, (irq_t) ack.b.id);
    if (ret == IRQ_RET_ERROR) {
        error_sync(false, "Error while handling irq %u", ack.b.id);
    }

    // Notify the GIC about the completion of the interrupt (active->inactive)
    gic->cpu->end_int = ack;

    return ret;
}

static int process(board_comp_t *comp) {
    if (cur_gic > ARRAYSIZE(gics)) {
        error("Max number of GICs components reached");
        return -1;
    }

    gic_t *gic = &gics[cur_gic];
    intc_t *intc = (intc_t *) gic;

    if (intc_init(intc, comp->id, comp, NULL, NULL) < 0) {
        error("Failed to initialize gic '%s'", comp->id);
        return -1;
    }
    intc->parent.stype = COMP_SUBTYPE_GIC;
    intc->ops.dispatch_irq = dispatch_irq;
    intc->ops.enable_irq = enable_irq;
    intc->ops.disable_irq = disable_irq;
    intc->ops.set_irq_trigger_mode = set_irq_trigger_mode;
    intc->ops.set_irq_target_cpus = set_irq_target_cpus;
    intc->ops.enable_int_signaling_to_this_cpu = enable_int_signaling_to_this_cpu;
    intc->ops.set_priority_filter = set_priority_filter;

    board_get_ptr_attr(comp, "distaddr", (void **) &gic->dist, NULL);
    if (gic->dist == NULL) {
        error("No distributor address was specified in the board information for '%s'", comp->id);
        return -1;
    }

    board_get_ptr_attr(comp, "cpuaddr", (void **) &gic->cpu, NULL);
    if (gic->cpu == NULL) {
        error("No cpu address was specified in the board information for '%s'", comp->id);
        return -1;
    }

    gic->num_irqs = (gic->dist->type.b.nlines + 1) * 32;
    if (gic->num_irqs == 0) {
        error("Invalid number of supported irqs");
        return -1;
    }
    debug("Max number of supported IRQs: %u", gic->num_irqs);

    info("Enabling global interrupts for groups 0 and 1");
    gic->dist->ctrl.b.enable_group0 = 1;
    gic->dist->ctrl.b.enable_group1 = 1;

    info("Disabling irq priority filtering (prio=0xff)");
    intc->ops.set_priority_filter(intc, 0xff);

    if (component_register((component_t *) gic) < 0) {
        error("Couldn't register gic '%s'", comp->id);
        return -1;
    }
    cur_gic++;

    return 0;
}

DEF_DRIVER_MANAGER(gicv2, process);
