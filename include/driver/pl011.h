#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * The UARTIMSC Register is the interrupt mask set/clear register.
 * It is a read/write register
 */
typedef volatile struct {
    union {
        uint32_t v;
        struct {
            // nUARTRI modem interrupt mask
            bool rimim: 1;
            // nUARTCTS modem interrupt mask
            bool ctsmim: 1;
            // nUARTDCD modem interrupt mask
            bool dcdmim: 1;
            // nUARTDSR modem interrupt mask
            bool dsrmim: 1;
            /**
             * Receive interrupt mask
             *
             * The receive interrupt changes state when one of the following events occurs:
             *      * If the FIFOs are enabled and the receive FIFO reaches the programmed trigger
             * level. When this happens, the receive interrupt is asserted HIGH. The receive
             * interrupt is cleared by reading data from the receive FIFO until it becomes less
             * than the trigger level, or by clearing the interrupt.
             *      * If the FIFOs are disabled (have a depth of one location) and data is received
             * thereby filling the location, the receive interrupt is asserted HIGH. The receive
             * interrupt is cleared by performing a single read of the receive FIFO, or by clearing
             * the interrupt.
             */
            bool rxim: 1;
            /**
             * Transmit interrupt mask
             *
             * The transmit interrupt changes state when one of the following events occurs:
             *      * If the FIFOs are enabled and the transmit FIFO is equal to or lower than the
             *      programmed trigger level then the transmit interrupt is asserted HIGH. The
             *      transmit interrupt is cleared by writing data to the transmit FIFO until it becomes
             *      greater than the trigger level, or by clearing the interrupt.
             *      * If the FIFOs are disabled (have a depth of one location) and there is no data
             *      present in the transmitters single location, the transmit interrupt is asserted HIGH.
             *      It is cleared by performing a single write to the transmit FIFO, or by clearing the
             *      interrupt
             */
            bool txim: 1;
            // Receive timeout interrupt mask
            bool rtim: 1;
            // Framing error interrupt mask
            bool feim: 1;
            // Parity error interrupt mask
            bool peim: 1;
            // Break error interrupt mask
            bool beim: 1;
            // Overrun error interrupt mask
            bool oeim: 1;
            uint32_t reserved: 21;
        } b;
    };
} int_mask_ctrl_t;

/* Register map for the UART PL011.
 *
 * UART memory map and registers based on:
 *      http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183g/DDI0183G_uart_pl011_r1p5_trm.pdf
 *
 * NOTE:
 *      The base address of the UART is not fixed, and can be different for any particular
 *      system implementation. The offset of each register from the base address is fixed.
 */
typedef volatile struct {
    // Data Register, UARTDR on page 3-5
    uint32_t dr;
    // Receive Status Register/Error Clear Register, UARTRSR/UARTECR on page 3-6
    uint32_t rsr_ecr;
    const uint8_t reserved0[16];
    // Flag Register, UARTFR on page 3-8
    const union {
        uint32_t v;
        struct {
            //  Clear to send
            uint8_t cts: 1;
            // Data set ready
            uint8_t dsr: 1;
            // Data carrier detect
            uint8_t dcd: 1;
            // UART busy. If this bit is set to 1, the UART is busy transmitting data. This bit remains set until the
            // complete byte, including all the stop bits, has been sent from the shift register.
            // This bit is set as soon as the transmit FIFO becomes non-empty, regardless of whether the UART is
            // enabled or not.
            uint8_t busy: 1;
            // Receive FIFO empty. The meaning of this bit depends on the state of the FEN bit in the UARTLCR_H
            // Register.
            // If the FIFO is disabled, this bit is set when the receive holding register is empty.
            // If the FIFO is enabled, the RXFE bit is set when the receive FIFO is empty.
            uint8_t rxfe: 1;
            // Transmit FIFO full. The meaning of this bit depends on the state of the FEN bit in the UARTLCR_H
            // Register.
            // If the FIFO is disabled, this bit is set when the transmit holding register is full.
            // If the FIFO is enabled, the TXFF bit is set when the transmit FIFO is full
            uint8_t txff: 1;
            // Receive FIFO full. The meaning of this bit depends on the state of the FEN bit in the UARTLCR_H
            // Register.
            // If the FIFO is disabled, this bit is set when the receive holding register is full.
            // If the FIFO is enabled, the RXFF bit is set when the receive FIFO is full.
            uint8_t rxff: 1;
            // Transmit FIFO empty. The meaning of this bit depends on the state of the FEN bit in the Line Control
            // Register, UARTLCR_H on page 3-12.
            // If the FIFO is disabled, this bit is set when the transmit holding register is empty.
            // If the FIFO is enabled, the TXFE bit is set when the transmit FIFO is empty.
            // This bit does not indicate if there is data in the transmit shift register.
            uint8_t txfe: 1;
            // Ring indicator
            uint8_t ri: 1;
        } b;
    } fr;
    uint8_t reserved1[4];
    // IrDA Low-Power Counter Register, UARTILPR on page 3-9
    uint32_t ilpr;
    // Integer Baud Rate Register, UARTIBRD on page 3-9
    uint32_t ibdr;
    // Fractional Baud Rate Register, UARTFBRD on page 3-10
    uint32_t fbdr;
    // Line Control Register, UARTLCR_H on page 3-12
    uint32_t lcr_h;
    // Control Register, UARTCR on page 3-15
    uint32_t cr;
    // Interrupt FIFO Level Select Register, UARTIFLS on page 3-17
    uint32_t ifls;
    // Interrupt Mask Set/Clear Register, UARTIMSC on page 3-17
    int_mask_ctrl_t imsc;
    //Raw Interrupt Status Register, UARTRIS on page 3-19
    const int_mask_ctrl_t ris;
    // Masked Interrupt Status Register, UARTMIS on page 3-20
    const int_mask_ctrl_t mis;
    // Interrupt Clear Register, UARTICR on page 3-21
    int_mask_ctrl_t icr;
    // DMA Control Register, UARTDMACR on page 3-22
    uint32_t dmacr;
} pl011_mm_t;
