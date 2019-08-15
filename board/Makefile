board-dir = board/$(subst $\",,$(CONFIG_SYS_BOARD))

core-y += $(board-dir)/

KBUILD_LDS := $(board-dir)/os_memmap.lds

quiet_cmd_prep_ldss ?= PREP    $@
	cmd_prep_ldss ?= $(CPP) $(cpp_flags) -I $(LINUXINCLUDE) -P -U$(ARCH) -D__ASSEMBLY__ -DLINKER_SCRIPT -o $@ $<

$(KBUILD_LDS): $(KBUILD_LDS).S FORCE
	$(call if_changed,prep_ldss)

CLEAN_FILES += $(KBUILD_LDS)

all: laritos
