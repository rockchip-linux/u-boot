/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_RK3128_COMMON_H
#define __CONFIG_RK3128_COMMON_H

#include "rockchip-common.h"

#define CONFIG_SYS_MAXARGS		16
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_MALLOC_LEN		(192 << 20)
#define CONFIG_SYS_CBSIZE		1024
#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_SYS_TEXT_BASE		0x60000000
#define CONFIG_SYS_INIT_SP_ADDR		0x60300000
#define CONFIG_SYS_LOAD_ADDR		0x60800800

#define GICC_BASE			0x1013A000
#define GICD_BASE			0x10139000

#define CONFIG_SUPPORT_EMMC_RPMB

#define CONFIG_SYS_BOOTM_LEN	(64 << 20)	/* 64M */

#define CONFIG_ROCKUSB_G_DNL_PID	0x310C
#define TEST_NUM_G

/* MMC/SD IP block */
#define CONFIG_BOUNCE_BUFFER

#define CONFIG_SUPPORT_VFAT
#define CONFIG_FS_EXT4

/* RAW SD card / eMMC locations. */
#define CONFIG_SYS_SPI_U_BOOT_OFFS	(128 << 10)

#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#define CONFIG_SYS_SDRAM_BASE		0x60000000
#define SDRAM_MAX_SIZE			0x80000000

#define CONFIG_SPI_FLASH
#define CONFIG_SPI
#define CONFIG_SF_DEFAULT_SPEED 20000000

#ifndef CONFIG_SPL_BUILD
#define CONFIG_PREBOOT

/* usb mass storage */
#define CONFIG_USB_FUNCTION_MASS_STORAGE

#define ENV_MEM_LAYOUT_SETTINGS \
	"scriptaddr=0x60500000\0" \
	"pxefile_addr_r=0x60600000\0" \
	"fdt_addr_r=0x61f00000\0" \
	"kernel_addr_r=0x62000000\0" \
	"ramdisk_addr_r=0x64000000\0"

#include <config_distro_bootcmd.h>
#define CONFIG_EXTRA_ENV_SETTINGS \
	ENV_MEM_LAYOUT_SETTINGS \
	"partitions=" PARTS_DEFAULT \
	ROCKCHIP_DEVICE_SETTINGS \
	RKIMG_DET_BOOTDEV \
	BOOTENV

#endif

/* rockchip ohci host driver */
#define CONFIG_USB_OHCI_NEW
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1

#endif
