#include <log.h>

#include <libc/string.h>
#include <loader/loader.h>
#include <loader/elf.h>
#include <loader/loader-elf.h>
#include <process/core.h>
#include <sync/spinlock.h>


/**
 * Offset in laritos.bin where the apps are loaded.
 * Apps are (for now) appended to laritos.bin via the tools/apps/install.sh script
 * This is temporary until we implement a better mechanism for flashing apps,
 * such as a file system on sd card.
 */
extern char __apps_start[];

pcb_t *loader_load_executable_from_memory(uint16_t appidx) {
    unsigned char *e_ident = __apps_start;

    debug("Loading app at 0x%p", e_ident);

    if (memcmp(e_ident, ELFMAG, sizeof(ELFMAG) - 1) != 0) {
        error("Executable format not recognized");
        return NULL;
    }


    pcb_t *pcb = NULL;

    switch (e_ident[EI_CLASS]) {
    case 1:;
        // ELF 32
        Elf32_Ehdr *elf = (Elf32_Ehdr *) e_ident;
        pcb = loader_elf32_load_from_memory(elf);
        break;
    case 2:
        // ELF 64
        error("ELF 64 not supported yet");
        break;
    default:
        // Unknown class
        error("Invalid ELF class %u", e_ident[EI_CLASS]);
        break;
    }

    if (pcb == NULL) {
        return NULL;
    }

    irqctx_t ctx;
    spinlock_acquire(&_laritos.proclock, &ctx);
    if (process_register_locked(pcb) < 0) {
        error_async("Could not register process at 0x%p", pcb);
        process_free(pcb);
    }
    spinlock_release(&_laritos.proclock, &ctx);

    return pcb;
}
