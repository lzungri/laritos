# SPDX-License-Identifier: GPL-2.0
VERSION = 0
PATCHLEVEL = 3
SUBLEVEL = 0
EXTRAVERSION =
NAME = laritOS

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.

# That's our default target when none is given on the command line
PHONY := _all
_all:

# We are using a recursive build, so we need to do a little thinking
# to get the ordering right.
#
# Most importantly: sub-Makefiles should only ever modify files in
# their own directory. If in some directory we have a dependency on
# a file in another dir (which doesn't happen often, but it's often
# unavoidable when linking the built-in.a targets which finally
# turn into vmlinux), we will call a sub make in that other dir, and
# after that we are sure that everything which is in that other dir
# is now up to date.
#
# The only cases where we need to modify files which have global
# effects are thus separated out and done before the recursive
# descending is started. They are now explicitly listed as the
# prepare rule.

ifneq ($(sub_make_done),1)

# Do not use make's built-in rules and variables
# (this increases performance and avoids hard-to-debug behaviour)
MAKEFLAGS += -rR

# Avoid funny character set dependencies
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

# Avoid interference with shell env settings
unexport GREP_OPTIONS

# Beautify output
# ---------------------------------------------------------------------------
#
# Normally, we echo the whole command before executing it. By making
# that echo $($(quiet)$(cmd)), we now have the possibility to set
# $(quiet) to choose other forms of output instead, e.g.
#
#         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
#         cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<
#
# If $(quiet) is empty, the whole command will be printed.
# If it is set to "quiet_", only the short version will be printed.
# If it is set to "silent_", nothing will be printed at all, since
# the variable $(silent_cmd_cc_o_c) doesn't exist.
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#	$(Q)ln $@ :<
#
# If KBUILD_VERBOSE equals 0 then the above command will be hidden.
# If KBUILD_VERBOSE equals 1 then the above command is displayed.
#
# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(findstring s,$(filter-out --%,$(MAKEFLAGS))),)
  quiet=silent_
endif

export quiet Q KBUILD_VERBOSE

# Kbuild will save output files in the current working directory.
# This does not need to match to the root of the kernel source tree.
#
# For example, you can do this:
#
#  cd /dir/to/store/output/files; make -f /dir/to/kernel/source/Makefile
#
# If you want to save output files in a different location, there are
# two syntaxes to specify it.
#
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the output directory.
# export KBUILD_OUTPUT=dir/to/store/output/files/; make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.

# Do we want to change the working directory?
ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
else
  # Set 'bin' as the default directory for laritOS
  KBUILD_OUTPUT := bin
endif

ifneq ($(KBUILD_OUTPUT),)
# Make's built-in functions such as $(abspath ...), $(realpath ...) cannot
# expand a shell special character '~'. We use a somewhat tedious way here.
abs_objtree := $(shell mkdir -p $(KBUILD_OUTPUT) && cd $(KBUILD_OUTPUT) && pwd)
$(if $(abs_objtree),, \
     $(error failed to create output directory "$(KBUILD_OUTPUT)"))

# $(realpath ...) resolves symlinks
abs_objtree := $(realpath $(abs_objtree))
else
abs_objtree := $(CURDIR)
endif # ifneq ($(KBUILD_OUTPUT),)

ifeq ($(abs_objtree),$(CURDIR))
# Suppress "Entering directory ..." unless we are changing the work directory.
MAKEFLAGS += --no-print-directory
else
need-sub-make := 1
endif

abs_srctree := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

ifneq ($(words $(subst :, ,$(abs_srctree))), 1)
$(error source directory cannot contain spaces or colons)
endif

ifneq ($(abs_srctree),$(abs_objtree))
# Look for make include files relative to root of kernel src
#
# This does not become effective immediately because MAKEFLAGS is re-parsed
# once after the Makefile is read. We need to invoke sub-make.
MAKEFLAGS += --include-dir=$(abs_srctree)
need-sub-make := 1
endif

ifneq ($(filter 3.%,$(MAKE_VERSION)),)
# 'MAKEFLAGS += -rR' does not immediately become effective for GNU Make 3.x
# We need to invoke sub-make to avoid implicit rules in the top Makefile.
need-sub-make := 1
# Cancel implicit rules for this Makefile.
$(lastword $(MAKEFILE_LIST)): ;
endif

export abs_srctree abs_objtree
export sub_make_done := 1

ifeq ($(need-sub-make),1)

PHONY += $(MAKECMDGOALS) sub-make

$(filter-out _all sub-make $(lastword $(MAKEFILE_LIST)), $(MAKECMDGOALS)) _all: sub-make
	@:

# Invoke a second make in the output directory, passing relevant variables
sub-make:
	$(Q)$(MAKE) -C $(abs_objtree) -f $(abs_srctree)/Makefile $(MAKECMDGOALS)

endif # need-sub-make
endif # sub_make_done

# We process the rest of the Makefile if this is the final invocation of make
ifeq ($(need-sub-make),)

# Do not print "Entering directory ...",
# but we want to display it when entering to the output directory
# so that IDEs/editors are able to understand relative filenames.
MAKEFLAGS += --no-print-directory

