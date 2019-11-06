#include <log.h>

#include <string.h>
#include <arch/cpu.h>
#include <process/pcb.h>

void arch_user_stack_init(struct pcb *pcb, void *lr) {
    /**
     * Stack layout:
     *
     *  sp (new) ->  spsr
     *               lr
     *               r0
     *               r1
     *               r2
     *               r3
     *               r4
     *               r5
     *               r6
     *               r7
     *               r8
     *               r9
     *               r10
     *               r11
     *               r12
     *  sp (old) ->  r13  ---points to--,
     *               xxx <--------------'
     */
    uint32_t *sp = (uint32_t *) ((char *) pcb->mm.stack_bottom + pcb->mm.stack_size - 4);

    // Clear all general-purpose regs
    memset(sp - 13, 0, sizeof(uint32_t) * 14);

    // Stack pointer at r13 (here is where the sp will be located right
    // after returning to user space)
    sp[0] = (uint32_t) (sp + 1);

    // GOT at r9
    sp[-4] = (uint32_t) pcb->mm.got_start;
    sp -= 14;

    // LR
    *sp-- = (uint32_t) lr;

    regpsr_t psr = { 0 };
    // User mode
    psr.b.mode = 0b10000;
    // IRQ enabled (not masked)
    psr.b.irq = 0;
    // FIQ disabled
    psr.b.fiq = 1;
    // SPSR
    *sp = psr.v;

    // Setup process stack pointer
    pcb->mm.sp = sp;
}
