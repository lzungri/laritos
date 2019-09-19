#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <component/intc.h>

/**
 * A read of the GICC_IAR returns the interrupt ID of the highest priority
 * pending interrupt for the CPU interface. The
 * read returns a spurious interrupt ID of 1023 if any of the following apply:
 *      * forwarding of interrupts by the Distributor to the CPU interface is
 *      disabled
 *      * signaling of interrupts by the CPU interface to the connected processor
 *      is disabled
 *      * no pending interrupt on the CPU interface has sufficient priority for
 *      the interface to signal it to the processor
 */
#define GICV2_SPURIOUS_INT_ID 1023


typedef enum {
    ARCH_REV_UNKNOWN,
    ARCH_REV_GICV1,
    ARCH_REV_GICV2,
    ARCH_REV_GICV3,
    ARCH_REV_GICV4,
} arch_rev_t;

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
 * Provides information about the configuration of the GIC. It indicates:
 *      * whether the GIC implements the Security Extensions
 *      * the maximum number of interrupt IDs that the GIC supports
 *      * the number of CPU interfaces implemented
 *      * if the GIC implements the Security Extensions, the maximum number of
 *        implemented Lockable Shared Peripheral Interrupts (LSPIs).
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            /**
             * Indicates the maximum number of interrupts that the GIC supports. If ITLinesNumber=N, the
             * maximum number of interrupts is 32(N+1). The interrupt ID range is from 0 to
             * (number of IDs – 1). For example:
             *      0b00011 Up to 128 interrupt lines, interrupt IDs 0-127
             */
            uint8_t nlines: 5;
            /**
             * Indicates the number of implemented CPU interfaces. The number of implemented CPU
             * interfaces is one more than the value of this field, for example if this field is
             * 0b011, there are four CPU interfaces
             */
            uint8_t ncpu: 3;
            uint8_t reserved0: 2;
            /**
             * Indicates whether the GIC implements the Security Extensions
             */
            bool sec_ext: 1;
            /**
             * If the GIC implements the Security Extensions, the value of this field is the
             * maximum number of implemented lockable SPIs, from 0 (0b00000) to 31 (0b11111)
             */
            uint8_t lspi: 5;
            uint32_t reserved1: 16;
        } b;
    };
} gic_dist_type_t;

/**
 * Provides a four-bit architecturally-defined architecture revision
 * field. The remaining bits of the register are IMPLEMENTATION DEFINED
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            uint8_t reserved0: 4;
            arch_rev_t arch_rev: 4;
            uint32_t reserved1: 24;
        } b;
    };
} perip_id2_t;

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
    const gic_dist_type_t type;
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
    uint8_t reserved2[184];
    /**
     * Provides a four-bit architecturally-defined architecture revision field.
     * The remaining bits of the register are IMPLEMENTATION DEFINED
     */
    perip_id2_t perip_id2;
} gic_dist_t;


/**
 * CPU Interface Control Register
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            /**
             * Enable for the signaling of Group 0 interrupts by the CPU interface to
             * the connected processor
             */
            bool enable_group0: 1;
            /**
             * Enable for the signaling of Group 1 interrupts by the CPU interface to
             * the connected processor
             */
            bool enable_group1: 1;
            /**
             * When the highest priority pending interrupt is a Group 1 interrupt, determines both:
             *      * whether a read of GICC_IAR acknowledges the interrupt, or returns a spurious
             *      interrupt ID
             *      * whether a read of GICC_HPPIR returns the ID of the highest priority pending
             *      interrupt, or returns a spurious interrupt ID
             */
            bool ack_ctrl: 1;
            /**
             * Controls whether the CPU interface signals Group 0 interrupts to a target
             * processor using the FIQ or the IRQ signal
             *      0 Signal Group 0 interrupts using the IRQ signal.
             *      1 Signal Group 0 interrupts using the FIQ signal.
             *
             * The GIC always signals Group 1 interrupts using the IRQ signal.
             */
            bool fiq_enable: 1;
            /**
             * Controls whether the GICC_BPR provides common control to Group 0 and
             * Group 1 interrupts
             */
            bool cbpr: 1;
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
            /**
             * For SGIs in a multiprocessor implementation, this field identifies the
             * processor that requested the interrupt. It returns the number of the
             * CPU interface that made the request
             */
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

    uint16_t num_irqs;
} gic_t;