# Call a source code checker (by default, "sparse") as part of the
# C compilation.
#
# Use 'make C=1' to enable checking of only re-compiled files.
# Use 'make C=2' to enable checking of *all* source files, regardless
# of whether they are re-compiled or not.
#
# See the file "Documentation/dev-tools/sparse.rst" for more details,
# including where to get the "sparse" utility.

ifeq ("$(origin C)", "command line")
  KBUILD_CHECKSRC = $(C)
endif

export KBUILD_CHECKSRC

ifeq ($(abs_srctree),$(abs_objtree))
        # building in the source tree
        srctree := .
	building_out_of_srctree :=
else
        ifeq ($(abs_srctree)/,$(dir $(abs_objtree)))
                # building in a subdirectory of the source tree
                srctree := ..
        else
                srctree := $(abs_srctree)
        endif
	building_out_of_srctree := 1
endif

ifneq ($(KBUILD_ABS_SRCTREE),)
srctree := $(abs_srctree)
endif

objtree		:= .
VPATH		:= $(srctree)

export building_out_of_srctree srctree objtree VPATH

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

version_h := include/generated/laritos/version.h
old_version_h := include/linux/version.h

clean-targets := %clean mrproper cleandocs
no-dot-config-targets := $(clean-targets) \
			 cscope help% %docs check% coccicheck \
			 $(version_h) headers headers_% \
			 %asm-generic kernelversion %src-pkg
no-sync-config-targets := $(no-dot-config-targets) install %install \
			   kernelrelease

config-targets  := 0
mixed-targets   := 0
dot-config      := 1
may-sync-config := 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		dot-config := 0
	endif
endif

ifneq ($(filter $(no-sync-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-sync-config-targets), $(MAKECMDGOALS)),)
		may-sync-config := 0
	endif
endif

ifneq ($(filter config %config,$(MAKECMDGOALS)),)
        config-targets := 1
        ifneq ($(words $(MAKECMDGOALS)),1)
                mixed-targets := 1
        endif
endif

# For "make -j clean all", "make -j mrproper defconfig all", etc.
ifneq ($(filter $(clean-targets),$(MAKECMDGOALS)),)
        ifneq ($(filter-out $(clean-targets),$(MAKECMDGOALS)),)
                mixed-targets := 1
        endif
endif

ifeq ($(mixed-targets),1)
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.

PHONY += $(MAKECMDGOALS) __build_one_by_one

$(filter-out __build_one_by_one, $(MAKECMDGOALS)): __build_one_by_one
	@:

__build_one_by_one:
	$(Q)set -e; \
	for i in $(MAKECMDGOALS); do \
		$(MAKE) -f $(srctree)/Makefile $$i; \
	done

else

include scripts/Kbuild.include

# Read KERNELRELEASE from include/config/kernel.release (if it exists)
KERNELRELEASE = $(shell cat include/config/kernel.release 2> /dev/null)
KERNELVERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))$(EXTRAVERSION)
export VERSION PATCHLEVEL SUBLEVEL KERNELRELEASE KERNELVERSION

include scripts/subarch.include


# Include the config file (if exists) and set the architecture for the board
-include include/config/auto.conf

ifeq ($(CONFIG_ARM),y)
ARCH = arm
else
ifeq ($(CONFIG_AARCH64),y)
ARCH = aarch64
endif
endif

# Cross compiling and selecting different set of gcc/bin-utils
# ---------------------------------------------------------------------------
#
# When performing cross compilation for other architectures ARCH shall be set
# to the target architecture. (See arch/* for the possibilities).
# ARCH can be set during invocation of make:
# make ARCH=ia64
# Another way is to have ARCH set in the environment.
# The default ARCH is the host where make is executed.

# CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CROSS_COMPILE).
# CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=ia64-linux-
# Alternatively CROSS_COMPILE can be set in the environment.
# Default value for CROSS_COMPILE is not to prefix executables
# Note: Some architectures assign CROSS_COMPILE in their arch/*/Makefile
ARCH		?= $(SUBARCH)

# Architecture as present in compile.h
UTS_MACHINE 	:= $(ARCH)
SRCARCH 	:= $(ARCH)

KCONFIG_CONFIG	?= .config
export KCONFIG_CONFIG

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

HOST_LFS_CFLAGS := $(shell getconf LFS_CFLAGS 2>/dev/null)
HOST_LFS_LDFLAGS := $(shell getconf LFS_LDFLAGS 2>/dev/null)
HOST_LFS_LIBS := $(shell getconf LFS_LIBS 2>/dev/null)

HOSTCC       = gcc
HOSTCXX      = g++
KBUILD_HOSTCFLAGS   := -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 \
		-fomit-frame-pointer -std=gnu89 $(HOST_LFS_CFLAGS) \
		$(HOSTCFLAGS)
KBUILD_HOSTCXXFLAGS := -O2 $(HOST_LFS_CFLAGS) $(HOSTCXXFLAGS)
KBUILD_HOSTLDFLAGS  := $(HOST_LFS_LDFLAGS) $(HOSTLDFLAGS)
KBUILD_HOSTLDLIBS   := $(HOST_LFS_LIBS) $(HOSTLDLIBS)

