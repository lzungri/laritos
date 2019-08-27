#include <log.h>
#include <uart.h>
#include <component.h>
#include <board.h>
#include <chardev.h>


int uart_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n)) {

    if (chardev_init((chardev_t *) uart, bcomp, COMP_TYPE_UART, init, deinit, read, write) < 0) {
        error("Failed to initialize '%s' UART component", bcomp->id);
        return -1;
    }
    return board_get_ptr_attr(bcomp, "baseaddr", &uart->baseaddr, NULL);
}

int uart_register(uart_t *uart) {
    return chardev_register((chardev_t *) uart);
}
