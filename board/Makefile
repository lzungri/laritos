board-dir = board/$(subst $\",,$(CONFIG_SYS_BOARD))

core-y += $(board-dir)/

KBUILD_LDS := $(board-dir)/os_memmap.lds

quiet_cmd_prep_ldss ?= PREP    $@
	cmd_prep_ldss ?= $(CPP) $(cpp_flags) -I $(LINUXINCLUDE) -P -U$(ARCH) -D__ASSEMBLY__ -DLINKER_SCRIPT -o $@ $<

$(KBUILD_LDS): $(KBUILD_LDS).S FORCE
	$(call if_changed,prep_ldss)

# Targets to embed the board information into the final elf
KBUILD_BI := $(board-dir)/$(subst $\",,$(CONFIG_SYS_BOARD_INFO_FILE))
KBUILD_BOARD_INFO := $(objtree)/boardinfo.o
KBUILD_BI_COPIED := $(basename $(KBUILD_BOARD_INFO))

quiet_cmd_build_bi ?= LD      $@
	cmd_build_bi ?= $(LD) -r -b binary $< -o $@

quiet_cmd_cp_to_root ?= CP      $@
	cmd_cp_to_root ?= cp $< $@

# HACK: Copy the boardinfo file into the root folder and rename it so that the ld auto-generated symbols
# don't look that ugly (e.g. _binary____board_qemu_arm_virt_default_bi_start -> _binary_boardinfo_start)
$(KBUILD_BI_COPIED): $(KBUILD_BI) FORCE
	$(call if_changed,cp_to_root)

# Generate an ELF binary from the board info plain-text file
$(KBUILD_BOARD_INFO): $(KBUILD_BI_COPIED) FORCE
	$(call if_changed,build_bi)


CLEAN_FILES += $(KBUILD_LDS) $(KBUILD_BOARD_INFO)


all: laritos