# Make variables (CC, etc...)
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
SIZE		= $(CROSS_COMPILE)size
PAHOLE		= pahole
LEX		= flex
YACC		= bison
AWK		= awk
PERL		= perl
PYTHON		= python
PYTHON2		= python2
PYTHON3		= python3
CHECK		= sparse

NOSTDINC_FLAGS :=
CFLAGS_MODULE   =
AFLAGS_MODULE   =
LDFLAGS_MODULE  =
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=
LDFLAGS_laritos =

# Use USERINCLUDE when you must reference the UAPI directories only.
USERINCLUDE    := -include $(srctree)/include/kconfig.h

# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
LINUXINCLUDE    :=  $(if $(building_out_of_srctree),-I$(srctree)/include/libc) \
		-I$(objtree)/include \
		$(USERINCLUDE)
ifdef CONFIG_TEST_ENABLED
LINUXINCLUDE += $(if $(building_out_of_srctree),-I$(srctree)/test/tests)
endif

KBUILD_AFLAGS   := -D__ASSEMBLY__ -fno-PIE

KBUILD_CFLAGS   := -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs \
		   -fno-strict-aliasing -fno-common -fno-PIE \
		   -Werror=implicit-function-declaration -Werror=implicit-int \
		   -Wno-format-security -std=gnu11

ifdef CONFIG_CC_USE_SHORT_WCHAR
KBUILD_CFLAGS += -fshort-wchar
else
KBUILD_CFLAGS += -fno-short-wchar
endif

# -ffreestanding: Assert that compilation targets a freestanding environment.  This implies -fno-builtin.
# 		A freestanding environment is one in which the standard library may not exist, and program startup may
# 		not necessarily be at "main".  The most obvious example is an OS kernel.  This is equivalent to -fno-hosted.
KBUILD_CFLAGS += -ffreestanding

KBUILD_CPPFLAGS := -D__KERNEL__

KBUILD_AFLAGS_KERNEL :=

KBUILD_CFLAGS_KERNEL :=

KBUILD_LDFLAGS :=

GCC_PLUGINS_CFLAGS :=

export ARCH SRCARCH CONFIG_SHELL HOSTCC KBUILD_HOSTCFLAGS CROSS_COMPILE AS LD CC
export CPP AR NM STRIP OBJCOPY OBJDUMP PAHOLE KBUILD_HOSTLDFLAGS KBUILD_HOSTLDLIBS
export MAKE LEX YACC AWK PERL PYTHON PYTHON2 PYTHON3 UTS_MACHINE
export HOSTCXX KBUILD_HOSTCXXFLAGS LDFLAGS_MODULE CHECK

export KBUILD_CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS KBUILD_LDFLAGS
export KBUILD_CFLAGS CFLAGS_KERNEL CFLAGS_MODULE
export NOSANITIZE CFLAGS_UBSAN
export KBUILD_AFLAGS AFLAGS_KERNEL AFLAGS_MODULE
export KBUILD_AFLAGS_KERNEL KBUILD_CFLAGS_KERNEL
export KBUILD_ARFLAGS

# Files to ignore in find ... statements

export RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o    \
			  -name CVS -o -name .pc -o -name .hg -o -name .git \) \
			  -prune -o
export RCS_TAR_IGNORE := --exclude SCCS --exclude BitKeeper --exclude .svn \
			 --exclude CVS --exclude .pc --exclude .hg --exclude .git

# ===========================================================================
# Rules shared between *config targets and build targets

# Basic helpers built in scripts/basic/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic
	$(Q)rm -f .tmp_quiet_recordmcount

PHONY += outputmakefile
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
# At the same time when output Makefile generated, generate .gitignore to
# ignore whole output directory
outputmakefile:
ifdef building_out_of_srctree
	$(Q)ln -fsn $(srctree) source
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile $(srctree)
	$(Q)test -e .gitignore || \
	{ echo "# this is build directory, ignore it"; echo "*"; } > .gitignore
endif

ifneq ($(shell $(CC) --version 2>&1 | head -n 1 | grep clang),)
ifneq ($(CROSS_COMPILE),)
CLANG_FLAGS	:= --target=$(notdir $(CROSS_COMPILE:%-=%))
GCC_TOOLCHAIN_DIR := $(dir $(shell which $(CROSS_COMPILE)elfedit))
CLANG_FLAGS	+= --prefix=$(GCC_TOOLCHAIN_DIR)
GCC_TOOLCHAIN	:= $(realpath $(GCC_TOOLCHAIN_DIR)/..)
endif
ifneq ($(GCC_TOOLCHAIN),)
CLANG_FLAGS	+= --gcc-toolchain=$(GCC_TOOLCHAIN)
endif
ifeq ($(shell $(AS) --version 2>&1 | head -n 1 | grep clang),)
CLANG_FLAGS	+= -no-integrated-as
endif
CLANG_FLAGS	+= -Werror=unknown-warning-option
KBUILD_CFLAGS	+= $(CLANG_FLAGS)
KBUILD_AFLAGS	+= $(CLANG_FLAGS)
export CLANG_FLAGS
endif

# The expansion should be delayed until arch/$(SRCARCH)/Makefile is included.
# Some architectures define CROSS_COMPILE in arch/$(SRCARCH)/Makefile.
# CC_VERSION_TEXT is referenced from Kconfig (so it needs export),
# and from include/config/auto.conf.cmd to detect the compiler upgrade.
CC_VERSION_TEXT = $(shell $(CC) --version 2>/dev/null | head -n 1)

