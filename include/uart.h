#pragma once

#include <stddef.h>
#include <component.h>

typedef struct {
    int (*write)(const void *buf, size_t n);
    int (*read)(void *buf, size_t n);
} uart_ops_t;

typedef struct {
    component_t parent;

    void *baseaddr;
    uart_ops_t ops;
} uart_t;


int uart_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(void *buf, size_t n), int (*write)(const void *buf, size_t n));
int uart_register(uart_t *uart);
