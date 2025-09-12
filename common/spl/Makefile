# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2012
# Texas Instruments Incorporated - https://www.ti.com/
# Aneesh V <aneesh@ti.com>
# Based on common/Makefile.
#

ifeq ($(CONFIG_TPL_BUILD), y)
ifndef CONFIG_TPL_TINY_FRAMEWORK
obj-$(CONFIG_SPL_FRAMEWORK) += spl.o
endif
else
ifdef CONFIG_SPL_BUILD
ifdef CONFIG_SPL_DECOMP_HEADER
obj-y += spl_decomp.o
else
obj-$(CONFIG_SPL_FRAMEWORK) += spl.o
endif
endif
endif

ifdef CONFIG_XPL_BUILD
obj-$(CONFIG_$(PHASE_)BOOTROM_SUPPORT) += spl_bootrom.o
ifeq ($(CONFIG_$(PHASE_)LOAD_FIT),y)
ifndef CONFIG_SPL_KERNEL_BOOT_PREBUILT
obj-y += spl_fit.o
endif
endif
obj-$(CONFIG_$(PHASE_)BLK_FS) += spl_blk_fs.o
obj-$(CONFIG_$(PHASE_)LEGACY_IMAGE_FORMAT) += spl_legacy.o
obj-$(CONFIG_$(PHASE_)RELOC_LOADER) += spl_reloc.o
obj-$(CONFIG_$(PHASE_)NOR_SUPPORT) += spl_nor.o
obj-$(CONFIG_$(PHASE_)XIP_SUPPORT) += spl_xip.o
obj-$(CONFIG_$(PHASE_)YMODEM_SUPPORT) += spl_ymodem.o
ifndef CONFIG_SPL_UBI
obj-$(CONFIG_$(PHASE_)NAND_SUPPORT) += spl_nand.o
obj-$(CONFIG_$(PHASE_)ONENAND_SUPPORT) += spl_onenand.o
endif
obj-$(CONFIG_$(PHASE_)UBI) += spl_ubi.o
obj-$(CONFIG_$(PHASE_)NET) += spl_net.o
obj-$(CONFIG_$(PHASE_)MMC) += spl_mmc.o
obj-$(CONFIG_$(PHASE_)MTD_SUPPORT) += spl_mtd_blk.o
obj-$(CONFIG_$(PHASE_)RKNAND_SUPPORT) += spl_rknand.o
obj-$(CONFIG_$(PHASE_)ATF) += spl_atf.o
obj-$(CONFIG_$(PHASE_)OPTEE_IMAGE) += spl_optee.o
obj-$(CONFIG_$(PHASE_)OPENSBI) += spl_opensbi.o
obj-$(CONFIG_$(PHASE_)LOAD_RKFW) += spl_rkfw.o
obj-$(CONFIG_$(PHASE_)AB) += spl_ab.o
obj-$(CONFIG_$(PHASE_)USB_STORAGE) += spl_usb.o
obj-$(CONFIG_$(PHASE_)FS_FAT) += spl_fat.o
obj-$(CONFIG_$(PHASE_)FS_EXT4) += spl_ext.o
obj-$(CONFIG_$(PHASE_)LOAD_IMX_CONTAINER) += spl_imx_container.o
obj-$(CONFIG_$(PHASE_)SATA) += spl_sata.o
obj-$(CONFIG_$(PHASE_)NVME) += spl_nvme.o
obj-$(CONFIG_$(PHASE_)SEMIHOSTING) += spl_semihosting.o
obj-$(CONFIG_$(PHASE_)UFS) += spl_ufs.o
obj-$(CONFIG_$(PHASE_)DFU) += spl_dfu.o
obj-$(CONFIG_$(PHASE_)SPI_LOAD) += spl_spi.o
obj-$(CONFIG_$(PHASE_)RAM_SUPPORT) += spl_ram.o
obj-$(CONFIG_$(PHASE_)USB_SDP_SUPPORT) += spl_sdp.o
obj-$(CONFIG_$(PHASE_)BOOT_IMAGE) += spl_boot_image.o
endif

obj-$(CONFIG_$(PHASE_)UPL) += spl_upl.o
