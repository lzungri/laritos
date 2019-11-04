#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void *regpc_t;
typedef void *regsp_t;
typedef void *regret_t;

/**
 * Type that holds the size of a process section for this architecture
 */
typedef uint32_t secsize_t;

/**
 * The DFSR holds status information about the last data fault
 */
typedef struct {
    union {
        uint32_t v;
        struct {
            /**
             * FS, bits[10, 3:0]
             * Fault status bits. For the valid encodings of these bits when using the Short-descriptor translation
             * table format, see Table B3-23 on page B3-1415. All encodings not shown in the table are reserved
             */
            uint8_t fs_l: 4;
            /**
             * Domain, bits[7:4]
             * The domain of the fault address.
             * ARM deprecates any use of this field, see The Domain field in the DFSR on page B3-1416.
             * This field is UNKNOWN on a Data Abort exception:
             *    * caused by a debug exception
             *    * caused by a Permission fault in an implementation includes the Large Physical Address
             * Extension.
             *
             */
            uint8_t domain: 4;
            /**
             * Bit[8]
             * Reserved, UNK/SBZP.
             */
            uint8_t reserved1: 1;
            /**
             * LPAE, bit[9], if the implementation includes the Large Physical Address Extension
             * On taking a Data Abort exception, this bit is set to 0 to indicate use of the Short-descriptor
             * translation table formats.
             * Hardware does not interpret this bit to determine the behavior of the memory system, and therefore
             * software can set this bit to 0 or 1 without affecting operation. Unless the register has been updated
             * to report a fault, a subsequent read of the register returns the value written to it.
             *
             * Bit[9], if the implementation does not include the Large Physical Address Extension
             * Reserved, UNK/SBZP.
             */
            bool lpae: 1;
            /**
             * FS, bits[10, 3:0]
             * Fault status bits. For the valid encodings of these bits when using the Short-descriptor translation
             * table format, see Table B3-23 on page B3-1415. All encodings not shown in the table are reserved
             */
            uint8_t fs_h: 1;
            /**
             * WnR, bit[11] Write not Read bit. On a synchronous exception, indicates whether the abort was caused by a write
             * instruction or by a read instruction. The possible values of this bit are:
             * 0. Abort caused by a read instruction.
             * 1. Abort caused by a write instruction.
             * For synchronous faults on CP15 cache maintenance operations, including the address translation
             * operations, this bit always returns a value of 1.
             * This bit is UNKNOWN on:
             *    * An asynchronous Data Abort exception
             *    * A Data Abort exception caused by a debug exception.
             */
            bool wnr: 1;
            /**
             * ExT, bit[12]
             * External abort type. This bit can provide an IMPLEMENTATION DEFINED classification of external
             * aborts.
             * For aborts other than external aborts this bit always returns 0.
             * In an implementation that does not provide any classification of external aborts, this bit is
             * UNK/SBZP.
             */
            bool ext: 1;
            /**
             * CM, bit[13], if implementation includes the Large Physical Address Extension
             * Cache maintenance fault. For synchronous faults, this bit indicates whether a cache maintenance
             * operation generated the fault. The possible values of this bit are:
             * 0. Abort not caused by a cache maintenance operation.
             * 1. Abort caused by a cache maintenance operation.
             * On a asynchronous Data Abort on a translation table walk, this bit is UNKNOWN .
             * On an asynchronous fault, this bit is UNKNOWN .
             *
             * Bit[13], if implementation does not include the Large Physical Address Extension
             * Reserved, UNK/SBZP.
             */
            bool cm: 1;
            /**
             * Bits[31:14]
             * Reserved, UNK/SBZP.
             */
            uint32_t reserved0: 18;
        } b;
    };
} dfsr_reg_t;

/**
 * The IFSR holds status information about the last instruction fault
 */
typedef struct {
    union {
        uint32_t v;
        struct {
            /**
             * FS, bits[10, 3:0]
             * Fault status bits. For the valid encodings of these bits when using the Short-descriptor translation
             * table format, see Table B3-23 on page B3-1415. All encodings not shown in the table are reserved.
             */
            uint8_t fs_l: 4;
            /**
             * Bits[8:4]
             * Reserved, UNK/SBZP.
             */
            uint8_t reserved2: 4;
            uint8_t reserved3: 1;
            /**
             * LPAE, bit[9], if the implementation includes the Large Physical Address Extension
             * On taking an exception, this bit is set to 0 to indicate use of the Short-descriptor translation table
             * format.
             * Bits[9], if the implementation does not include the Large Physical Address Extension
             */
            bool lpae: 1;
            /**
             * FS, bits[10, 3:0]
             * Fault status bits. For the valid encodings of these bits when using the Short-descriptor translation
             * table format, see Table B3-23 on page B3-1415. All encodings not shown in the table are reserved.
             */
            uint8_t fs_h: 1;
            /**
             * Bit[11]
             * Reserved, UNK/SBZP.
             */
            bool reserved1: 1;
            /**
             * ExT, bit[12] External abort type. This bit can provide an IMPLEMENTATION DEFINED classification of external
             * aborts.
             * For aborts other than external aborts this bit always returns 0.
             * In an implementation that does not provide any classification of external aborts, this bit is
             * UNK/SBZP.
             */
            bool ext: 1;
            /**
             * Bits[31:13] Reserved, UNK/SBZP.
             */
            uint32_t reserved0: 19;
        } b;
    };
} ifsr_reg_t;

