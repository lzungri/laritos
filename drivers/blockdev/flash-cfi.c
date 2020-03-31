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

//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <board/types.h>
#include <board/core.h>
#include <time/core.h>
#include <driver/core.h>
#include <component/component.h>
#include <component/blockdev.h>
#include <fs/pseudofs.h>
#include <mm/heap.h>


typedef struct {
    blockdev_t parent;

    void *baseaddr;
} flash_cfi_t;


static int read(blockdev_t *blk, void *buf, size_t n, uint32_t offset) {
    flash_cfi_t *flash = (flash_cfi_t *) blk;
    return pseudofs_raw_write_to_buf(buf, n, flash->baseaddr, flash->parent.size_kbs << 10, offset, false);
}

static int write(blockdev_t *blk, void *buf, size_t n, uint32_t offset) {
    flash_cfi_t *flash = (flash_cfi_t *) blk;
    return pseudofs_raw_write_to_buf(flash->baseaddr, flash->parent.size_kbs << 10, buf, n, offset, false);
}

static int process(board_comp_t *comp) {
    flash_cfi_t *flash = component_alloc(sizeof(flash_cfi_t));
    if (flash == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    if (blockdev_component_init((blockdev_t *) flash, comp, NULL, NULL, read, write) < 0) {
        error("Failed to register '%s'", comp->id);
        goto fail;
    }

    board_get_ptr_attr(comp, "baseaddr", &flash->baseaddr);

    component_set_info((component_t *) flash, "Flash CFI", "AMD, Intel, Sharp, Fujitsu", "Common Flash Memory Interface");

    if (blockdev_component_register((blockdev_t *) flash) < 0) {
        error("Couldn't register '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(flash);
    return -1;
}

DRIVER_MODULE(flash_cfi, process);
