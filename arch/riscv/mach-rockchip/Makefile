# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2026 Rockchip Electronics Co., Ltd

# Reuse the architecture-independent Rockchip implementation. Only cache.c is
# RISC-V specific; ARM startup, CPU, MMU and SoC-specific objects stay excluded.
rockchip_common := ../../arm/mach-rockchip

obj-$(CONFIG_ROCKCHIP_RK182X) += rk182x/

ifdef CONFIG_SPL_BUILD
obj-y += $(rockchip_common)/spl.o
obj-y += $(rockchip_common)/spl-boot-order.o
obj-y += $(rockchip_common)/spl_common.o
obj-y += $(rockchip_common)/spl_boot_mode.o
obj-y += $(rockchip_common)/param.o
obj-$(CONFIG_ROCKCHIP_META) += $(rockchip_common)/meta.o
obj-$(CONFIG_SPL_ROCKCHIP_HWID_DTB) += $(rockchip_common)/hwid_dtb.o
else
obj-y += $(rockchip_common)/misc.o
obj-y += $(rockchip_common)/board.o
obj-y += $(rockchip_common)/loader_tag.o
obj-y += $(rockchip_common)/boot_mode.o
obj-y += $(rockchip_common)/bootargs.o
obj-y += $(rockchip_common)/bootm.o
obj-y += $(rockchip_common)/bootdev.o
obj-y += $(rockchip_common)/efi.o
obj-y += $(rockchip_common)/download.o
obj-y += $(rockchip_common)/fastboot.o
obj-y += $(rockchip_common)/load_dtb.o
obj-y += $(rockchip_common)/memory.o
obj-y += $(rockchip_common)/param.o

obj-$(CONFIG_ROCKCHIP_FIT_IMAGE) += $(rockchip_common)/fit.o
obj-$(CONFIG_ROCKCHIP_HWID_DTB) += $(rockchip_common)/hwid_dtb.o
obj-$(CONFIG_ROCKCHIP_UIMAGE) += $(rockchip_common)/uimage.o
obj-$(CONFIG_ROCKCHIP_MINIDUMP) += $(rockchip_common)/mini_dump.o
obj-$(CONFIG_ROCKCHIP_RESOURCE_IMAGE) += $(rockchip_common)/resource.o
obj-$(CONFIG_SPL_KERNEL_BOOT) += $(rockchip_common)/spl_resource.o
endif

obj-y += $(rockchip_common)/cpu.o
obj-y += $(rockchip_common)/sdram.o
obj-$(CONFIG_ROCKCHIP_PRELOADER_ATAGS) += $(rockchip_common)/atags.o
obj-$(CONFIG_ROCKCHIP_VENDOR_PARTITION) += $(rockchip_common)/vendor.o
obj-$(CONFIG_ROCKCHIP_VENDOR_PARTITION) += $(rockchip_common)/vendor_misc.o
obj-$(CONFIG_PSTORE) += $(rockchip_common)/pstore.o
obj-$(CONFIG_$(SPL_)FIT) += $(rockchip_common)/fit_misc.o
