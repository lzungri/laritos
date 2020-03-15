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


int read(blockdev_t *blk, void *buf, size_t n, uint32_t offset) {
    flash_cfi_t *flash = (flash_cfi_t *) blk;
    return pseudofs_raw_write_to_buf(buf, n, flash->baseaddr, flash->parent.size_kbs << 10, offset, false);
}

int write(blockdev_t *blk, void *buf, size_t n, uint32_t offset) {
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
