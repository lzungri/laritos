#include <log.h>
#include <uart.h>
#include <component.h>
#include <board.h>
#include <chardev.h>

int uart_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n)) {
    if (component_init((component_t *) uart, bcomp->id, bcomp, COMP_TYPE_UART, init, deinit) < 0) {
        error("Failed to initialize '%s' uart component", bcomp->id);
        return -1;
    }
    board_get_ptr_attr(bcomp, "baseaddr", &uart->baseaddr, NULL);
    if (uart->baseaddr == NULL) {
        error("No baseaddr was specified in the board information");
        return -1;
    }

    // Initialize a character device to read/write the uart device
    if (chardev_init((chardev_t *) &uart->cdev, bcomp, NULL, NULL, read, write) < 0) {
        error("Failed to initialize uart chardev");
        return -1;
    }
    return 0;
}

int uart_register(uart_t *uart) {
    if (component_register((component_t *) &uart->cdev) < 0) {
        error("Failed to register '%s' uart chardev", uart->parent.id);
        return -1;
    }

    if (component_register((component_t *) uart) < 0) {
        error("Failed to register '%s' uart component", uart->parent.id);
        component_unregister((component_t *) &uart->cdev);
        return -1;
    }

    return 0;
}

int uart_init_and_register(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n)) {
    if (uart_init(uart, bcomp, NULL, NULL, read, write) < 0){
        error("Failed to initialize '%s'", bcomp->id);
        return -1;
    }

    if (uart_register(uart) < 0) {
        error("Couldn't register uart '%s'", bcomp->id);
        return -1;
    }
    return 0;
}
