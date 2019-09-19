#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <irq.h>
#include <circbuf.h>
#include <component/component.h>
#include <component/hwcomp.h>
#include <component/intc.h>
#include <component/stream.h>

#include <generated/autoconf.h>

typedef struct uart {
    hwcomp_t parent;

    void *baseaddr;

    irq_t irq;
    bool intio;
    irq_trigger_mode_t irq_trigger;
    intc_t *intc;
    irq_handler_t irq_handler;
    uint8_t rxbuf[CONFIG_UART_BUF_SIZE];
    circbuf_t cb;

    stream_t stream;
} uart_t;

int uart_init(component_t *c);
int uart_deinit(component_t *c);

int uart_component_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n));
int uart_component_register(uart_t *uart);
int uart_component_init_and_register(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*read)(stream_t *s, void *buf, size_t n), int (*write)(stream_t *s, const void *buf, size_t n));
