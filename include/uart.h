#pragma once

#include <stddef.h>
#include <component.h>
#include <stdint.h>
#include <stdbool.h>
#include <stream.h>

typedef struct uart {
    component_t parent;

    void *baseaddr;
    // TODO Implement
    uint32_t baudrate;
    // TODO Implement
    bool fifo_enabled;
    stream_t stream;
} uart_t;


int uart_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n));
int uart_register(uart_t *uart);
int uart_init_and_register(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n));
