/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <stdbool.h>
#include <loader/elf.h>
#include <arch/elf32.h>
#include <math.h>

/**
 * Relocates symbol on image at address <imgaddr>
 *
 * NOTE: Thumb mode not supported
 */
int arch_elf32_relocate_symbol(void *imgaddr, const Elf32_Rel *rel, const Elf32_Sym *sym, uint32_t *got) {
    char *imgbase = imgaddr;

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
    case R_ARM_V4BX:
        /**
         * According to ARM ld:
         *    The ‘R_ARM_V4BX’ relocation (defined by the ARM AAELF specification) enables objects compiled for the ARMv4
         *    architecture to be interworking-safe when linked with other objects compiled for ARMv4t, but also allows pure
         *    ARMv4 binaries to be built from the same ARMv4 objects.
         *
         *    In the latter case, the switch --fix-v4bx must be passed to the linker, which causes v4t BX rM instructions to
         *    be rewritten as MOV PC,rM, since v4 processors do not have a BX instruction.
         *
         *    In the former case, the switch should not be used, and ‘R_ARM_V4BX’ relocations are ignored.
         */

        // Since we are targeting the armv7-a architecture, which supports bx instructions, we just ignore this relocation
        break;
    case R_ARM_JUMP24:
        // R_ARM_JUMP24 and R_ARM_CALL have different behavior when using THUMB mode. Since we only use ARM mode, we can
        // treat them the same way.
    case R_ARM_CALL:
        // Ignore this type of relocation, they are already resolved
        break;

        /**
         * Instruction | REL Addend (A)
         * BL, BLX     | sign_extend (insn[23:0] << 2)
         *
         * X = ((S + A) | T) – P
         * X = X & 0x03FFFFFE
         */
        uint32_t S = sym->st_value;
        uint32_t *P = (uint32_t *) (imgbase + rel->r_offset);
        int32_t A = sign_extend_32(*P & 0x00FFFFFF, 24) << 2;

        int32_t X = (S + A) - (uint32_t) P;
        X = (X & 0x03FFFFFE) >> 2;

        *P = (*P & 0xFF000000) | (X & 0x00FFFFFF);
        break;
    case R_ARM_GOT32:
    case R_ARM_GOT_BREL12:;
        /**
         * From ARM docs: Offset of the GOT entry from the GOT origin. Stored in the offset field of an ARM LDR instruction
         */

        // Get the got index of the symbol
        uint32_t gotidx = *((uint32_t *) (imgbase + rel->r_offset)) / 4;
        // Update the got associated with that symbol to point to the right offset starting from imgbase
        got[gotidx] = (uint32_t) (imgbase + sym->st_value);
        break;
    case R_ARM_ABS32:;
        // Get the relocation address
        uint32_t *addr = (uint32_t *) (imgbase + rel->r_offset);
        // Add the imgbase to the offset
        *addr += (uint32_t) imgbase;
        break;
    default:
        error("Unknown relocation type=0x%lX at offset=0x%lX for symbol value=0x%lX", ELF32_R_TYPE(rel->r_info), rel->r_offset, sym->st_value);
        return -1;
    }

    return 0;
}
