#include <log.h>

#include <process/core.h>
#include <process/validation.h>


bool process_is_valid_kernel_exec_addr_locked(void *addr) {
    extern void *__text_start[];
    extern void *__text_end[];
    return addr >= (void *) __text_start && addr < (void *) __text_end;
}

bool process_is_valid_exec_addr_locked(pcb_t *pcb, void *addr) {
    return addr >= pcb->mm.text_start && (char *) addr < (char *) pcb->mm.text_start + pcb->mm.text_size;
}

bool process_is_valid_context_locked(pcb_t *pcb, spctx_t *ctx) {
    // TODO Add more validations (e.g. mode, lr)
    void *retaddr = arch_context_get_retaddr(ctx);

    // If it is a kernel process or it is currently running in kernel mode, then
    // validate if the address is within the kernel text address space
    if (pcb->kernel || arch_context_is_kernel(ctx)) {
        return process_is_valid_kernel_exec_addr_locked(retaddr);
    }
    return process_is_valid_exec_addr_locked(pcb, retaddr);
}
