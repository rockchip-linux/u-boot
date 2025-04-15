# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2014 Google, Inc
# Copyright (c) 2019 Rockchip Electronics Co., Ltd.

# We don't want the bootrom-helper present in a full U-Boot build, as
# this may have entered from ATF with the stack-pointer pointing to
# inaccessible/protected memory (and the bootrom-helper assumes that
# the stack-pointer is valid before switching to the U-Boot stack).
obj-spl-$(CONFIG_ROCKCHIP_BROM_HELPER) += bootrom.o
obj-spl-$(CONFIG_SPL_ROCKCHIP_COMMON_BOARD) += spl.o spl-boot-order.o spl_common.o
obj-tpl-$(CONFIG_ROCKCHIP_BROM_HELPER) += bootrom.o
obj-tpl-$(CONFIG_TPL_ROCKCHIP_COMMON_BOARD) += tpl.o spl_common.o
obj-tpl-$(CONFIG_ROCKCHIP_PX30) += px30-board-tpl.o

obj-spl-$(CONFIG_ROCKCHIP_RK3036) += rk3036-board-spl.o

ifeq ($(CONFIG_XPL_BUILD)$(CONFIG_TPL_BUILD),)

# Always include boot_mode.o, as we bypass it (i.e. turn it off)
# inside of boot_mode.c when CONFIG_ROCKCHIP_BOOT_MODE_REG is 0.  This way,
# we can have the preprocessor correctly recognise both 0x0 and 0
# meaning "turn it off".
obj-y += boot_mode.o
obj-$(CONFIG_ROCKCHIP_COMMON_BOARD) += board.o
endif

ifeq ($(CONFIG_TPL_BUILD),)
obj-$(CONFIG_DISPLAY_CPUINFO) += cpu-info.o
endif

obj-$(CONFIG_$(PHASE_)RAM) += sdram.o

obj-$(CONFIG_ROCKCHIP_PX30) += px30/
obj-$(CONFIG_ROCKCHIP_RK3036) += rk3036/
obj-$(CONFIG_ROCKCHIP_RK3066) += rk3066/
obj-$(CONFIG_ROCKCHIP_RK3128) += rk3128/
obj-$(CONFIG_ROCKCHIP_RK3188) += rk3188/
obj-$(CONFIG_ROCKCHIP_RK322X) += rk322x/
obj-$(CONFIG_ROCKCHIP_RK3288) += rk3288/
obj-$(CONFIG_ROCKCHIP_RK3308) += rk3308/
obj-$(CONFIG_ROCKCHIP_RK3328) += rk3328/
obj-$(CONFIG_ROCKCHIP_RK3368) += rk3368/
obj-$(CONFIG_ROCKCHIP_RK3399) += rk3399/
obj-$(CONFIG_ROCKCHIP_RK3568) += rk3568/
obj-$(CONFIG_ROCKCHIP_RK3576) += rk3576/
obj-$(CONFIG_ROCKCHIP_RK3588) += rk3588/
obj-$(CONFIG_ROCKCHIP_RV1108) += rv1108/
obj-$(CONFIG_ROCKCHIP_RV1126) += rv1126/

# Clear out SPL objects, in case this is a TPL build
obj-spl-$(CONFIG_TPL_BUILD) =

# Now add SPL/TPL objects back into the main build
obj-$(CONFIG_XPL_BUILD) += $(obj-spl-y)
obj-$(CONFIG_TPL_BUILD) += $(obj-tpl-y)

ifeq ($(CONFIG_SPL_BUILD),)
obj-y += bootargs.o
obj-y += bootdev.o
obj-y += download.o
obj-y += load_dtb.o
obj-y += loader_tag.o
obj-y += lowlevel_init.o
obj-y += memory.o
obj-y += param.o
obj-y += bootm.o
obj-y += misc.o
obj-y += efi.o
obj-$(CONFIG_ROCKCHIP_PRELOADER_ATAGS)	+= atags.o
obj-$(CONFIG_SET_DFU_ALT_INFO)		+= dfu_alt_info.o
obj-$(CONFIG_ROCKCHIP_FIT_IMAGE)	+= fit.o fit_misc.o
obj-$(CONFIG_ROCKCHIP_FPGA)		+= fpga.o
obj-$(CONFIG_PSTORE)			+= pstore.o
obj-$(CONFIG_ROCKCHIP_RESOURCE_IMAGE)	+= resource.o
obj-$(CONFIG_ROCKCHIP_HWID_DTB)		+= hwid_dtb.o
obj-$(CONFIG_ROCKCHIP_SMCCC)		+= smccc.o
obj-$(CONFIG_SPL_KERNEL_BOOT)		+= spl_resource.o
obj-$(CONFIG_ROCKCHIP_UIMAGE)		+= uimage.o
obj-$(CONFIG_ROCKCHIP_VENDOR_PARTITION) += vendor.o vendor_misc.o
obj-$(CONFIG_ROCKCHIP_MINIDUMP)		+= mini_dump.o 
else
obj-y += spl_boot_mode.o
endif

obj-$(CONFIG_$(SPL_)FIT)		+= fit_misc.o
