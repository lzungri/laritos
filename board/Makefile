machine-$(CONFIG_BOARD_QEMU_ARM_VIRT) += qemu-arm-virt
machine-$(CONFIG_BOARD_QEMU_AARCH64_VIRT) += qemu-aarch64-virt

# The first directory contains additional information for the boot setup code
#ifneq ($(machine-y),)
#MACHINE  := arch/arm/mach-$(word 1,$(machine-y))/
#else
#MACHINE  :=
#endif
#ifeq ($(CONFIG_ARCH_MULTIPLATFORM),y)
#MACHINE  :=
#endif

machdirs := $(patsubst %,board/%/,$(machine-y))

core-y += $(machdirs)

all:	laritos

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

