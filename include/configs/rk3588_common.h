/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2021 Rockchip Electronics Co., Ltd
 * Copyright (c) 2023 Edgeble AI Technologies Pvt. Ltd.
 */

#ifndef __CONFIG_RK3588_COMMON_H
#define __CONFIG_RK3588_COMMON_H

#include "rockchip-common.h"

#define CFG_IRAM_BASE			0xff000000

#define CFG_SYS_SDRAM_BASE		0
#define SDRAM_MAX_SIZE			0xf0000000

#define GICD_BASE			0xfe600000
#define GICR_BASE			0xfe680000
#define GICC_BASE			0xfe600000

/* secure otp */
#define OTP_UBOOT_ROLLBACK_OFFSET	0x150
#define OTP_UBOOT_ROLLBACK_WORDS	2	/* 64 bits, 2 words */
#define OTP_ALL_ONES_NUM_BITS		32
#define OTP_SECURE_BOOT_ENABLE_ADDR	0x20
#define OTP_SECURE_BOOT_ENABLE_SIZE	1
#define OTP_RSA4096_ENABLE_ADDR		0x21
#define OTP_RSA4096_ENABLE_SIZE		1
#define OTP_RSA_HASH_ADDR		0x9c0
#define OTP_RSA_HASH_SIZE		32

/* rockusb */
#define CFG_USB_DNL_BASE		0xfc000000
#define CONFIG_ROCKUSB_G_DNL_PID	0x350b

#define ENV_MEM_LAYOUT_SETTINGS		\
	"scriptaddr=0x00c00000\0"	\
	"script_offset_f=0xffe000\0"	\
	"script_size_f=0x2000\0"	\
	"pxefile_addr_r=0x00e00000\0"	\
	"fdt_addr_r=0x08300000\0"	\
	"fdtoverlay_addr_r=0x02000000\0"	\
	"kernel_addr_r=0x00400000\0"	\
	"ramdisk_addr_r=0x0a200000\0"	\
	"kernel_comp_addr_r=0x05480000\0"	\
	"kernel_comp_size=0x2000000\0"

#define CFG_EXTRA_ENV_SETTINGS		\
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0"	\
	"partitions=" PARTS_DEFAULT	\
	ENV_MEM_LAYOUT_SETTINGS		\
	ROCKCHIP_DEVICE_SETTINGS	\
	"boot_targets=" BOOT_TARGETS "\0"


#define GICD_BASE                       0xfe600000
#define GICR_BASE                       0xfe680000
#define GICC_BASE                       0xfe600000
#define CONFIG_LIB_HW_RAND

#endif /* __CONFIG_RK3588_COMMON_H */
