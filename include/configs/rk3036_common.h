/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#ifndef __CONFIG_RK3036_COMMON_H
#define __CONFIG_RK3036_COMMON_H

#include <asm/arch/hardware.h>

#define CONFIG_SYS_NO_FLASH
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_SYS_MAXARGS		16
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_MALLOC_LEN		(32 << 20)
#define CONFIG_SYS_CBSIZE		1024
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SYS_THUMB_BUILD
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_TIMER_RATE		(24 * 1000 * 1000)
#define CONFIG_SYS_TIMER_BASE		0x200440a0 /* TIMER5 */
#define CONFIG_SYS_TIMER_COUNTER	(CONFIG_SYS_TIMER_BASE + 8)

#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_MEM32

#define CONFIG_SYS_TEXT_BASE		0x60000000
#define CONFIG_SYS_INIT_SP_ADDR		0x60100000
#define CONFIG_SYS_LOAD_ADDR		0x60800800
#define CONFIG_SPL_STACK		0x10081fff
#define CONFIG_SPL_TEXT_BASE		0x10081004

#define CONFIG_ROCKCHIP_MAX_INIT_SIZE	(4 << 10)
#define CONFIG_ROCKCHIP_CHIP_TAG	"RK30"

#define CONFIG_ROCKCHIP_COMMON

/* MMC/SD IP block */
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC
#define CONFIG_SDHCI
#define CONFIG_DWMMC
#define CONFIG_BOUNCE_BUFFER

#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_PARTITION_UUIDS
#define CONFIG_CMD_PART

#define CONFIG_CMD_CACHE
#define CONFIG_CMD_TIME

#define CONFIG_SYS_SDRAM_BASE		0x60000000
#define CONFIG_NR_DRAM_BANKS		1
#define SDRAM_BANK_SIZE			(512UL << 20UL)

#define CONFIG_SPI_FLASH
#define CONFIG_SPI
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI
#define CONFIG_SPI_FLASH_GIGADEVICE
#define CONFIG_SF_DEFAULT_SPEED 20000000

#define CONFIG_CMD_I2C

/* FASTBOOT */
#define CONFIG_CMD_FASTBOOT
#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_DOWNLOAD
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_S3C_UDC_OTG
#define CONFIG_USB_GADGET_RK_UDC_OTG_PHY
#define CONFIG_USB_FUNCTION_FASTBOOT
#define CONFIG_USB_FUNCTION_MASS_STORAGE
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV		0
#define CONFIG_FASTBOOT_BUF_ADDR	(CONFIG_SYS_SDRAM_BASE \
					+ SDRAM_BANK_SIZE / 2)
#define CONFIG_FASTBOOT_BUF_SIZE		0x07000000
#define CONFIG_USB_GADGET_VBUS_DRAW		0
#define CONFIG_SYS_CACHELINE_SIZE		64
#define CONFIG_G_DNL_MANUFACTURER		"Rockchip"
#define CONFIG_G_DNL_VENDOR_NUM		0x2207
#define CONFIG_G_DNL_PRODUCT_NUM		0x0006

#ifndef CONFIG_SPL_BUILD
#define CONFIG_CMD_GPT
#define CONFIG_RANDOM_UUID

#define CONFIG_OF_LIBFDT
#define CONFIG_CMDLINE_EDITING
#define CONFIG_AUTO_COMPLETE
#define CONFIG_BOOTDELAY     2
#define CONFIG_SYS_LONGHELP
#define CONFIG_MENU
#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_SYS_HUSH_PARSER

#define CONFIG_ENV_SIZE                 (32 << 10)
#undef CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV          0
#define CONFIG_SYS_MMC_ENV_PART         2
#define CONFIG_ENV_OFFSET               8064
#define CONFIG_ENV_OFFSET_REDUND        (CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE / 512)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT

/* We use reserved partition to store env now, so it should match env configs */
#define PARTS_DEFAULT \
	"uuid_disk=${uuid_gpt_disk};" \
        "name=loader,start=32K,size=4000K,uuid=${uuid_gpt_loader};" \
        "name=reserved,size=64K,uuid=${uuid_gpt_reserved};" \
        "name=misc,size=4M,uuid=${uuid_gpt_misc};" \
        "name=recovery,size=32M,uuid=${uuid_gpt_recovery};" \
        "name=boot_a,size=32M,uuid=${uuid_gpt_boot_a};" \
        "name=boot_b,size=32M,uuid=${uuid_gpt_boot_b};" \
        "name=system_a,size=818M,uuid=${uuid_gpt_system_a};" \
        "name=system_b,size=818M,uuid=${uuid_gpt_system_b};" \
        "name=vendor_a,size=50M,uuid=${uuid_gpt_vendor_a};" \
        "name=vendor_b,size=50M,uuid=${uuid_gpt_vendor_b};" \
        "name=cache,size=100M,uuid=${uuid_gpt_cache};" \
        "name=metadata,size=16M,uuid=${uuid_gpt_metadata};" \
        "name=persist,size=4M,uuid=${uuid_gpt_persist};" \
        "name=userdata,size=-,uuid=${uuid_gpt_userdata};\0" \

/* Linux fails to load the fdt if it's loaded above 512M on a evb-rk3036 board,
 * so limit the fdt reallocation to that */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_high=0x1fffffff\0" \
	"partitions=" PARTS_DEFAULT \

#define CONFIG_BOOTCOMMAND \
	"if mmc rescan; then " \
		"echo SD/MMC found...;" \
		"gpt write mmc 0 ${partitions}; mmc rescan;" \
		"mmc read 65000000 14000 4000; bootm 65000000;" \
	"fi;" \

#endif

#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_INITRD_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_SYS_BOOTPARAMS_LEN   (64*1024)
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_PREBOOT
#define CONFIG_SYS_BOOT_RAMDISK_HIGH
#endif
