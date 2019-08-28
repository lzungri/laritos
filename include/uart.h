#pragma once

#include <stddef.h>
#include <component.h>
#include <chardev.h>

typedef struct uart {
    component_t parent;

    void *baseaddr;
    chardev_t cdev;
} uart_t;


int uart_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n));
int uart_register(uart_t *uart);
int uart_init_and_register(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(chardev_t *cd, void *buf, size_t n), int (*write)(chardev_t *cd, const void *buf, size_t n));