ifeq ($(config-targets),1)
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

# Read specific board Makefile
include board/Makefile

config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else
# ===========================================================================
# Build targets only - this includes vmlinux, arch specific targets, clean
# targets and others. In general all targets except *config targets.

# If building an external module we do not care about the all: rule
# but instead _all depend on modules
PHONY += all
_all: all

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.

KBUILD_BUILTIN := 1

export KBUILD_BUILTIN

ifeq ($(dot-config),1)
include include/config/auto.conf
endif

# The all: target is the default when no target is given on the
# command line.
# This allow a user to issue only 'make' to build a kernel including modules
# Defaults to vmlinux, but the arch makefile usually adds further targets
all: laritos

CFLAGS_GCOV	:= -fprofile-arcs -ftest-coverage \
	$(call cc-option,-fno-tree-loop-im) \
	$(call cc-disable-warning,maybe-uninitialized,)
export CFLAGS_GCOV

# The arch Makefile can set ARCH_{CPP,A,C}FLAGS to override the default
# values of the respective KBUILD_* variables
ARCH_CPPFLAGS :=
ARCH_AFLAGS :=
ARCH_CFLAGS :=
include arch/$(SRCARCH)/Makefile

# Read specific board Makefile
include board/Makefile

ifeq ($(dot-config),1)
ifeq ($(may-sync-config),1)
# Read in dependencies to all Kconfig* files, make sure to run syncconfig if
# changes are detected. This should be included after arch/$(SRCARCH)/Makefile
# because some architectures define CROSS_COMPILE there.
include include/config/auto.conf.cmd

$(KCONFIG_CONFIG):
	@echo >&2 '***'
	@echo >&2 '*** Configuration file "$@" not found!'
	@echo >&2 '***'
	@echo >&2 '*** Please run some configurator (e.g. "make oldconfig" or'
	@echo >&2 '*** "make menuconfig" or "make xconfig").'
	@echo >&2 '***'
	@/bin/false

# The actual configuration files used during the build are stored in
# include/generated/ and include/config/. Update them if .config is newer than
# include/config/auto.conf (which mirrors .config).
#
# This exploits the 'multi-target pattern rule' trick.
# The syncconfig should be executed only once to make all the targets.
%/auto.conf %/auto.conf.cmd %/tristate.conf: $(KCONFIG_CONFIG)
	$(Q)$(MAKE) -f $(srctree)/Makefile syncconfig
else
# External modules and some install targets need include/generated/autoconf.h
# and include/config/auto.conf but do not care if they are up-to-date.
# Use auto.conf to trigger the test
PHONY += include/config/auto.conf

include/config/auto.conf:
	$(Q)test -e include/generated/autoconf.h -a -e $@ || (		\
	echo >&2;							\
	echo >&2 "  ERROR: Kernel configuration is invalid.";		\
	echo >&2 "         include/generated/autoconf.h or $@ are missing.";\
	echo >&2 "         Run 'make oldconfig && make prepare' on kernel src to fix it.";	\
	echo >&2 ;							\
	/bin/false)

endif # may-sync-config
endif # $(dot-config)

# We include this folder after the arch/xxx/include so that the architecture
# headers have precedence over the core headers.
# This is useful for example when an architecture doesn't implement a particular
# arch_xxx() function and then the OS uses the default one under include/arch/
KBUILD_CFLAGS	+= $(if $(building_out_of_srctree),-I$(srctree)/include)

KBUILD_CFLAGS	+= $(call cc-option,-fno-delete-null-pointer-checks,)
KBUILD_CFLAGS	+= $(call cc-disable-warning,frame-address,)
KBUILD_CFLAGS	+= $(call cc-disable-warning, format-truncation)
KBUILD_CFLAGS	+= $(call cc-disable-warning, format-overflow)
KBUILD_CFLAGS	+= $(call cc-disable-warning, address-of-packed-member)

ifndef CONFIG_CC_DISABLE_OPTIMIZATIONS
ifdef CONFIG_CC_OPTIMIZE_FOR_SIZE
KBUILD_CFLAGS	+= -Os
else
KBUILD_CFLAGS   += -O2
endif
endif

ifdef CONFIG_CC_DISABLE_WARN_MAYBE_UNINITIALIZED
KBUILD_CFLAGS   += -Wno-maybe-uninitialized
endif

# Tell gcc to never replace conditional load with a non-conditional one
KBUILD_CFLAGS	+= $(call cc-option,--param=allow-store-data-races=0)

include scripts/Makefile.gcc-plugins

ifdef CONFIG_READABLE_ASM
# Disable optimizations that make assembler listings hard to read.
# reorder blocks reorders the control in the function
# ipa clone creates specialized cloned functions
# partial inlining inlines only parts of functions
KBUILD_CFLAGS += $(call cc-option,-fno-reorder-blocks,) \
                 $(call cc-option,-fno-ipa-cp-clone,) \
                 $(call cc-option,-fno-partial-inlining)
endif

ifneq ($(CONFIG_FRAME_WARN),0)
KBUILD_CFLAGS += $(call cc-option,-Wframe-larger-than=${CONFIG_FRAME_WARN})
endif

