#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <board/types.h>
#include <component/component.h>
#include <component/blockdev.h>
#include <component/intc.h>
#include <irq/types.h>
#include <sync/rmutex.h>


#define DEF_BLOCK_SIZE_BITS 7
#define DEF_BLOCK_SIZE (1 << DEF_BLOCK_SIZE_BITS)

typedef enum {
    CMD0_GO_IDLE_STATE = 0,
    CMD1_SEND_OP_CMD = 1,
    CMD2_ALL_SEND_CID = 2,
    CMD3_SEND_RELATIVE_ADDR = 3,
    CMD7_SE_DESELECT_CARD = 7,
    CMD16_SET_BLOCKLEN = 16,
    CMD17_READ_SINGLE_BLOCK = 17,
    CMD24_WRITE_SINGLE_BLOCK = 24,
    CMD55_APP_CMD = 55,
} mmi_cmd_type_t;

typedef enum {
    ACMD41_SD_APP_OP_COND = 41
} mmi_appcmd_type_t;

typedef struct {
    uint8_t idx;
    uint8_t has_resp;
    uint32_t arg;
    uint32_t resp[4];
} mci_cmd_t;

struct mci;
typedef struct {
    int (*send_command)(struct mci *mci, mci_cmd_t *cmd);
    /**
     * @param buf: Pointer to buffer (must be of at least DEF_BLOCK_SIZE bytes)
     */
    int (*read_block)(struct mci *mci, void *buf, uint32_t offset);
    /**
     * @param buf: Pointer to buffer (must be of at least DEF_BLOCK_SIZE bytes)
     */
    int (*write_block)(struct mci *mci, void *buf, uint32_t offset);
} mci_ops_t;

typedef struct mci {
    component_t parent;

    uint32_t block_size;

    /**
     * Relative card address (Note: This driver only supports one card)
     *
     * This address is used for the addressed host-card communication after the
     * card identification procedure. The default value of the RCA register
     * is 0x0000. The value 0x0000 is reserved to set all cards into the
     * Stand-by State with CMD7.
     */
    uint16_t rca;

    /**
     * The 32-bit operation conditions register stores the VDD voltage profile of
     * the card (Note: This driver only supports one card)
     * Bit:
     *   15: 2.7-2.8
     *   16: 2.8-2.9
     *   17: 2.9-3.0
     *   18: 3.0-3.1
     *   19: 3.1-3.2
     *   20: 3.2-3.3
     *   21: 3.3-3.4
     *   22: 3.4-3.5
     *   23: 3.5-3.6
     *
     * Everything else: reserved
     */
    uint32_t ocr;

    intc_t *intc;
    irq_t irq;
    irq_trigger_mode_t irq_trigger;

    rmutex_t mutex;

    mci_ops_t ops;
} mci_t;

typedef struct {
    blockdev_t parent;
    mci_t *mci;
} mci_sdcard_t;


int mci_identify_and_register_new_card(mci_t *mci);
int mci_unregister_card(mci_t *mci);
int mci_register_card(mci_t *mci);
bool mci_need_to_wait(mci_cmd_t *cmd);
int mci_component_init(mci_t *mci, board_comp_t *comp,
        int (*init)(component_t *c), int (*deinit)(component_t *c));
int mci_component_register(mci_t *mci);
