#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <intc.h>

/**
 * Controls the generation of SGIs
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint8_t intid: 4;
            uint16_t reserved0: 11;
            bool nsatt: 1;
            uint8_t cpu_target_list: 8;
            uint8_t target_list_filter: 2;
            uint8_t reserved1: 6;
        } b;
    };
} gic_dist_sgi_t;

/**
 * Enables the forwarding of pending interrupts from the Distributor to the CPU interfaces
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            bool enable_group0: 1;
            bool enable_group1: 1;
            uint32_t reserved0: 30;
        } b;
    };
} gic_dist_ctrl_t;

/**
 * The Distributor centralizes all interrupt sources, determines the priority of each
 * interrupt, and for each CPU interface forwards the interrupt with the highest priority
 * to the interface, for priority masking and preemption handling
 *
 */
typedef volatile struct {
    /**
     * Distributor Control Register
     */
    gic_dist_ctrl_t ctrl;
    /**
     * Interrupt Controller Type Register
     */
    const uint32_t type;
    /**
     * Distributor Implementer Identification Register
     */
    const uint32_t iid;
    /**
     * Reserved
     */
    uint8_t reserved0[0x74];
    /**
     * Interrupt Group Registers
     */
    uint8_t group[128];
    /**
     * Interrupt Set-Enable Registers
     */
    uint8_t set_enable[128];
    /**
     * Interrupt Clear-Enable Registers
     */
    uint8_t clear_enable[128];
    /**
     * Interrupt Set-Pending Registers
     */
    uint8_t set_pending[128];
    /**
     * Interrupt Clear-Pending Registers
     */
    uint8_t clear_pending[128];
    /**
     * GICv2 Interrupt Set-Active Registers
     */
    uint8_t set_active[128];
    /**
     * GICv2 Interrupt Clear-Active Registers
     */
    uint8_t clear_active[128];
    /**
     * Interrupt Priority Registers
     */
    uint8_t priority[1024];
    /**
     * Interrupt Processor Targets Registers
     */
    uint8_t targets[1024];
    /**
     * Interrupt Configuration Registers
     */
    uint8_t cfg[512];
    /**
     * Non-secure Access Control Registers, optional
     */
    uint8_t non_secure_actrl[256];
    /**
     * Software Generated Interrupt Register
     */
    gic_dist_sgi_t sgi;
    /**
     * Reserved
     */
    uint8_t reserved1[12];
    /**
     * SGI Clear-Pending Registers
     */
    uint8_t sgi_clear_pending[16];
    /**
     * SGI Set-Pending Registers
     */
    uint8_t sgi_set_pending[16];
} gic_dist_t;


/**
 * CPU Interface Control Register
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            /**
             * Enable for the signaling of Group 1 interrupts by the CPU interface to
             * the connected processor
             */
            bool enable_group1: 1;
            uint8_t reserved0: 4;
            /**
             * When the signaling of FIQs by the CPU interface is disabled, this bit partly
             * controls whether the bypass FIQ signal is signaled to the processor
             */
            bool fiq_bypass: 1;
            /**
             * When the signaling of IRQs by the CPU interface is disabled, this bit partly
             * controls whether the bypass IRQ signal is signaled to the processor
             */
            bool irq_bypass: 1;
            uint8_t reserved1: 2;
            /**
             * Controls the behavior of Non-secure accesses to the GICC_EOIR and GICC_DIR registers:
             *      0 GICC_EOIR has both priority drop and deactivate interrupt functionality.
             *      Accesses to the GICC_DIR are UNPREDICTABLE.
             *
             *      1 GICC_EOIR has priority drop functionality only. The GICC_DIR register has
             *      deactivate interrupt functionality.
             */
            bool eoi_mode: 1;
            uint32_t reserved2: 12;
        } b;
    };
} gic_cpu_ctrl_t;

/**
 * Interrupt Priority Mask Register
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint8_t priority: 8;
            uint32_t reserved0: 24;
        } b;
    };
} gic_cpu_prio_mask_t;

/**
 * Interrupt Acknowledge Register
 *
 * The processor reads this register to obtain the interrupt ID of the signaled interrupt.
 * This read acts as an acknowledge for the interrupt
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint16_t id: 10;
            uint8_t cpuid: 3;
            uint32_t reserved0: 19;
        } b;
    };
} gic_cpu_int_ack_t;

/**
 * Each CPU interface block provides the interface for a processor that is connected to the GIC
 */
typedef volatile struct {
    gic_cpu_ctrl_t ctrl;
    gic_cpu_prio_mask_t prio_mask;
    /**
     * Binary Point Register
     */
    uint32_t bin_point;
    gic_cpu_int_ack_t int_ack;
    /**
     * A processor writes to this register to inform the CPU interface either:
     *      * that it has completed the processing of the specified interrupt
     *      * in a GICv2 implementation, when the appropriate GICC_CTLR.EOImode bit is set
     *        to 1, to indicate that the interface should perform priority drop for the specified
     *        interrupt.
     */
    gic_cpu_int_ack_t end_int;

    // TODO: Populate all the other registers
} gic_cpu_t;

/**
 * The GIC is a centralized resource for supporting and managing interrupts in a system that
 * includes at least one processor
 */
typedef struct {
    intc_t parent;

    gic_dist_t *dist;
    gic_cpu_t *cpu;
} gic_t;
