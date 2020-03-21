/**
 * DISCLAIMER: This driver is just a simple implementation of the SD card protocol
 * and it is only intended to be used under qemu, which ignores the voltage, clock,
 * regulators configuration among other vital but irrelevant stuff :p.
 *
 * The driver only supports one card attached to the MCI
 *
 * Implementation is based on:
 *      - http://academy.cba.mit.edu/classes/networking_communications/SD/SD.pdf
 *      - https://www.nxp.com/docs/en/application-note/AN3049.pdf
 *      - u-boot arm_pl180_mmci.c driver
 *
 *      ...but mostly on debugging the qemu hw/arm/pl181.c and hw/sd/sd.c source
 */

#define DEBUG
#include <log.h>

#include <printf.h>
#include <stdint.h>
#include <string.h>
#include <board/core.h>
#include <component/component.h>
#include <component/mci.h>
#include <utils/math.h>
#include <mm/heap.h>

#define VDD_VOLTAGE_WINDOW_MASK 0xffffff


bool mci_need_to_wait(mci_cmd_t *cmd) {
    switch (cmd->idx) {
    case CMD1_SEND_OP_CMD:
    case CMD2_ALL_SEND_CID:
    case CMD3_SEND_RELATIVE_ADDR:
    case CMD7_SE_DESELECT_CARD:
    case CMD16_SET_BLOCKLEN:
    case CMD17_READ_SINGLE_BLOCK:
    case CMD24_WRITE_SINGLE_BLOCK:
    case CMD55_APP_CMD:
    case ACMD41_SD_APP_OP_COND:
        return true;
    }
    return false;
}

static int read(blockdev_t *blk, void *buf, size_t blen, uint32_t offset) {
    mci_sdcard_t *sdcard = (mci_sdcard_t *) blk;

    uint32_t nbytes = 0;
    while (blen > nbytes) {
        char block[DEF_BLOCK_SIZE];
        if (sdcard->mci->ops.read_block(sdcard->mci, block, offset + nbytes) < 0) {
            error("Failed to read %u bytes at offset=%lu from card", sizeof(block), nbytes);
            return -1;
        }
        memcpy((char *) buf + nbytes, block, min(blen - nbytes, sizeof(block)));
        nbytes += sizeof(block);
    }

    return blen;
}

static int write(blockdev_t *blk, void *buf, size_t blen, uint32_t offset) {
    mci_sdcard_t *sdcard = (mci_sdcard_t *) blk;

    uint32_t nbytes = 0;
    while (blen - nbytes >= sdcard->parent.sector_size) {
        if (sdcard->mci->ops.write_block(sdcard->mci, (char *) buf + nbytes, offset + nbytes) < 0) {
            error("Failed to write %u bytes at offset=%lu from card", DEF_BLOCK_SIZE, nbytes);
            return -1;
        }
        nbytes += sdcard->parent.sector_size;
    }

    // Apparently, we can only read/write block-sized chunks (see pl181 datasheet,
    // data control register, block size field). Thus, if we are not writing a
    // multiple of a block, we need to read the last block from the card, replace
    // the corresponding content and write it back
    if (blen - nbytes > 0) {
        char block[DEF_BLOCK_SIZE];
        if (sdcard->mci->ops.read_block(sdcard->mci, block, offset + nbytes) < 0) {
            error("Failed to read %u bytes at offset=%lu from card", sizeof(block), nbytes);
            return -1;
        }

        // Replace block content with the new data
        memcpy(block, (char *) buf + nbytes, blen - nbytes);

        // Write the block back
        if (sdcard->mci->ops.write_block(sdcard->mci, block, offset + nbytes) < 0) {
            error("Failed to write %u bytes at offset=%lu from card", DEF_BLOCK_SIZE, nbytes);
            return -1;
        }
    }

    return blen;
}

