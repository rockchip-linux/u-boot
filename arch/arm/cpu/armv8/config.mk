#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
#
# SPDX-License-Identifier:	GPL-2.0+
#
PLATFORM_RELFLAGS += -fno-common -ffixed-x18

PF_CPPFLAGS_ARMV8 := $(call cc-option, -march=armv8-a)
ifdef CONFIG_ROCKCHIP
PF_CPPFLAGS_ARMV8 += $(call cc-option, -fno-pic)
PF_CPPFLAGS_ARMV8 += $(call cc-option, -mgeneral-regs-only)
endif # CONFIG_ROCKCHIP

PF_NO_UNALIGNED := $(call cc-option, -mstrict-align)
PLATFORM_CPPFLAGS += $(PF_CPPFLAGS_ARMV8)
PLATFORM_CPPFLAGS += $(PF_NO_UNALIGNED)
