board-dir = board/$(subst $\",,$(CONFIG_SYS_BOARD))

core-y += $(board-dir)/


KBUILD_LDS := $(board-dir)/os_memmap.lds

quiet_cmd_prep_ldss ?= PREP    $@
	cmd_prep_ldss ?= $(CPP) $(cpp_flags) -I $(LINUXINCLUDE) -P -U$(ARCH) -D__ASSEMBLY__ -DLINKER_SCRIPT -o $@ $<

$(KBUILD_LDS): $(KBUILD_LDS).S FORCE
	$(call if_changed,prep_ldss)

all: laritos

## Convert bzImage to zImage
#bzImage: zImage
#
#BOOT_TARGETS	= zImage Image xipImage bootpImage uImage
#INSTALL_TARGETS	= zinstall uinstall install
#
#PHONY += bzImage $(BOOT_TARGETS) $(INSTALL_TARGETS)
#
#bootpImage uImage: zImage
#zImage: Image
#
#$(BOOT_TARGETS): laritos
#	$(Q)$(MAKE) $(build)=$(boot) MACHINE=$(MACHINE) $(boot)/$@
#	@$(kecho) '  Kernel: $(boot)/$@ is ready'
#
#$(INSTALL_TARGETS):
#	$(Q)$(MAKE) $(build)=$(boot) MACHINE=$(MACHINE) $@
#
# We use MRPROPER_FILES and CLEAN_FILES now
#archclean:
#	$(Q)$(MAKE) $(clean)=$(boot)