/**
 * The Current Program Status Register (CPSR) holds processor status and control information.
 *
 * Note: This is only intended to be used for parsing the cpsr register value read via the mrs
 * instruction.
 */
typedef struct {
    union {
        uint32_t v;
        struct {
            /**
             * Mode field. This field determines the current mode of the processor.
             */
            uint8_t mode: 5;
            /**
             * Thumb execution state bit. This bit and the J execution state bit, bit[24],
             * determine the instruction set state of the processor, ARM, Thumb, Jazelle, or ThumbEE
             */
            bool thumb: 1;
            /**
             * FIQ mask bit
             */
            bool fiq: 1;
            /**
             * IRQ mask bit
             */
            bool irq: 1;
            /**
             * Asynchronous abort mask bit
             */
            bool async_abort: 1;
            /**
             * Endianness execution state bit
             */
            bool big_endian: 1;
            /**
             * IT[7:0], bits[15:10, 26:25]
             */
            uint8_t it0: 6;
            /**
             * Greater than or Equal flags, for the parallel addition and subtraction (SIMD) instructions
             */
            uint8_t ge: 4;
            uint8_t reserved0 : 4;
            /**
             * Jazelle execution state bit. This bit and the T execution state bit, bit[5],
             * determine the instruction set state of the processor, ARM, Thumb, Jazelle, or ThumbEE
             */
            bool jazelle: 1;
            /**
             * IT[7:0], bits[15:10, 26:25]
             * If-Then execution state bits for the Thumb IT (If-Then) instruction
             */
            uint8_t it1: 2;
            /**
             * Cumulative saturation bit
             */
            bool q: 1;
            /**
             * Overflow condition flag
             */
            bool v: 1;
            /**
             * Carry condition flag
             */
            bool c: 1;
            /**
             * Zero condition flag
             */
            bool z: 1;
            /**
             * Negative condition flag
             */
            bool n: 1;
        } b;
    };
} regpsr_t;

typedef struct {
    int32_t regs[13];
    regsp_t sp;
    regret_t lr;
    regpc_t pc;
    regpsr_t psr;
} regs_t;

/**
 * Note: const because changing this value will have no effect on the actual register
 *
 * @return: Current program status register
 */
static inline const regpsr_t get_cpsr(void) {
    regpsr_t cpsr;
    asm("mrs %0, cpsr" : "=r" (cpsr.v));
    return cpsr;
}

static inline void set_cpsr(regpsr_t *psr) {
    asm("msr cpsr, %0" : : "r" (psr->v));
}

/**
 * Note: const because changing this value will have no effect on the actual register
 *
 * @return: Saved program status register
 */
static inline const regpsr_t get_spsr(void) {
    regpsr_t spsr;
    asm("mrs %0, spsr" : "=r" (spsr.v));
    return spsr;
}

/**
 * Note: __attribute__((always_inline)) so that this function is always expanded and thus
 * we get a useful PC, not just the PC inside the arch_regs_get_pc() function (in case it
 * wasn't expanded by the compiler)
 *
 * @return: Current program counter
 */
__attribute__((always_inline)) static inline regpc_t arch_regs_get_pc(void) {
    regpc_t pc;
    asm("mov %0, pc" : "=r" (pc));
    return (char *) pc - 4;
}

/**
 * Note: __attribute__((always_inline)) so that this function is always expanded and thus
 * we get a useful return value, not just the return address of the arch_regs_get_pc() function (in case it
 * wasn't expanded by the compiler)
 *
 * @return: Function return address
 */
__attribute__((always_inline)) static inline regret_t arch_regs_get_retaddr(void) {
    regret_t ret;
    asm("mov %0, lr" : "=r" (ret));
    return ret;
}

/**
 * Indicates the processor number in the Cortex-Ax processor:
 *
 * @return:
 *      0x0                 CPU ID for one processor.
 *      0x0, 0x1            CPU ID for two processors.
 *      0x0, 0x1, 0x2       CPU ID for three processors.
 *      0x0, 0x1, 0x2, 0x3  CPU ID for four processors.
 */
static inline uint8_t arch_cpu_get_id(void) {
    uint32_t v;
    // Read Multiprocessor Affinity CP15 Register
    asm("mrc p15, 0, %0, c0, c0, 5" : "=r" (v));
    return v & 0b11;
}

static inline void arch_regs_set_got(void *got, regs_t *regs) {
    regs->regs[9] = (int32_t) got;
}

static inline void arch_regs_set_stack(void *stack, regs_t *regs) {
    regs->sp = stack;
}

static inline void arch_regs_set_pc(void *pc, regs_t *regs) {
    regs->pc = pc;
}

static inline void arch_regs_set_usermode(regs_t *regs) {
    // User mode
    regs->psr.b.mode = 0b10000;
}

static inline void arch_regs_set_irq_on(regs_t *regs) {
    // IRQ enabled (not masked)
    regs->psr.b.irq = 0;
    // FIQ disabled
    regs->psr.b.fiq = 1;
}

static inline void arch_wfi(void) {
    asm("wfi");
}