stackp-flags-$(CONFIG_CC_HAS_STACKPROTECTOR_NONE) := -fno-stack-protector
stackp-flags-$(CONFIG_STACKPROTECTOR)             := -fstack-protector
stackp-flags-$(CONFIG_STACKPROTECTOR_STRONG)      := -fstack-protector-strong
KBUILD_CFLAGS += $(stackp-flags-y)

# Benchmarking flags
benchmarking-flags-$(CONFIG_BENCHM_STACK_USAGE) += -fstack-usage
KBUILD_CFLAGS += $(benchmarking-flags-y)

ifdef CONFIG_CC_IS_CLANG
KBUILD_CPPFLAGS += -Qunused-arguments
KBUILD_CFLAGS += -Wno-format-invalid-specifier
KBUILD_CFLAGS += -Wno-gnu
# Quiet clang warning: comparison of unsigned expression < 0 is always false
KBUILD_CFLAGS += -Wno-tautological-compare
# CLANG uses a _MergedGlobals as optimization, but this breaks modpost, as the
# source of a reference will be _MergedGlobals and not on of the whitelisted names.
# See modpost pattern 2
KBUILD_CFLAGS += -mno-global-merge
else

# These warnings generated too much noise in a regular build.
# Use make W=1 to enable them (see scripts/Makefile.extrawarn)
KBUILD_CFLAGS += -Wno-unused-but-set-variable
endif

KBUILD_CFLAGS += $(call cc-disable-warning, unused-const-variable)
ifdef CONFIG_FRAME_POINTER
KBUILD_CFLAGS	+= -fno-omit-frame-pointer -fno-optimize-sibling-calls
else
endif

# Initialize all stack variables with a pattern, if desired.
ifdef CONFIG_INIT_STACK_ALL
KBUILD_CFLAGS	+= -ftrivial-auto-var-init=pattern
endif

DEBUG_CFLAGS	:= $(call cc-option, -fno-var-tracking-assignments)

ifdef CONFIG_DEBUG_INFO
ifdef CONFIG_DEBUG_INFO_SPLIT
DEBUG_CFLAGS	+= -gsplit-dwarf
else
DEBUG_CFLAGS	+= -g
endif
KBUILD_AFLAGS	+= -Wa,-gdwarf-2
endif
ifdef CONFIG_DEBUG_INFO_DWARF4
DEBUG_CFLAGS	+= -gdwarf-4
endif

ifdef CONFIG_DEBUG_INFO_REDUCED
DEBUG_CFLAGS	+= $(call cc-option, -femit-struct-debug-baseonly) \
		   $(call cc-option,-fno-var-tracking)
endif

KBUILD_CFLAGS += $(DEBUG_CFLAGS)
export DEBUG_CFLAGS

# We trigger additional mismatches with less inlining
ifdef CONFIG_DEBUG_SECTION_MISMATCH
KBUILD_CFLAGS += $(call cc-option, -fno-inline-functions-called-once)
endif

ifdef CONFIG_LD_DEAD_CODE_DATA_ELIMINATION
KBUILD_CFLAGS_KERNEL += -ffunction-sections -fdata-sections
LDFLAGS_laritos += --gc-sections
endif

# arch Makefile may override CC so keep this after arch Makefile is included
NOSTDINC_FLAGS += -nostdinc -isystem $(shell $(CC) -print-file-name=include)

# Variable Length Arrays (VLAs) should not be used anywhere in the kernel
KBUILD_CFLAGS += -Wvla

# disable pointer signed / unsigned warnings in gcc 4.0
KBUILD_CFLAGS += -Wno-pointer-sign

# disable stringop warnings in gcc 8+
KBUILD_CFLAGS += $(call cc-disable-warning, stringop-truncation)

# disable invalid "can't wrap" optimizations for signed / pointers
KBUILD_CFLAGS	+= $(call cc-option,-fno-strict-overflow)

# clang sets -fmerge-all-constants by default as optimization, but this
# is non-conforming behavior for C and in fact breaks the kernel, so we
# need to disable it here generally.
KBUILD_CFLAGS	+= $(call cc-option,-fno-merge-all-constants)

# for gcc -fno-merge-all-constants disables everything, but it is fine
# to have actual conforming behavior enabled.
KBUILD_CFLAGS	+= $(call cc-option,-fmerge-constants)

# Make sure -fstack-check isn't enabled (like gentoo apparently did)
KBUILD_CFLAGS  += $(call cc-option,-fno-stack-check,)

# conserve stack if available
KBUILD_CFLAGS   += $(call cc-option,-fconserve-stack)

# Prohibit date/time macros, which would make the build non-deterministic
KBUILD_CFLAGS   += $(call cc-option,-Werror=date-time)

# enforce correct pointer usage
KBUILD_CFLAGS   += $(call cc-option,-Werror=incompatible-pointer-types)

# Require designated initializers for all marked structures
KBUILD_CFLAGS   += $(call cc-option,-Werror=designated-init)

# change __FILE__ to the relative path from the srctree
KBUILD_CFLAGS	+= $(call cc-option,-fmacro-prefix-map=$(srctree)/=)

