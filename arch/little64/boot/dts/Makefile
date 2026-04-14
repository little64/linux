# SPDX-License-Identifier: GPL-2.0

dtstree := $(srctree)/arch/little64/boot/dts

dtb-$(CONFIG_BUILTIN_DTB) += little64.dtb

always-y += $(dtb-y)
targets += $(dtb-y)

DTC_FLAGS += -R 4 -p 0x1000

# Kbuild converts little64.dtb → little64.dtb.o automatically via scripts/Makefile.lib
obj-$(CONFIG_BUILTIN_DTB) += $(addsuffix .o, $(dtb-y))
