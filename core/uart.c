#include <log.h>
#include <uart.h>
#include <component.h>
#include <board.h>


int uart_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(uart_t *uart, void *buf, size_t n), int (*write)(uart_t *uart, const void *buf, size_t n)) {

    if (component_init((component_t *) uart, bcomp, COMP_TYPE_UART, init, deinit) < 0) {
        error("Failed to initialize '%s' UART component", bcomp->id);
        return -1;
    }
    uart->ops.read = read;
    uart->ops.write = write;

    return board_get_ptr_attr(bcomp, "baseaddr", &uart->baseaddr, NULL);
}

int uart_register(uart_t *uart) {
    return component_register((component_t *) uart);
}