# ensure -fcf-protection is disabled when using retpoline as it is
# incompatible with -mindirect-branch=thunk-extern
ifdef CONFIG_RETPOLINE
KBUILD_CFLAGS += $(call cc-option,-fcf-protection=none)
endif

# use the deterministic mode of AR if available
KBUILD_ARFLAGS := $(call ar-option,D)

include scripts/Makefile.extrawarn
include scripts/Makefile.ubsan

# Add any arch overrides and user supplied CPPFLAGS, AFLAGS and CFLAGS as the
# last assignments
KBUILD_CPPFLAGS += $(ARCH_CPPFLAGS) $(KCPPFLAGS)
KBUILD_AFLAGS   += $(ARCH_AFLAGS)   $(KAFLAGS)
KBUILD_CFLAGS   += $(ARCH_CFLAGS)   $(KCFLAGS)

KBUILD_LDFLAGS_MODULE += --build-id
LDFLAGS_laritos += --build-id

# -nostdlib: Do not use the standard system startup files or libraries when linking
LDFLAGS_laritos += -nostdlib
GNU_LIBGCC_A := $(shell $(CC) -print-file-name=libgcc.a)
# -lgcc: From gcc doc, When you specify -nostdlib or -nodefaultlibs you should usually specify -lgcc as well.
#		This ensures that you have no unresolved references to internal GCC library subroutines
#	    (e.g. __aeabi_uidivmod for arm-none-eabi-)
# -no-whole-archive: Only include those objects referenced by the program, otherwise it will fail when searching
# 	    for symbols not (yet?) implemented in laritOS, (e.g. malloc(), abort())
LDFLAGS_laritos += --no-whole-archive -L $(dir $(GNU_LIBGCC_A)) -lgcc

ifeq ($(CONFIG_STRIP_ASM_SYMS),y)
LDFLAGS_laritos	+= $(call ld-option, -X,)
endif

# Disable msg: "libgcc.a(_udivmoddi4.o) uses 4-byte wchar_t yet the output is to use 2-byte wchar_t; use of
# wchar_t values across objects may fail"
#LDFLAGS_laritos	+= --no-wchar-size-warning


# ---------------
# laritOS targets
# ---------------


# Default kernel image to build when no specific target is given.
# KBUILD_IMAGE may be overruled on the command line or
# set in the environment
# Also any assignments in arch/$(ARCH)/Makefile take precedence over
# this default value
export KBUILD_IMAGE ?= laritos

PHONY += prepare0

core-y += core/ board/
core-$(CONFIG_TEST_ENABLED) += test/

drivers-y += drivers/

laritos-dirs := $(patsubst %/,%,$(filter %/, $(init-y) $(core-y) $(drivers-y) $(libs-y)))
laritos-alldirs := $(sort $(laritos-dirs) $(patsubst %/,%,$(filter %/, $(init-) $(core-) $(drivers-) $(libs-))))

init-y		:= $(patsubst %/, %/built-in.a, $(init-y))
core-y		:= $(patsubst %/, %/built-in.a, $(core-y))
drivers-y	:= $(patsubst %/, %/built-in.a, $(drivers-y))
libs-y1		:= $(patsubst %/, %/lib.a, $(libs-y))
libs-y2		:= $(patsubst %/, %/built-in.a, $(filter-out %.a, $(libs-y)))

KBUILD_LARITOS_OBJS := $(head-y) $(init-y) $(core-y) $(libs-y2) $(drivers-y)
KBUILD_LARITOS_LIBS := $(libs-y1)

laritos-deps := $(KBUILD_LARITOS_OBJS) $(KBUILD_LARITOS_LIBS)

quiet_cmd_link_laritos ?= LD      $@
	cmd_link_laritos ?= $(LD) -T $(KBUILD_LDS) -whole-archive $(KBUILD_LARITOS_OBJS) $(KBUILD_BOARD_INFO) -o $@ $(KBUILD_LDFLAGS) $(LDFLAGS_laritos) -Map $@.map

laritos.elf: $(laritos-deps) $(KBUILD_BOARD_INFO) $(KBUILD_LDS) FORCE
	$(call if_changed,link_laritos)

# Remove symbols, relocation info, debugging metadata, etc
quiet_cmd_objcopy_laritos ?= OBJCOPY $@
	cmd_objcopy_laritos ?= $(OBJCOPY) -O binary $< $@

laritos.bin: laritos.elf FORCE
	$(call if_changed,objcopy_laritos)

SYSTEM_IMG_FOLDER := image/system

PHONY += $(SYSTEM_IMG_FOLDER)
$(SYSTEM_IMG_FOLDER):
	@mkdir -p $@

quiet_cmd_sysimg ?= SYSIMG  $@
cmd_sysimg ?= \
	dd if=/dev/zero of=$@ bs=1M count=$(CONFIG_FS_SYSTEM_IMAGE_SIZE) status=none

system.img: $(SYSTEM_IMG_FOLDER)
	$(call if_changed,sysimg)

quiet_cmd_img_laritos ?= IMAGE   $@
	cmd_img_laritos ?= \
		dd if=/dev/zero of=$@ bs=1M count=64 status=none; \
		dd if=$< of=$@ conv=notrunc status=none; \
		dd if=/home/lzungri/dev/fs/data.img of=$@ bs=$(CONFIG_FS_SYSTEM_IMAGE_OFFSET) seek=1 conv=notrunc status=none

