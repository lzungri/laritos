#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <component/mci.h>
#include <sync/spinlock.h>
#include <sync/condition.h>

/**
 * The MCIPower register controls an external power supply.
 * You can switch the power on and off, and adjust the output voltage
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint8_t ctrl: 2;
            uint8_t voltage: 4;
            bool opendrain: 1;
            bool rod: 1;
            uint32_t reserved: 24;
        } b;
    };
} pwrctrl_t;

typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint8_t div;
            bool enable: 1;
            bool pwrsave: 1;
            bool bypass: 1;
            uint32_t reserved: 21;
        } b;
    };
} clkctrl_t;

/**
 * The MCICommand register contains the command index and command type bits:
 *   - The command index is sent to a card as part of a command message
 *   - The command type bits control the Command Path State Machine (CPSM).
 *     Writing 1 to the enable bit starts the command send operation, while clearing the bit disables the CPSM.
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint8_t index: 6;
            bool wait_resp: 1;
            bool long_response: 1;
            bool interrupt: 1;
            bool pending: 1;
            bool cpsm_enable: 1;
            uint32_t reserved: 21;
        } b;
    };
} mmcrawcmd_t;

/**
 * The MCIRespCommand register contains the command index field of the last
 * command response received
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint8_t idx: 6;
            uint32_t reserved: 26;
        } b;
    };
} cmdrespidx_t;

/**
 * The MCIDataCtrl register controls the Data path state machine
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            bool enable: 1;
            bool read: 1;
            bool stream_mode: 1;
            bool dma_enable: 1;
            uint8_t block_size: 4;
            uint32_t reserved: 24;
        } b;
    };
} datactrl_t;

/**
 * The MCIDataLength register contains the number of data bytes to be
 * transferred. The value is loaded into the data counter when data
 * transfer starts
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint16_t len: 16;
            uint32_t reserved: 16;
        } b;
    };
} datalen_t;

typedef volatile struct {
    union {
        uint32_t v;
        struct {
            bool cmdcrcfail: 1;
            bool datacrcfail: 1;
            bool cmdtimeout: 1;
            bool datatimeout: 1;
            bool txunderrun: 1;
            bool rxoverrun: 1;
            bool cmdrespend: 1;
            bool cmdsent: 1;
            bool dataend: 1;
            uint8_t reserved: 1;
            bool datablockend: 1;
            bool cmdactive: 1;
            bool txactive: 1;
            bool rxactive: 1;
            bool txfifohalfempty: 1;
            bool rxfifohalffull: 1;
            bool txfifofull: 1;
            bool rxfifofull: 1;
            bool txfifoempty: 1;
            bool rxfifoempty: 1;
            bool txdataavail: 1;
            bool rxdataavail: 1;
            uint32_t reserved1: 10;
        } b;
    };
} mmcstatus_t;

/**
 * The MCIClear register is a write-only register. The corresponding
 * static status flags can be cleared by writing a 1 to the corresponding
 * bit in the register
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            bool cmdcrcfail: 1;
            bool datacrcfail: 1;
            bool cmdtimeout: 1;
            bool datatimeout: 1;
            bool txunderrun: 1;
            bool rxoverrun: 1;
            bool cmdrespend: 1;
            bool cmdsent: 1;
            bool dataend: 1;
            bool reserved: 1;
            bool datablockend: 1;
            uint32_t reserved1: 21;
        } b;
    };
} clear_t;

/**
 * The MCIFifoCnt register contains the remaining number of words to
 * be written to or read from the FIFO. The FIFO counter loads the
 * value from the data length register (see Data length register,
 * MCIDataLength) when the Enable bit is set in the data control register
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint16_t datacount: 15;
            uint32_t reserved1: 17;
        } b;
    };
} fifocnt_t;


/**
 * Register map for the PL181 MCI.
 *
 * MCI memory map and registers based on:
 *      http://www.digchip.com/data/038/038-00576-0-IPC7013.pdf
 *      http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf
 *
 * NOTE:
 *      The base address of the PL181 is not fixed, and can be different for any particular
 *      system implementation. The offset of each register from the base address is fixed.
 */
typedef volatile struct {
    pwrctrl_t pwrctrl;
    clkctrl_t clkctrl;
    /**
     * The MCIArgument register contains a 32-bit command argument,
     * which is sent to a card as part of a command message
     */
    uint32_t cmdarg;
    mmcrawcmd_t cmd;
    cmdrespidx_t cmdresp_idx;
    /**
     * The MCIResponse0-3 registers contain the status of a card, which is part
     * of the received response
     */
    uint32_t cmdresp[4];
    /**
     * The MCIDataTimer register contains the data timeout period,
     * in card bus clock periods
     */
    uint32_t datatimer;
    datalen_t datalen;
    datactrl_t datactrl;
    /**
     * The MCIDataCnt register loads the value from the data length register
     * (see Data length register, MCIDataLength) when the DPSM moves from
     * the IDLE state to the WAIT_R or WAIT_S state. As data is transferred,
     * the counter decrements the value until it reaches 0. The DPSM then
     * moves to the IDLE state and the data status end flag is set.
     */
    datalen_t datacnt;
    mmcstatus_t status;
    clear_t clear;
    /**
     * There are two interrupt mask registers, MCIMask0-1, one for
     * each interrupt request signal
     */
    mmcstatus_t intmask[2];
    uint32_t reserved0;
    fifocnt_t fifocnt;
    uint8_t reserved1[52];
    /**
     * The receive and transmit FIFOs can be read or written as 32-bit
     * wide registers. The FIFOs contain 16 entries on 16 sequential
     * addresses. This allows the microprocessor to use its load and
     * store multiple operands to read/write to the FIFO
     */
    uint32_t fifo[16];
    uint8_t reserved2[3872];
    char periph_id[8];
} pl181_mm_t;


typedef struct {
    mci_t parent;

    pl181_mm_t *mm;
    spinlock_t lock;
    condition_t cond;
} pl181_mci_t;
