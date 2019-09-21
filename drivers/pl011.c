#include <log.h>
#include <component/uart.h>
#include <component/hwcomp.h>
#include <component/stream.h>
#include <irq.h>
#include <board-types.h>
#include <driver/driver.h>
#include <driver/pl011.h>
#include <utils/utils.h>

#define MAX_UARTS 3

// TODO Use dynamic memory instead
static uart_t uarts[MAX_UARTS];
static uint8_t cur_uart;


static int transmit_data(bytestream_t *bs) {
    uart_t *uart = container_of(bs, uart_t, bs);
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    if (pl011->fr.b.txff) {
        // Enable Transmit interrupt to detect FIFO space availability
        pl011->imsc.b.txim = 1;
        // No space left, return
        return 0;
    }

    uint8_t data;
    uint32_t sent = 0;
    // TODO Implement this using peek and check for txff flags as well
    while (circbuf_nb_read(&bs->txcb, &data, sizeof(data)) > 0) {
        pl011->dr = data;
        sent += sizeof(data);
    }
    return sent;
}

static inline int put_into_bytestream(uart_t *uart) {
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;
    // Read as long as there is some data in the FIFO
    while (!pl011->fr.b.rxfe) {
        // Only the last byte contains the actual data
        uint8_t data = (uint8_t) (pl011->dr & 0xff);
        if (uart->bs.ops._put(&uart->bs, &data, sizeof(data)) < sizeof(data)) {
            error_async("Couldn't write '%c' into uart bytestream", data);
            return -1;
        }
    }
    return 0;
}

static irqret_t irq_handler(irq_t irq, void *data) {
    uart_t *uart = (uart_t *) data;
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    // RECEIVE interrupt
    if (pl011->mis.b.rxim) {
        verbose_async("UART data received irq");
        if (put_into_bytestream(uart) < 0) {
            error_async("Couldn't read data");
            return IRQ_RET_ERROR;
        }
        // Clear rx interrupt
        pl011->icr.b.rxim = 1;
    }

    // TRANSMIT interrupt
    if (pl011->mis.b.txim) {
        verbose_async("UART data transmitted irq");

        // Disable tx interrupt. It will be enabled when FIFO is full and we
        // want to wait until it becomes available for writing
        pl011->imsc.b.txim = 0;
        // Clear tx interrupt
        pl011->icr.b.txim = 1;

        // Retry sending the data
        transmit_data(&uart->bs);
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

    // Enable Receive interrupt. Since FIFO is disabled by default, we
    // will get an int for every input char
    pl011->imsc.b.rxim = 1;

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
    if (uart_component_init_and_register(uart, comp, init, deinit, transmit_data) < 0){
        error("Failed to register '%s'", comp->id);
        return -1;
    }
    hwcomp_set_info((hwcomp_t *) uart, "PrimeCell UART (pl011)", "ARM", "AMBA compliant SoC");
    cur_uart++;

    return 0;
}

DEF_DRIVER_MANAGER(pl011, process);
