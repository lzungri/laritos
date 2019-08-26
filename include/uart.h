#pragma once

#include <stddef.h>
#include <component.h>

struct uart;
typedef struct {
    int (*write)(struct uart *uart, const void *buf, size_t n);
    int (*read)(struct uart *uart, void *buf, size_t n);
} uart_ops_t;

typedef struct uart {
    component_t parent;

    void *baseaddr;
    uart_ops_t ops;
} uart_t;


int uart_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(uart_t *uart, void *buf, size_t n), int (*write)(uart_t *uart, const void *buf, size_t n));
int uart_register(uart_t *uart);
