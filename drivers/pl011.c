#include <log.h>
#include <driver.h>
#include <board.h>
#include <uart.h>
#include <drivers/pl011.h>
#include <stream.h>
#include <utils.h>

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

    // TODO Use interrupts
    if (s->blocking) {
        // Wait until there is some data in the FIFO
        while (pl011->fr.b.rxfe);
    }

    int i;
    uint8_t *data = buf;
    // Read as long as there is some data in the FIFO and the amount read is < n
    for (i = 0; i < n && !pl011->fr.b.rxfe; i++) {
        data[i] = pl011->dr;
    }
    return i;
}

static int process(board_comp_t *comp) {
    if (cur_uart > ARRAYSIZE(uarts)) {
        error("Max number of uart components reached");
        return -1;
    }
    if (uart_init_and_register(&uarts[cur_uart], comp, NULL, NULL, read, write) < 0){
        error("Failed to register '%s'", comp->id);
        return -1;
    }
    cur_uart++;

    return 0;
}

DEF_DRIVER_MANAGER(pl011, process);
