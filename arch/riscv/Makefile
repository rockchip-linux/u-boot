# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2017 Andes Technology Corporation.
# Rick Chen, Andes Technology Corporation <rick@andestech.com>

ifeq ($(CONFIG_ARCH_RV64I),y)
	ARCH_BASE = rv64im
	ABI_BASE = lp64
endif
ifeq ($(CONFIG_ARCH_RV32I),y)
	ARCH_BASE = rv32im
	ABI_BASE = ilp32
endif
ifeq ($(CONFIG_RISCV_ISA_A),y)
	ARCH_A = a
endif
ifeq ($(CONFIG_RISCV_ISA_F),y)
	ARCH_F = f
endif
ifeq ($(CONFIG_RISCV_ISA_D),y)
	ARCH_D = d
	ABI_D = d
endif
ifeq ($(CONFIG_RISCV_ISA_C),y)
	ARCH_C = c
endif
ifeq ($(CONFIG_RISCV_ISA_ZBB),y)
	ARCH_ZBB = _zbb
endif
ifeq ($(CONFIG_CMODEL_MEDLOW),y)
	CMODEL = medlow
endif
ifeq ($(CONFIG_CMODEL_MEDANY),y)
	CMODEL = medany
endif


RISCV_MARCH = $(ARCH_BASE)$(ARCH_A)$(ARCH_F)$(ARCH_D)$(ARCH_C)$(ARCH_ZBB)
ABI = $(ABI_BASE)$(ABI_D)

# Newer binutils versions default to ISA spec version 20191213 which moves some
# instructions from the I extension to the Zicsr and Zifencei extensions.
toolchain-need-zicsr-zifencei := $(call cc-option-yn, -mabi=$(ABI) -march=$(RISCV_MARCH)_zicsr_zifencei)
ifeq ($(toolchain-need-zicsr-zifencei),y)
	RISCV_MARCH := $(RISCV_MARCH)_zicsr_zifencei
endif

ARCH_FLAGS = -march=$(RISCV_MARCH) -mabi=$(ABI) \
	     -mcmodel=$(CMODEL)

ifeq ($(CONFIG_$(XPL_)FRAMEPOINTER),y)
	ARCH_FLAGS += -fno-omit-frame-pointer
endif

PLATFORM_CPPFLAGS	+= $(ARCH_FLAGS)
CFLAGS_EFI		+= $(ARCH_FLAGS)

head-y := arch/riscv/cpu/start.o

# Per-SoC machine directories (e.g. arch/riscv/mach-rockchip).
# Each enabled machine contributes its own objects to the link and exposes
# its include/ directory so that the <asm/arch-rockchip/*.h> headers resolve.
machine-$(CONFIG_ARCH_ROCKCHIP)		+= rockchip
machdirs := $(patsubst %,arch/riscv/mach-%/,$(machine-y))
PLATFORM_CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))

libs-y += arch/riscv/cpu/
ifneq ($(CONFIG_ARCH_ROCKCHIP),y)
libs-y += arch/riscv/cpu/$(CPU)/
endif
libs-y += arch/riscv/lib/
# Link the per-SoC machine objects built under the machdirs directories.
libs-y += $(machdirs)