laritos.img: laritos.bin system.img FORCE
	$(call if_changed,img_laritos)
	$(Q)echo ''
	$(Q)$(SIZE) laritos.elf
	$(Q)echo ''

laritos: laritos.img

targets := laritos laritos.img laritos.bin laritos.elf $(KBUILD_BOARD_INFO) $(KBUILD_BI_COPIED) $(KBUILD_LDS)

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
$(sort $(laritos-deps)): $(laritos-dirs) ;

# Handle descending into subdirectories listed in $(laritos-dirs)
# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language
#
PHONY += $(laritos-dirs)
$(laritos-dirs): prepare
	$(Q)$(MAKE) $(build)=$@ need-builtin=1

filechk_kernel.release = \
	echo "$(KERNELVERSION)$$($(CONFIG_SHELL) $(srctree)/scripts/setlocalversion $(srctree))"

# Store (new) KERNELRELEASE string in include/config/kernel.release
include/config/kernel.release: FORCE
	$(call filechk,kernel.release)

# Additional helpers built in scripts/
# Carefully list dependencies so we do not try to build scripts twice
# in parallel
PHONY += scripts
scripts: scripts_basic include/config/auto.conf
	$(Q)$(MAKE) $(build)=$(@)

# Things we need to do before we recursively start building the kernel
# or the modules are listed in "prepare".
# A multi level approach is used. prepareN is processed before prepareN-1.
# archprepare is used in arch Makefiles and when processed asm symlink,
# version.h and scripts_basic is processed / created.

PHONY += prepare archprepare prepare3

# prepare3 is used to check if we are building in a separate output directory,
# and if so do:
# 1) Check that make has not been executed in the kernel src $(srctree)
prepare3: include/config/kernel.release
ifdef building_out_of_srctree
	@$(kecho) '  Using $(srctree) as source for kernel'
	$(Q)if [ -f $(srctree)/.config -o \
		 -d $(srctree)/include/config -o \
		 -d $(srctree)/arch/$(SRCARCH)/include/generated ]; then \
		echo >&2 "  $(srctree) is not clean, please run 'make ARCH=$(ARCH) mrproper'"; \
		echo >&2 "  in the '$(srctree)' directory.";\
		/bin/false; \
	fi;
endif

archprepare: scripts prepare3 outputmakefile $(version_h) include/generated/utsrelease.h

prepare0: archprepare
	$(Q)$(MAKE) $(build)=.

# All the preparing..
prepare: prepare0

# Generate some files
# ---------------------------------------------------------------------------

# KERNELRELEASE can change from a few different places, meaning version.h
# needs to be updated, so this check is forced on all builds

uts_len := 64
define filechk_utsrelease.h
	if [ `echo -n "$(KERNELRELEASE)" | wc -c ` -gt $(uts_len) ]; then \
	  echo '"$(KERNELRELEASE)" exceeds $(uts_len) characters' >&2;    \
	  exit 1;                                                         \
	fi;                                                               \
	echo \#define UTS_RELEASE \"$(KERNELRELEASE)\"
endef

define filechk_version.h
	echo \#define LINUX_VERSION_CODE $(shell                         \
	expr $(VERSION) \* 65536 + 0$(PATCHLEVEL) \* 256 + 0$(SUBLEVEL)); \
	echo '#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))'
endef

$(version_h): FORCE
	$(call filechk,version.h)
	$(Q)rm -f $(old_version_h)

include/generated/utsrelease.h: include/config/kernel.release FORCE
	$(call filechk,utsrelease.h)

###
# Cleaning is done on three levels.
# make clean     Delete most generated files
#                Leave enough to build external modules
# make mrproper  Delete the current configuration, and all generated files

CLEAN_FILES += laritos.img laritos.bin laritos.elf

# Directories & files removed with 'make mrproper'
MRPROPER_DIRS  += include/config include/generated          \
		  arch/$(SRCARCH)/include/generated .tmp_objdiff
MRPROPER_FILES += .config .config.old .version

# clean - Delete most, but leave enough to build external modules
#
clean: rm-dirs  := $(CLEAN_DIRS)
clean: rm-files := $(CLEAN_FILES)
clean-dirs      := $(addprefix _clean_, . $(laritos-alldirs))

PHONY += $(clean-dirs) clean archclean laritosclean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

clean: archclean

# mrproper - Delete all generated files, including .config
#
mrproper: rm-dirs  := $(wildcard $(MRPROPER_DIRS))
mrproper: rm-files := $(wildcard $(MRPROPER_FILES))
mrproper-dirs      := $(addprefix _mrproper_,scripts)

PHONY += $(mrproper-dirs) mrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

mrproper: clean $(mrproper-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)


# Brief documentation of the typical targets used
# ---------------------------------------------------------------------------

