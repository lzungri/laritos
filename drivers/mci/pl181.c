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

/**
 * DISCLAIMER: This driver is just a simple implementation of the SD card protocol for the
 * ARM pl181 and it is only intended to be used under qemu, which ignores the voltage, clock,
 * regulators configuration among other vital but irrelevant stuff :p.
 *
 * Implementation is based on:
 *      - http://academy.cba.mit.edu/classes/networking_communications/SD/SD.pdf
 *      - https://www.nxp.com/docs/en/application-note/AN3049.pdf
 *      - u-boot arm_pl180_mmci.c driver
 *
 *      ...but mostly on debugging the qemu hw/arm/pl181.c and hw/sd/sd.c source
 */

//#define DEBUG
#include <log.h>

#include <stdbool.h>
#include <string.h>
#include <board/types.h>
#include <board/core.h>
#include <driver/core.h>
#include <driver/pl181.h>
#include <component/component.h>
#include <component/blockdev.h>
#include <component/mci.h>
#include <fs/pseudofs.h>
#include <time/core.h>
#include <math.h>
#include <utils/endian.h>
#include <dstruct/bitset.h>
#include <mm/heap.h>
#include <sync/spinlock.h>
#include <sync/condition.h>
#include <irq/types.h>
#include <component/intc.h>


#define PL181_PART 0x181

#define COMMAND_REG_DELAY_US 300
#define DATA_REG_DELAY_US 1000
#define CLK_CHANGE_DELAY_US 2000

// Maximum value by default
#define DEF_DATA_TIMER 0xffffffff

/**
 * Number of uint32_t entries in the FIFO
 */
#define PL181_FIFO_LEN 16

#define CLEAR_MASK 0x7ff


