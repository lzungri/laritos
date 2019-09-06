#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * Structure mapping the set of registers pushed into the
 * stack by the handlers
 */
typedef struct {
    int32_t r[12];
    int32_t sp;
    int32_t lr;
} spregs_t;

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
            uint8_t reserved2: 1;
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


static inline int32_t get_spsr(void) {
    int32_t spsr;
    asm("mrs %0, spsr" : "=r" (spsr));
    return spsr;
}