boards := $(wildcard $(srctree)/board/configs/*_defconfig)
boards := $(sort $(notdir $(boards)))

PHONY += help
help:
	@echo  'Generic targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '  system.img      - Build sytem image only'
	@echo  '  dir/            - Build all files in dir and below'
	@echo  '  dir/file.[ois]  - Build specified target only'
	@echo  '  printmap        - Output the link map info (memory mapping, symbols, etc) (use with make -s)'
	@echo  '  kernelrelease	  - Output the release version string (use with make -s)'
	@echo  '  kernelversion	  - Output the version stored in Makefile (use with make -s)'
	@echo  '  image_name	  - Output the image name (use with make -s)'
	@echo  ''
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep the config'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  ''

	@echo  'Configuration targets:'
	@$(if $(boards), \
		$(foreach b, $(boards), \
		printf "  %-24s - Build for %s\\n" $(b) $(subst _defconfig,,$(b));) \
		echo '')

	@echo  'Logging and static analysis:'
	@echo  '  make V=0|1 [targets] 0 => quiet build (default), 1 => verbose build'
	@echo  '  make V=2   [targets] 2 => give reason for rebuild of target'
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	@echo  '  make C=1   [targets] Check re-compiled c source with $$CHECK (sparse by default)'
	@echo  '  make C=2   [targets] Force check of all c source with $$CHECK'
	@echo  '  make W=n   [targets] Enable extra gcc checks, n=1,2,3 where'
	@echo  '		1: warnings which may be relevant and do not occur too often'
	@echo  '		2: warnings which occur quite often but may still be relevant'
	@echo  '		3: more obscure warnings, can most likely be ignored'
	@echo  '		Multiple levels can be combined with W=12 or W=123'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '


# Misc
# ---------------------------------------------------------------------------

PHONY += scripts_gdb
scripts_gdb: prepare0
	$(Q)$(MAKE) $(build)=scripts/gdb
	$(Q)ln -fsn $(abspath $(srctree)/scripts/gdb/vmlinux-gdb.py)

ifdef CONFIG_GDB_SCRIPTS
all: scripts_gdb
endif

printmap: laritos.elf
	$(LD) --print-map $< -T $(KBUILD_LDS)

clean: $(clean-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)
	@find $(if $(KBUILD_EXTMOD), $(KBUILD_EXTMOD), .) $(RCS_FIND_IGNORE) \
		\( -name '*.[aios]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '*.dwo' -o -name '*.lst' \
		-o -name '*.su' -o -name '*.mod' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.lex.c' -o -name '*.tab.[ch]' \
		-o -name '*.asn1.[ch]' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name modules.builtin -o -name '.tmp_*.o.*' \
		-o -name '*.c.[012]*.*' \
		-o -name '*.ll' \
		-o -name '*.gcno' \) -type f -print | xargs rm -f

# Scripts to check various things for consistency
# ---------------------------------------------------------------------------

PHONY += includecheck coccicheck

includecheck:
	find $(srctree)/* $(RCS_FIND_IGNORE) \
		-name '*.[hcS]' -type f -print | sort \
		| xargs $(PERL) -w $(srctree)/scripts/checkincludes.pl

coccicheck:
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/$@

PHONY += kernelrelease kernelversion image_name

kernelrelease:
	@echo "$(KERNELVERSION)$$($(CONFIG_SHELL) $(srctree)/scripts/setlocalversion $(srctree))"

kernelversion:
	@echo $(KERNELVERSION)

image_name:
	@echo $(KBUILD_IMAGE)

# Clear a bunch of variables before executing the submake

ifeq ($(quiet),silent_)
tools_silent=s
endif

tools/: FORCE
	$(Q)mkdir -p $(objtree)/tools
	$(Q)$(MAKE) LDFLAGS= MAKEFLAGS="$(tools_silent) $(filter --j% -j,$(MAKEFLAGS))" O=$(abspath $(objtree)) subdir=tools -C $(srctree)/tools/

tools/%: FORCE
	$(Q)mkdir -p $(objtree)/tools
	$(Q)$(MAKE) LDFLAGS= MAKEFLAGS="$(tools_silent) $(filter --j% -j,$(MAKEFLAGS))" O=$(abspath $(objtree)) subdir=tools -C $(srctree)/tools/ $*

# Single targets
# ---------------------------------------------------------------------------
# Single targets are compatible with:
# - build with mixed source and output
# - build with separate output dir 'make O=...'
# - external modules
#
#  target-dir => where to store outputfile
#  build-dir  => directory in kernel source tree to use

build-target = $(if $(KBUILD_EXTMOD), $(KBUILD_EXTMOD)/)$@
build-dir = $(patsubst %/,%,$(dir $(build-target)))

%.i: prepare FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(build-target)
%.ll: prepare FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(build-target)
%.lst: prepare FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(build-target)
%.o: prepare FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(build-target)
%.s: prepare FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(build-target)
%.symtypes: prepare FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(build-target)

# FIXME Should go into a make.lib or something
# ===========================================================================

quiet_cmd_rmdirs = $(if $(wildcard $(rm-dirs)),CLEAN   $(wildcard $(rm-dirs)))
      cmd_rmdirs = rm -rf $(rm-dirs)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -f $(rm-files)

# read saved command lines for existing targets
existing-targets := $(wildcard $(sort $(targets)))

-include $(foreach f,$(existing-targets),$(dir $(f)).$(notdir $(f)).cmd)

endif   # ifeq ($(config-targets),1)
endif   # ifeq ($(mixed-targets),1)
endif   # need-sub-make

PHONY += FORCE
FORCE:

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
