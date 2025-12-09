/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 */

#ifndef __CONFIG_RK3538_COMMON_H
#define __CONFIG_RK3538_COMMON_H

#define CFG_CPUID_OFFSET	0x22

#include "rockchip-common.h"

#define CFG_IRAM_BASE			0xfafd0000
#define CFG_SYS_SDRAM_BASE		0
#define SDRAM_MAX_SIZE			0xf0000000

#define GICD_BASE			0xfdf11000
#define GICC_BASE			0xfdf12000

/* secure otp */
#define OTP_UBOOT_ROLLBACK_OFFSET	0x310
#define OTP_UBOOT_ROLLBACK_WORDS	2	/* 64 bits, 2 words */
#define OTP_ALL_ONES_NUM_BITS		32
#define OTP_SECURE_BOOT_ENABLE_ADDR	0x20
#define OTP_SECURE_BOOT_ENABLE_SIZE	1
#define OTP_DISABLE_UPGRADE_ADDR	0x154
#define OTP_DISABLE_USB_VAL		0x3
#define OTP_DISABLE_SD_VAL		0xc
#define OTP_DISABLE_UART_VAL		0x30
#define OTP_DISABLE_SPI2APB_VAL		0xc0
#define OTP_RSA_HASH_ADDR		0x180
#define OTP_RSA_HASH_SIZE		32
#define OTP_NEXT_RSA_HASH_ADDR		0x1a0
#define OTP_NEXT_RSA_HASH_SIZE		32
#define OTP_RSA_HASH_REVOKE_VAL		0x3
#define OTP_RSA_HASH_REVOKE_ADDR	0x23
#define OTP_RSA_HASH_REVOKE_SIZE	1
#define OTP_DICE_UDS_ADDR		0x160
#define OTP_DICE_UDS_SIZE		32

/* rockusb */
#define CONFIG_ROCKUSB_G_DNL_PID        0x351b

#ifndef ROCKCHIP_DEVICE_SETTINGS
#define ROCKCHIP_DEVICE_SETTINGS
#endif

#define ENV_MEM_LAYOUT_SETTINGS		\
	"scriptaddr=0x00500000\0" \
	"script_offset_f=0xffe000\0"	\
	"script_size_f=0x2000\0"	\
	"pxefile_addr_r=0x00600000\0" \
	"fdt_addr_r=0x08300000\0"	\
	"fdtoverlay_addr_r=0x02000000\0"	\
	"kernel_addr_r=0x00400000\0"	\
	"kernel_addr_c=0x05480000\0"	\
	"ramdisk_addr_r=0x0a200000\0"	\
	"kernel_comp_addr_r=0x05480000\0"	\
	"kernel_comp_size=0x2000000\0"

#define CFG_EXTRA_ENV_SETTINGS		\
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0"	\
	"partitions=" PARTS_DEFAULT	\
	ENV_MEM_LAYOUT_SETTINGS		\
	ROCKCHIP_DEVICE_SETTINGS	\
	"boot_targets=" BOOT_TARGETS "\0"

#endif /* __CONFIG_RK3538_COMMON_H */
