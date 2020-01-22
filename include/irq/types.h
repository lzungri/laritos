#pragma once

#include <stdint.h>
#include <utils/utils.h>
#include <arch/irq.h>
#include <arch/context-types.h>


typedef enum {
    IRQ_TRIGGER_EDGE_LOW_HIGH = 1,
    IRQ_TRIGGER_EDGE_HIGH_LOW = 2,
    IRQ_TRIGGER_LEVEL_HIGH = 4,
    IRQ_TRIGGER_LEVEL_LOW = 8,
} irq_trigger_mode_t;

typedef enum {
    IRQ_RET_ERROR = -1,
    IRQ_RET_HANDLED,
    IRQ_RET_HANDLED_KEEP_PROCESSING,
    IRQ_RET_NOT_HANDLED,

    IRQ_RET_LEN,
} irqret_t;

typedef uint16_t irq_t;

typedef irqret_t (*irq_handler_t)(irq_t irq, void *data);
