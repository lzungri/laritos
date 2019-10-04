#
# arch/arm/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#

LDFLAGS_laritos	:= --no-undefined -X --pic-veneer

# This should work on most of the modern platforms
KBUILD_DEFCONFIG := qemu_arm_virt_defconfig

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
KBUILD_CPPFLAGS	+= -mbig-endian
CHECKFLAGS	+= -D__ARMEB__
AS		+= -EB
KBUILD_LDFLAGS	+= -EB
else
KBUILD_CPPFLAGS	+= -mlittle-endian
CHECKFLAGS	+= -D__ARMEL__
AS		+= -EL
KBUILD_LDFLAGS	+= -EL
endif

# This selects which instruction set is used.
# Note that GCC does not numerically define an architecture version
# macro, but instead defines a whole series of macros which makes
# testing for a specific architecture or later rather impossible.
arch-$(CONFIG_ARMV7_A) = -march=armv7-a -Wa,-march=armv7-a
arch-$(CONFIG_ARMV8_A) = -march=armv8-a -Wa,-march=armv8-a

# Evaluate arch cc-option calls now
arch-y := $(arch-y)

CFLAGS_ISA	:=$(call cc-option,-marm,) $(AFLAGS_NOWARN)
AFLAGS_ISA	:=$(CFLAGS_ISA)

# Need -Uarm for gcc < 3.x
KBUILD_CFLAGS	+=$(CFLAGS_ABI) $(CFLAGS_ISA) $(arch-y) $(call cc-option,-mshort-load-bytes,$(call cc-option,-malignment-traps,)) -msoft-float -Uarm
KBUILD_AFLAGS	+=$(CFLAGS_ABI) $(AFLAGS_ISA) $(arch-y) -msoft-float

CHECKFLAGS	+= -D__arm__

# Select target cpu
cpu-$(CONFIG_SYS_CPU_GENERIC_ARMV7A) = armv7-a
cpu-$(CONFIG_SYS_CPU_CORTEX_A9) = cortex-a9
cpu-$(CONFIG_SYS_CPU_CORTEX_A15) = cortex-a15
KBUILD_CFLAGS += -I$(srctree)/arch/arm/$(cpu-y)/include

# Select target cpu family
cpu-family-$(CONFIG_ARMV7_A) = armv7-a
cpu-family-$(CONFIG_ARMV8_A) = armv8-a
KBUILD_CFLAGS += -I$(srctree)/arch/arm/$(cpu-family-y)/include

# Add cpu dir to the core builder
core-y += arch/arm/$(cpu-y)/
