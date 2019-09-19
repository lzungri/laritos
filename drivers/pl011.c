#include <log.h>
#include <driver.h>
#include <uart.h>
#include <hwcomp.h>
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

    if (s->blocking) {
        // Wait until there is space in the FIFO
        while (pl011->fr.b.txff) {
            if (uart->intio) {
                // Enable Transmit interrupt to detect FIFO space availability
                pl011->imsc.b.txim = 1;
                // TODO Replace with a nice synchro mechanism
                // Any interrupt will wake the cpu up
                asm("wfi");
            }
        }
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

static int readall_into_circbuf(uart_t *uart) {
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;
    // Read as long as there is some data in the FIFO
    while (!pl011->fr.b.rxfe) {
        // Only the last byte contains the actual data
        uint8_t data = (uint8_t) (pl011->dr & 0xff);
        if (circbuf_write(&uart->cb, &data, sizeof(data)) <= 0) {
            error_sync(false, "Couldn't write '%c' into uart circular buffer", data);
            return -1;
        }
    }
    return 0;
}

static int read(stream_t *s, void *buf, size_t n) {
    uart_t *uart = container_of(s, uart_t, stream);
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    // If using interrupt-driven io, read from circular buffer
    if (uart->intio) {
        if (s->blocking) {
            // Wait until there is some data in the buffer
            while (uart->cb.datalen == 0) {
                // TODO Replace with a nice synchro mechanism
                // Any interrupt (rx int is enabled) will wake the cpu up
                asm("wfi");
            }
        }
        return circbuf_read(&uart->cb, buf, n);
    }

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

    // Check whether this is a receive int
    if (pl011->mis.b.rxim) {
        verbose_sync(false, "UART data received irq");
        if (readall_into_circbuf(uart) < 0) {
            error_sync(false, "Couldn't read data");
            return IRQ_RET_ERROR;
        }
        // Clear rx interrupt
        pl011->icr.b.rxim = 1;
    }

    // Check whether this is a transmit int
    if (pl011->mis.b.txim) {
        verbose_sync(false, "UART data transmitted irq");

        // Disable tx interrupt. It will be enabled when FIFO is full and we
        // want to wait until it becomes available for writing
        pl011->imsc.b.txim = 0;
        // Clear tx interrupt
        pl011->icr.b.txim = 1;
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
    hwcomp_set_info((hwcomp_t *) uart, "PrimeCell UART (pl011)", "ARM", "AMBA compliant SoC");
    cur_uart++;

    return 0;
}

DEF_DRIVER_MANAGER(pl011, process);
