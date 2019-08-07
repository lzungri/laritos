#
# arch/arm/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995-2001 by Russell King

LDFLAGS_laritos	:= --no-undefined -X --pic-veneer
ifeq ($(CONFIG_CPU_ENDIAN_BE8),y)
LDFLAGS_laritos	+= --be8
KBUILD_LDFLAGS_MODULE	+= --be8
endif

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

core-y += arch/arm/cpu/
