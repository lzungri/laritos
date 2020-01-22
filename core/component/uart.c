#include <log.h>
#include <stdbool.h>
#include <irq/irq.h>
#include <board/board-types.h>
#include <board/board.h>
#include <component/component.h>
#include <component/intc.h>
#include <component/bytestream.h>
#include <component/uart.h>
#include <dstruct/circbuf.h>
#include <utils/utils.h>


int uart_init(uart_t *uart) {
    // Setup irq stuff if using interrupt-driven io
    if (uart->intio) {
        if (intc_enable_irq_with_handler(uart->intc,
                uart->irq, uart->irq_trigger, uart->irq_handler, uart) < 0) {
            error("Failed to enable irq %u with handler 0x%p", uart->irq, uart->irq_handler);
            return -1;
        }
    }
    return 0;
}

int uart_deinit(uart_t *uart) {
    if (uart->intio) {
        return intc_disable_irq_with_handler(uart->intc, uart->irq, uart->irq_handler);
    }
    return 0;
}

int uart_component_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*transmit)(bytestream_t *bs)) {
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

        if (board_get_component_attr(bcomp, "intc", (component_t **) &uart->intc) < 0) {
            error("invalid or no interrupt controller specified in the board info");
            return -1;
        }
    }

    // Initialize a bytestream device to read/write the uart device
    if (bytestream_component_init(&uart->bs, bcomp,
            uart->rxbuf, sizeof(uart->rxbuf),
            uart->txbuf, sizeof(uart->txbuf),
            transmit) < 0) {
        error("Failed to initialize uart bytestream");
        return -1;
    }
    return 0;
}

int uart_component_register(uart_t *uart) {
    if (component_register((component_t *) &uart->bs) < 0) {
        error("Failed to register '%s' uart bytestream", uart->parent.id);
        return -1;
    }

    if (component_register((component_t *) uart) < 0) {
        error("Failed to register '%s' uart component", uart->parent.id);
        component_unregister((component_t *) &uart->bs);
        return -1;
    }

    return 0;
}

int uart_component_init_and_register(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*transmit_data)(bytestream_t *s)) {
    if (uart_component_init(uart, bcomp, init, deinit, transmit_data) < 0){
        error("Failed to initialize '%s'", bcomp->id);
        return -1;
    }

    if (uart_component_register(uart) < 0) {
        error("Couldn't register uart '%s'", bcomp->id);
        return -1;
    }
    return 0;
}
