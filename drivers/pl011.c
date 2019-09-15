#define DEBUG
#include <log.h>
#include <driver.h>
#include <uart.h>
#include <drivers/pl011.h>
#include <stream.h>
#include <utils.h>
#include <irq.h>
#include <board-types.h>

#define MAX_UARTS 3

// TODO Use dynamic memory instead
static uart_t uarts[MAX_UARTS];
static uint8_t cur_uart;


static int write(stream_t *s, const void *buf, size_t n) {
    uart_t *uart = container_of(s, uart_t, stream);
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    // TODO Use interrupts
    if (s->blocking) {
        // Wait until there is space in the FIFO
        while (pl011->fr.b.txff);
    } else if (pl011->fr.b.txff) {
        // No space left and non-blocking io mode, return
        return 0;
    }

    const uint8_t *data = buf;
    int i;
    for (i = 0; i < n; i++) {
        pl011->dr = data[i];
    }
    return n;
}

static int read(stream_t *s, void *buf, size_t n) {
    uart_t *uart = container_of(s, uart_t, stream);
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;



//    if (uart->intio) {
//
//    }




    // TODO Use interrupts
    if (s->blocking) {
        // Wait until there is some data in the FIFO
        while (pl011->fr.b.rxfe);
    }

    int i;
    uint8_t *data = buf;
    // Read as long as there is some data in the FIFO and the amount read is < n
    for (i = 0; i < n && !pl011->fr.b.rxfe; i++) {
        // Only the last byte contains the actual data
        data[i] = pl011->dr & 0xff;
    }
    return i;
}

static irqret_t irq_handler(irq_t irq, void *data) {
    uart_t *uart = (uart_t *) data;
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    // Check whether this is a received data interrupt (it should, since that
    // is the only enabled irq)
    if (pl011->mis.b.rxim) {
        verbose_sync(false, "UART data received irq");

        // TODO

        // Clear rx interrupt
        pl011->icr.b.rxim = 1;
    }

    return IRQ_RET_HANDLED;
}

static int init(component_t *c) {
    if (uart_init(c) < 0) {
        error("Failed to initialize uart for component '%s'", c->id);
        return -1;
    }

    uart_t *uart = (uart_t *) c;
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    // Disable interrupts
    pl011->imsc.v = 0;
    // Clear flagged interrupts
    pl011->icr.v = 0xffff;

    if (uart->intio) {
        // Enable Receive interrupt. Since FIFO is disabled by default, we
        // will get an int for every input char
        pl011->imsc.b.rxim = 1;
    }

    return 0;
}

static int deinit(component_t *c) {
    uart_t *uart = (uart_t *) c;
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;
    // Disable interrupts
    pl011->imsc.v = 0;
    // Clear flagged interrupts
    pl011->icr.v = 0xffff;

    return uart_deinit(c);
}

static int process(board_comp_t *comp) {
    if (cur_uart > ARRAYSIZE(uarts)) {
        error("Max number of uart components reached");
        return -1;
    }
    uart_t *uart = &uarts[cur_uart];
    uart->irq_handler = irq_handler;
    if (uart_component_init_and_register(uart, comp, init, deinit, read, write) < 0){
        error("Failed to register '%s'", comp->id);
        return -1;
    }
    cur_uart++;

    return 0;
}

DEF_DRIVER_MANAGER(pl011, process);