int mci_register_card(mci_t *mci) {
    board_comp_t comp;
    char id[COMPONENT_MAX_ID_LEN] = { 0 };
    snprintf(id, sizeof(id), "%s-sd0", mci->parent.id);
    comp.id = id;
    comp.attrlen = 2;
    // These values should be obtained dynamically from the MCI for each card,
    // maybe next time. Assume a 32MB sdcard
    comp.attr[0].name = "sectorsize";
    comp.attr[0].value = "128";
    comp.attr[1].name = "nsectors";
    comp.attr[1].value = "262144";

    mci_sdcard_t *sdcard = component_alloc(sizeof(mci_sdcard_t));
    if (sdcard == NULL) {
        error("Failed to allocate memory for '%s'", comp.id);
        return -1;
    }
    sdcard->mci = mci;

    if (blockdev_component_init((blockdev_t *) sdcard, &comp, NULL, NULL, read, write) < 0) {
        error("Failed to register '%s'", comp.id);
        goto fail;
    }

    component_set_info((component_t *) sdcard, "SD card", "Unknown", "Secure Digital card");

    if (blockdev_component_register((blockdev_t *) sdcard) < 0) {
        error("Couldn't register '%s'", comp.id);
        goto fail;
    }

    return 0;

fail:
    free(sdcard);
    return -1;
}

int mci_unregister_card(mci_t *mci) {
    char id[COMPONENT_MAX_ID_LEN] = { 0 };
    snprintf(id, sizeof(id), "%s-sd0", mci->parent.id);
    component_t *bdev = component_get_by_id(id);
    if (bdev == NULL) {
        error("No sdcard with id '%s'", id);
        return -1;
    }
    return component_unregister(bdev);
}

int mci_identify_and_register_new_card(mci_t *mci) {
    // The following initialization code is a VERY simplified version of the protocol
    // described in the "4.2 Card Identification Mode" section of the SD physical layer
    // specification:
    //      http://academy.cba.mit.edu/classes/networking_communications/SD/SD.pdf
    //
    // It only supports one card connected to the MCI

    // Set card in idle state
    if (mci->ops.send_command(mci, &(mci_cmd_t) { .idx = CMD0_GO_IDLE_STATE }) < 0) {
        error("Couldn't change card to idle mode");
        return -1;
    }

    // Prepare card for application command (ACMD41)
    if (mci->ops.send_command(mci, &(mci_cmd_t) { .idx = CMD55_APP_CMD }) < 0) {
        error("Couldn't prepare card to accept an application command");
        return -1;
    }

    // Validate the card supports the given voltage range (qemu just ignores this).
    // 1 << 23 means 3.5v to 3.6v (pl181_t documentation for more info)
    mci_cmd_t cmd41 = { .idx = ACMD41_SD_APP_OP_COND, .arg = 1 << 23 };
    if (mci->ops.send_command(mci, &cmd41) < 0) {
        error("Couldn't prepare card to accept an application command");
        return -1;
    }
    mci->ocr = cmd41.resp[0] & VDD_VOLTAGE_WINDOW_MASK;
    debug("Card OCR=0x%08lx", mci->ocr);

    // Get the unique card id
    mci_cmd_t cmd2 = { .idx = CMD2_ALL_SEND_CID };
    if (mci->ops.send_command(mci, &cmd2) < 0) {
        error("Couldn't retrieve the card id");
        return -1;
    }
    info("SD card id: 0x%08lx%08lx%08lx%08lx", cmd2.resp[0], cmd2.resp[1], cmd2.resp[2], cmd2.resp[3]);

    // Publish a new relative card address (used later for host <-> card communication)
    mci_cmd_t cmd3 = { .idx = CMD3_SEND_RELATIVE_ADDR };
    if (mci->ops.send_command(mci, &cmd3) < 0) {
        error("Couldn't publish a new rca");
        return -1;
    }
    mci->rca = cmd3.resp[0] >> 16;
    debug("Card RCA=0x%08x", mci->rca);

    // Select the card, put it into transfer mode and make it ready for reading/writing data.
    // The RCA value got in the previous command is sent as a uint32_t with the rca value
    // in the most significant bytes
    if (mci->ops.send_command(mci, &(mci_cmd_t) { .idx = CMD7_SE_DESELECT_CARD, .arg = mci->rca << 16 }) < 0) {
        error("Couldn't put card in transfer mode");
        return -1;
    }

    // Set block size
    if (mci->ops.send_command(mci, &(mci_cmd_t) { .idx = CMD16_SET_BLOCKLEN, .arg = DEF_BLOCK_SIZE }) < 0) {
        error("Failed to set block size");
        return -1;
    }
    mci->block_size = DEF_BLOCK_SIZE;

    if (mci_register_card(mci) < 0) {
        error("Couldn't register card");
        return -1;
    }

    return 0;
}
