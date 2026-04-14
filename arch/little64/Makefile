#
# Makefile for the Little64 architecture
#

BITS := 64
UTS_MACHINE := little64
SRCARCH := little64

# Target triple for clang
CLANG_TARGET_FLAGS_little64 := little64-unknown-elf

# Flags for clang/llvm
# Note: -march not supported for little64 target in LLVM yet
KBUILD_AFLAGS +=

# Linker relaxation is not always supported for little64, keep it simple
KBUILD_LDFLAGS += -z max-page-size=4096

# Default target for 'make all'
KBUILD_DEFCONFIG := little64_defconfig

# arch-specific subdirectories
core-y += arch/little64/kernel/
core-y += arch/little64/mm/
libs-y += arch/little64/lib/

