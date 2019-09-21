#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <irq.h>
#include <component/component.h>
#include <component/hwcomp.h>
#include <component/intc.h>
#include <component/bytestream.h>
#include <dstruct/circbuf.h>
#include <generated/autoconf.h>

typedef struct uart {
    hwcomp_t parent;

    void *baseaddr;

    irq_t irq;
    bool intio;
    irq_trigger_mode_t irq_trigger;
    intc_t *intc;
    irq_handler_t irq_handler;
    uint8_t rxbuf[CONFIG_UART_RXBUF_SIZE];
    uint8_t txbuf[CONFIG_UART_TXBUF_SIZE];

    bytestream_t bs;
} uart_t;

int uart_init(component_t *c);
int uart_deinit(component_t *c);

int uart_component_init(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*transmit)(bytestream_t *s));
int uart_component_register(uart_t *uart);
int uart_component_init_and_register(uart_t *uart, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c),
        int (*transmit_data)(bytestream_t *s));