static int wait_for_response(pl181_mci_t *dev, mci_cmd_t *cmd) {
    mmcstatus_t imask = { 0 };
    imask.b.cmdtimeout = true;
    imask.b.cmdrespend = true;
    imask.b.cmdsent = true;

    int ret = 0;
    while (true) {
        irqctx_t ctx;
        spinlock_acquire(&dev->lock, &ctx);

        dev->mm->intmask[0] = imask;
        mmcstatus_t status = dev->mm->status;
        while (!(status.b.cmdtimeout || status.b.cmdrespend || status.b.cmdsent)) {
            condition_wait_locked(&dev->cond, &dev->lock, &ctx);
            status = dev->mm->status;
        }

        spinlock_release(&dev->lock, &ctx);

        verbose("Status for command=%u: 0x%08lx", cmd->idx, status.v);

        if (status.b.cmdtimeout) {
            error("Timeout for command=%u", cmd->idx);
            ret = -1;
            break;
        }

        if (cmd->has_resp && status.b.cmdrespend) {
            cmd->resp[0] = dev->mm->cmdresp[0];
            cmd->resp[1] = dev->mm->cmdresp[1];
            cmd->resp[2] = dev->mm->cmdresp[2];
            cmd->resp[3] = dev->mm->cmdresp[3];

            debug("Response for command=%u: 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx",
                    cmd->idx, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
            break;
        }

        if (!cmd->has_resp && status.b.cmdsent) {
            break;
        }
    }

    // Clear every status bit
    dev->mm->clear.v = CLEAR_MASK;

    return ret;
}

static int send_command(mci_t *mci, mci_cmd_t *cmd) {
    pl181_mci_t *dev = (pl181_mci_t *) mci;

    debug("Sending command=%u (arg=0x%08lx)", cmd->idx, cmd->arg);

    cmd->has_resp = mci_need_to_wait(cmd);

    mmcrawcmd_t rawcmd = { 0 };
    rawcmd.b.cpsm_enable = true;
    rawcmd.b.index = cmd->idx;
    rawcmd.b.wait_resp = cmd->has_resp;

    // Write command argument
    dev->mm->cmdarg = cmd->arg;
    // Wait for the sd to process it
    usleep(COMMAND_REG_DELAY_US);

    // Send command
    dev->mm->cmd = rawcmd;

    if (wait_for_response(dev, cmd) < 0) {
        error("Failed to process command=%u (arg=0x%08lx)", cmd->idx, cmd->arg);
        return -1;
    }

    return 0;
}

static int write_block(mci_t *mci, void *buf, uint32_t offset) {
    pl181_mci_t *dev = (pl181_mci_t *) mci;
    debug("Writing block of %lu bytes at offset=%lu", dev->parent.block_size, offset);
    if (mci->ops.send_command(mci, &(mci_cmd_t) { .idx = CMD24_WRITE_SINGLE_BLOCK, .arg = offset }) < 0) {
        error("Failed to send command for writing %lu bytes at offset=%lu", dev->parent.block_size, offset);
        return -1;
    }

    dev->mm->datatimer = DEF_DATA_TIMER;
    dev->mm->datalen.b.len = dev->parent.block_size;

    datactrl_t datactrl = { 0 };
    bitset_t bs = ~dev->parent.block_size;
    datactrl.b.block_size = BITSET_NBITS - bitset_ffz(bs) - 1,
    datactrl.b.dma_enable = false,
    datactrl.b.enable = true,
    datactrl.b.stream_mode = false,
    datactrl.b.read = false,

    dev->mm->datactrl = datactrl;

    usleep(DATA_REG_DELAY_US);

    mmcstatus_t imask = { 0 };
    imask.b.txfifohalfempty = true;
    imask.b.datatimeout = true;

    int ret = 0;
    uint32_t *src = buf;
    uint32_t bytesleft = dev->parent.block_size;
    while (bytesleft >= sizeof(uint32_t)) {
        irqctx_t ctx;
        spinlock_acquire(&dev->lock, &ctx);

        dev->mm->intmask[0] = imask;
        mmcstatus_t status = dev->mm->status;
        while (!(status.b.txfifohalfempty || status.b.datatimeout)) {
            condition_wait_locked(&dev->cond, &dev->lock, &ctx);
            status = dev->mm->status;
        }

        spinlock_release(&dev->lock, &ctx);

        if (status.b.datatimeout) {
            error("Timeout");
            ret = -1;
            break;
        }

        if (!status.b.txfifohalfempty) {
            continue;
        }

        int i;
        for (i = 0; i < PL181_FIFO_LEN; i++) {
            // FIFO data is transferred in little endian
            dev->mm->fifo[0] = cpu_to_le32(*src++);
        }
        bytesleft -= sizeof(uint32_t) * PL181_FIFO_LEN;
    }

    mmcstatus_t status = dev->mm->status;
    // Check the data was transferred successfully
    if (!status.b.datablockend) {
        error("Couldn't not complete transfer");
        ret = -1;
    }

    // Clear every status bit
    dev->mm->clear.v = CLEAR_MASK;

    return 0;
}

static int read_block(mci_t *mci, void *buf, uint32_t offset) {
    pl181_mci_t *dev = (pl181_mci_t *) mci;
    debug("Reading block of %lu bytes at offset=%lu", dev->parent.block_size, offset);

    if (mci->ops.send_command(mci, &(mci_cmd_t) { .idx = CMD17_READ_SINGLE_BLOCK, .arg = offset }) < 0) {
        error("Failed to send command for reading %lu bytes at offset=%lu", dev->parent.block_size, offset);
        return -1;
    }

    dev->mm->datatimer = DEF_DATA_TIMER;
    dev->mm->datalen.b.len = dev->parent.block_size;

    datactrl_t datactrl = { 0 };
    bitset_t bs = ~dev->parent.block_size;
    datactrl.b.block_size = BITSET_NBITS - bitset_ffz(bs) - 1,
    datactrl.b.dma_enable = false,
    datactrl.b.enable = true,
    datactrl.b.stream_mode = false,
    datactrl.b.read = true,

    dev->mm->datactrl = datactrl;

    usleep(DATA_REG_DELAY_US);

    mmcstatus_t imask = { 0 };
    imask.b.rxdataavail = true;
    imask.b.datatimeout = true;
    imask.b.rxoverrun = true;

    int ret = 0;
    uint32_t *dest = buf;
    uint32_t bytesleft = dev->parent.block_size;
    while (bytesleft >= sizeof(uint32_t)) {
        irqctx_t ctx;
        spinlock_acquire(&dev->lock, &ctx);

        dev->mm->intmask[0] = imask;

        mmcstatus_t status = dev->mm->status;
        while (!(status.b.rxdataavail || status.b.datatimeout || status.b.rxoverrun)) {
            condition_wait_locked(&dev->cond, &dev->lock, &ctx);
            status = dev->mm->status;
        }

        spinlock_release(&dev->lock, &ctx);

        if (status.b.datatimeout) {
            error("Timeout");
            ret = -1;
            break;
        }
        if (status.b.rxoverrun) {
            error("RX FIFO overrun");
            ret = -1;
            break;
        }

        if (!status.b.rxdataavail) {
            continue;
        }

        // FIFO data is transferred in little endian
        *dest++ = le32_to_cpu(dev->mm->fifo[0]);
        bytesleft -= sizeof(uint32_t);
    }

    // Check the data was transferred successfully
    mmcstatus_t status = dev->mm->status;
    if (!status.b.datablockend) {
        error("Couldn't not complete transfer");
        ret = -1;
    }

    // Clear every status bit
    dev->mm->clear.v = CLEAR_MASK;

    return ret;
}

static irqret_t pl181_irq_handler(irq_t irq, void *data) {
    pl181_mci_t *mci = (pl181_mci_t *) data;

    irqctx_t ctx;
    spinlock_acquire(&mci->lock, &ctx);

    // Acknowledge device / clear interrupt mask
    mci->mm->intmask[0].v = 0;
    // Wake up any process that may be waiting for the condition
    condition_notify_locked(&mci->cond);

    spinlock_release(&mci->lock, &ctx);

    return IRQ_RET_HANDLED;
}

static int init(component_t *c) {
    pl181_mci_t *mci = (pl181_mci_t *) c;

    pwrctrl_t pwr = { 0 };
    // Power on
    pwr.b.ctrl = 0b11,
    // Max voltage (qemu ignores this value)
    pwr.b.voltage = 0b1111,
    pwr.b.opendrain = false,
    pwr.b.rod = true,
    mci->mm->pwrctrl = pwr;

    // Configure clock (btw, qemu doesn't care about it...)
    clkctrl_t clk = { 0 };
    clk.b.enable = true,
    clk.b.div = 128,
    clk.b.pwrsave = false,
    mci->mm->clkctrl = clk;

    usleep(CLK_CHANGE_DELAY_US);

    // Reset interrupt masks
    mci->mm->intmask[0].v = 0;
    mci->mm->intmask[1].v = 0;

    // Check peripheral part id (12 bits)
    uint16_t part = (mci->mm->periph_id[1] << 8 | mci->mm->periph_id[0]) & 0x0fff;
    debug("Peripheral part=0x%x", part);
    if (part != PL181_PART) {
        error("Peripheral not supported");
        return -1;
    }

    if (mci_identify_and_register_new_card((mci_t *) mci) < 0) {
        error("Failed to initialize multimedia card interface");
        return -1;
    }

    // Enable the MCI irq on the interrupt controller
    if (intc_enable_irq_with_handler(mci->parent.intc, mci->parent.irq,
            mci->parent.irq_trigger, pl181_irq_handler, mci) < 0) {
        error("Failed to enable irq %u for '%s'", mci->parent.irq, ((component_t *) mci)->id);
        goto fail_irq;
    }

    return 0;

fail_irq:
    mci_unregister_card((mci_t *) mci);
    return -1;
}

static int deinit(component_t *c) {
    pl181_mci_t *mci = (pl181_mci_t *) c;

    mci_unregister_card((mci_t *) mci);

    // Power off
    pwrctrl_t pwr = { 0 };
    mci->mm->pwrctrl = pwr;

    // Disable clock
    mci->mm->clkctrl.b.enable = false;

    usleep(CLK_CHANGE_DELAY_US);

    return 0;
}

static int process(board_comp_t *comp) {
    pl181_mci_t *mci = component_alloc(sizeof(pl181_mci_t));
    if (mci == NULL) {
        error("Failed to allocate memory for '%s'", comp->id);
        return -1;
    }

    spinlock_init(&mci->lock);
    condition_init(&mci->cond);
    mci->parent.ops.send_command = send_command;
    mci->parent.ops.read_block = read_block;
    mci->parent.ops.write_block = write_block;

    if (mci_component_init((mci_t *) mci, comp, init, deinit) < 0) {
        error("Failed to register '%s'", comp->id);
        goto fail;
    }

    board_get_ptr_attr_def(comp, "baseaddr", (void **) &mci->mm, NULL);
    if (mci->mm == NULL) {
        error("No baseaddr was specified in the board information");
        goto fail;
    }

    if (board_get_component_attr(comp, "intc", (component_t **) &mci->parent.intc) < 0) {
        error("Invalid or no interrupt controller specified in the board info");
        goto fail;
    }

    int irq;
    if (board_get_int_attr(comp, "irq", &irq) < 0 || irq < 0) {
        error("Invalid or no irq was specified in the board info");
        goto fail;
    }
    mci->parent.irq = irq;

    board_get_irq_trigger_attr_def(comp, "trigger", &mci->parent.irq_trigger, IRQ_TRIGGER_LEVEL_HIGH);

    component_set_info((component_t *) mci, "PL181 MCI", "ARM", "ARM Primecell Multimedia Card Interface");

    if (mci_component_register((mci_t *) mci) < 0) {
        error("Couldn't register '%s'", comp->id);
        goto fail;
    }

    return 0;

fail:
    free(mci);
    return -1;
}

DRIVER_MODULE(pl181, process);
