/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>
#include <component/uart.h>
#include <component/stream.h>
#include <irq/types.h>
#include <board/types.h>
#include <driver/core.h>
#include <driver/pl011.h>
#include <utils/utils.h>
#include <mm/heap.h>


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
    while (circbuf_peek(&bs->txcb, &data, sizeof(data)) > 0) {
        if (pl011->fr.b.txff) {
            // Enable Transmit interrupt to detect FIFO space availability
            pl011->imsc.b.txim = 1;
            circbuf_peek_complete(&bs->txcb, false);
            break;
        }
        pl011->dr = data;
        sent += sizeof(data);
        circbuf_peek_complete(&bs->txcb, true);
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

static irqret_t pl011_irq_handler(irq_t irq, void *data) {
    uart_t *uart = (uart_t *) data;
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    // RECEIVE interrupt
    if (pl011->mis.b.rxim) {
        verbose_async("UART data received irq");
        if (put_into_bytestream(uart) < 0) {
            error_async("Couldn't read data");
            pl011->icr.b.rxim = 1;
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
    uart_t *uart = (uart_t *) c;
    if (uart_init(uart) < 0) {
        error("Failed to initialize uart for component '%s'", c->id);
        return -1;
    }

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

    return uart_deinit(uart);
}

static int process(board_comp_t *comp) {
    uart_t *uart = component_alloc(sizeof(uart_t));
    if (uart == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }
    uart->irq_handler = pl011_irq_handler;
    if (uart_component_init_and_register(uart, comp, init, deinit, transmit_data) < 0){
        error("Failed to register '%s'", comp->id);
        goto fail;
    }
    component_set_info((component_t *) uart, "PrimeCell UART (pl011)", "ARM", "UART AMBA compliant SoC");

    return 0;

fail:
    free(uart);
    return -1;
}

DRIVER_MODULE(pl011, process);
