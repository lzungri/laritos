#include <log.h>

#include <stdbool.h>
#include <loader/elf.h>
#include <arch/elf32.h>
#include <utils/math.h>

/**
 * Relocate symbol on image at address <imgaddr>
 *
 * NOTE: Thumb mode not supported
 */
int arch_elf32_relocate_symbol(void *imgaddr, const Elf32_Rel *rel, const Elf32_Sym *sym) {

    /**
     * From ARM IHI0044F_aaelf.pdf document.
     *
     * The following nomenclature is used for the operation:
     * 	  - S (when used on its own) is the address of the symbol.
     * 	  - A is the addend for the relocation.
     * 	  - P is the address of the place being relocated (derived from r_offset).
     * 	  - Pa is the adjusted address of the place being relocated, defined as (P & 0xFFFFFFFC).
     * 	  - T is 1 if the target symbol S has type STT_FUNC and the symbol addresses a Thumb instruction; it is 0
     * 	    otherwise.
     * 	  - B(S) is the addressing origin of the output segment defining the symbol S. The origin is not required to be the
     * 	    base address of the segment. This value must always be word-aligned.
     * 	  - GOT_ORG is the addressing origin of the Global Offset Table (the indirection table for imported data
     * 	    addresses). This value must always be word-aligned. See §4.6.1.8, Proxy generating relocations.
     * 	  - GOT(S) is the address of the GOT entry for the symbol S.
     *
     * 	  X is the 32-bit result of normal relocation processing
     */
    switch (ELF32_R_TYPE(rel->r_info)) {
    case R_ARM_CALL:;
        /**
         * Instruction | REL Addend (A)
         * BL, BLX     | sign_extend (insn[23:0] << 2)
         *
         * X = ((S + A) | T) – P
         */
        uint32_t S = sym->st_value;
        uint32_t *P = (uint32_t *) ((char *) imgaddr + rel->r_offset);
        int32_t A = sign_extend_32(*P & 0x00FFFFFF, 24) << 2;

        int32_t X = (S + A) - (uint32_t) P;
        X >>= 2;
        X &= 0x00FFFFFF;

        *P = (*P & 0xFF000000) | (X & 0x00FFFFFF);
        break;
    default:
        error("Unknown relocation type=0x%lX at offset=0x%lX for symbol value=0x%lX", ELF32_R_TYPE(rel->r_info), rel->r_offset, sym->st_value);
//        return -1;
    }

    return 0;
}
