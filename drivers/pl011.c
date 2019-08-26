#include <log.h>
#include <driver.h>
#include <board.h>
#include <uart.h>
#include <drivers/pl011.h>
#include <utils.h>

#define MAX_UARTS 3

// TODO Use dynamic memory instead
static uart_t uarts[3];
static uint8_t cur_uart;

static int write(uart_t *uart, const void *buf, size_t n) {
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;

    // Wait until there is space in the FIFO
    while (pl011->fr.b.txff);

    const uint8_t *data = buf;
    int i;
    for (i = 0; i < n; i++) {
        pl011->dr = data[i];
    }
    return n;
}

static int read(uart_t *uart, void *buf, size_t n) {
    pl011_mm_t *pl011 = (pl011_mm_t *) uart->baseaddr;
    int i;
    uint8_t *data = buf;
    // Read as long as there is some data in the FIFO and the amount read is < n
    for (i = 0; i < n && !pl011->fr.b.rxfe; i++) {
        data[i] = pl011->dr;
    }
    return i;
}

static int init(component_t *c) {
    return 0;
}

static int deinit(component_t *c) {
    return 0;
}

static int process(board_comp_t *comp) {
    if (cur_uart > ARRAYSIZE(uarts)) {
        error("Max number of uart components reached");
        return -1;
    }

    uart_t *uart = &uarts[cur_uart];

    if (uart_init(uart, comp, init, deinit, read, write) < 0){
        error("Failed to initialize '%s'", comp->id);
        return -1;
    }

    if (uart_register(uart) < 0) {
        error("Couldn't register uart '%s'", comp->id);
        return -1;
    }

    cur_uart++;

    return 0;
}

DEF_DRIVER_MANAGER(pl011, process);
