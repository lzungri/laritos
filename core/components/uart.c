#include <log.h>
#include <uart.h>
#include <component.h>
#include <hwcomp.h>
#include <stream.h>
#include <stdbool.h>
#include <irq.h>
#include <intc.h>
#include <cpu.h>
#include <board-types.h>
#include <board.h>
#include <circbuf.h>
#include <utils.h>


int uart_init(component_t *c) {
    uart_t *uart = (uart_t *) c;

    circbuf_init(&uart->cb, uart->rxbuf, ARRAYSIZE(uart->rxbuf));

    // Setup irq stuff if using interrupt-driven io
    if (uart->intio) {
        if (intc_enable_irq_with_handler(uart->intc,
                uart->irq, uart->irq_trigger, uart->irq_handler, c) < 0) {
            error("Failed to enable irq %u with handler 0x%p", uart->irq, uart->irq_handler);
            return -1;
        }
    }
    return 0;
}

int uart_deinit(component_t *c) {
    uart_t *uart = (uart_t *) c;
    return uart->intio ?
            intc_disable_irq_with_handler(uart->intc, uart->irq, uart->irq_handler) :
            0;
}

int uart_component_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n)) {
    if (hwcomp_init((hwcomp_t *) uart, bcomp->id, bcomp, COMP_SUBTYPE_UART, init, deinit) < 0) {
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

        if (board_get_component_attr(bcomp, "intc", (component_t **) &uart->intc) < 0) {
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
        error("Failed to register '%s' uart stream", uart->parent.parent.id);
        return -1;
    }

    if (component_register((component_t *) uart) < 0) {
        error("Failed to register '%s' uart component", uart->parent.parent.id);
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
