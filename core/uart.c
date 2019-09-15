#include <log.h>
#include <uart.h>
#include <component.h>
#include <stream.h>
#include <stdbool.h>
#include <irq.h>
#include <intc.h>
#include <board-types.h>
#include <board.h>


int uart_init(component_t *c) {
    uart_t *uart = (uart_t *) c;
    intc_t *intc;
    if (uart->intio) {
        intc = uart->intc;
        intc->ops.set_irq_enable(intc, uart->irq, true);
        intc->ops.set_irq_trigger_mode(intc, uart->irq, uart->irq_trigger);
        if (intc->ops.set_irq_target_cpus(intc, uart->irq, BIT_FOR_CPU(0)) < 0) {
            error("Failed to set the irq targets");
            goto error_irq_enable;
        }
        if (intc->ops.set_irqs_enable_for_this_cpu(intc, true) < 0) {
            error("Failed to enable irqs for this cpu");
            goto error_target;
        }
        if (intc->ops.add_irq_handler(intc, uart->irq, uart->irq_handler, c) < 0) {
            error("Failed to add handler 0x%p for irq %u", uart->irq_handler, uart->irq);
            goto error_handler;
        }
    }

    return 0;

error_handler:
    intc->ops.set_irqs_enable_for_this_cpu(intc, false);
error_target:
    intc->ops.set_irq_target_cpus(intc, uart->irq, 0);
error_irq_enable:
    intc->ops.set_irq_enable(intc, uart->irq, false);
    return -1;
}

int uart_deinit(component_t *c) {
    uart_t *uart = (uart_t *) c;
    if (uart->intio) {
        intc_t *intc = uart->intc;
        intc->ops.remove_irq_handler(intc, uart->irq, uart->irq_handler);
        intc->ops.set_irqs_enable_for_this_cpu(intc, false);
        intc->ops.set_irq_target_cpus(intc, uart->irq, 0);
        intc->ops.set_irq_enable(intc, uart->irq, false);
    }
    return 0;
}

int uart_component_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n)) {
    if (component_init((component_t *) uart, bcomp->id, bcomp, COMP_TYPE_UART, init, deinit) < 0) {
        error("Failed to initialize '%s' uart component", bcomp->id);
        return -1;
    }
    board_get_ptr_attr_def(bcomp, "baseaddr", &uart->baseaddr, NULL);
    if (uart->baseaddr == NULL) {
        error("No baseaddr was specified in the board information");
        return -1;
    }

    board_get_bool_attr_def(bcomp, "intio", &uart->intio, false);
    if (uart->intio) {
        int irq;
        if (board_get_int_attr(bcomp, "irq", &irq) < 0 || irq < 0) {
            error("Invalid or no irq was specified in the board info");
            return -1;
        }
        uart->irq = irq;
        board_get_irq_trigger_attr_def(bcomp, "trigger", &uart->irq_trigger, IRQ_TRIGGER_LEVEL_HIGH);

        if (board_get_component_attr(bcomp, "intc", (component_t **) &uart->intc) < 0 || uart->intc == NULL) {
            error("invalid or no interrupt controller specified in the board info");
            return -1;
        }
    }

    // Initialize a stream device to read/write the uart device
    if (stream_component_init((stream_t *) &uart->stream, bcomp, NULL, NULL, read, write) < 0) {
        error("Failed to initialize uart stream");
        return -1;
    }
    return 0;
}

int uart_component_register(uart_t *uart) {
    if (component_register((component_t *) &uart->stream) < 0) {
        error("Failed to register '%s' uart stream", uart->parent.id);
        return -1;
    }

    if (component_register((component_t *) uart) < 0) {
        error("Failed to register '%s' uart component", uart->parent.id);
        component_unregister((component_t *) &uart->stream);
        return -1;
    }

    return 0;
}

int uart_component_init_and_register(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n)) {
    if (uart_component_init(uart, bcomp, init, deinit, read, write) < 0){
        error("Failed to initialize '%s'", bcomp->id);
        return -1;
    }

    if (uart_component_register(uart) < 0) {
        error("Couldn't register uart '%s'", bcomp->id);
        return -1;
    }
    return 0;
}
