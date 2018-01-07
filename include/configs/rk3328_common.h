/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_RK3328_COMMON_H
#define __CONFIG_RK3328_COMMON_H

#include "rockchip-common.h"

#define CONFIG_MISC_INIT_R

#define CONFIG_SYS_MALLOC_LEN		(32 << 20)
#define CONFIG_SYS_CBSIZE		1024
#define CONFIG_SKIP_LOWLEVEL_INIT

/* #define CONFIG_MTD_DEVICE */

#define CONFIG_SPL_FRAMEWORK
#if defined(CONFIG_SPL_SPI_SUPPORT)
#define CONFIG_SPL_SPI_LOAD
#endif

#define CONFIG_SYS_NS16550_MEM32

#define CONFIG_SYS_TEXT_BASE		0x00200000
#define CONFIG_SYS_INIT_SP_ADDR		0x00300000
#define CONFIG_SYS_LOAD_ADDR		0x00800800
#define CONFIG_SPL_STACK		0x00400000
#define CONFIG_SPL_TEXT_BASE		0x00000000
#define CONFIG_SPL_MAX_SIZE             0x40000
#define CONFIG_SPL_BSS_START_ADDR       0x00400000
#define CONFIG_SPL_BSS_MAX_SIZE         0x20000
#define CONFIG_TPL_STACK		0xff098000
#define CONFIG_TPL_TEXT_BASE		0xff091000
#define CONFIG_SYS_BOOTM_LEN	(64 << 20)	/* 64M */

#define GICD_BASE			0xFF811000
#define GICC_BASE			0xFF812000

#define CONFIG_ROCKUSB_G_DNL_PID	0x320C

#define CONFIG_UDP_CHECKSUM

/* MMC/SD IP block */
#define CONFIG_BOUNCE_BUFFER

#define CONFIG_SUPPORT_VFAT
#define CONFIG_FS_EXT4

/* RAW SD card / eMMC locations. */
#define CONFIG_SYS_SPI_U_BOOT_OFFS	(CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/* FAT sd card locations. */
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#define CONFIG_SYS_SDRAM_BASE		0
#define CONFIG_NR_DRAM_BANKS		1
#define SDRAM_MAX_SIZE			0xff000000

#define CONFIG_SPI_FLASH 1
#define CONFIG_SPI
#define CONFIG_SF_DEFAULT_SPEED 20000000

#ifndef CONFIG_SPL_BUILD

/* usb mass storage */
#define CONFIG_USB_FUNCTION_MASS_STORAGE

#define ENV_MEM_LAYOUT_SETTINGS \
	"scriptaddr=0x00500000\0" \
	"pxefile_addr_r=0x00600000\0" \
	"fdt_addr_r=0x01f00000\0" \
	"kernel_addr_r=0x02000000\0" \
	"ramdisk_addr_r=0x04000000\0"

#include <config_distro_bootcmd.h>
#define CONFIG_EXTRA_ENV_SETTINGS \
	ENV_MEM_LAYOUT_SETTINGS \
	"fdtfile=" FDTFILE \
	"partitions=" PARTS_DEFAULT \
	BOOTENV

#endif

/* rockchip ohci host driver */
#define CONFIG_USB_OHCI_NEW
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1

#define CONFIG_USB_FUNCTION_MASS_STORAGE

#define CONFIG_PREBOOT

/* enable usb config for usb ether */
#define CONFIG_USB_HOST_ETHER

#define CONFIG_USB_ETHER_ASIX
#define CONFIG_USB_ETHER_ASIX88179
#define CONFIG_USB_ETHER_MCS7830
#define CONFIG_USB_ETHER_SMSC95XX
#define CONFIG_USB_ETHER_RTL8152

#endif
