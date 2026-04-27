/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 *
 */

#ifndef __CONFIG_RV1126B_COMMON_H
#define __CONFIG_RV1126B_COMMON_H

#define CFG_CPUID_OFFSET                0x22

#include "rockchip-common.h"

#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_TEXT_BASE		0x4fe00000
#define CONFIG_SPL_MAX_SIZE		0x00080000
#define CONFIG_SPL_BSS_START_ADDR	0x4fee0000
#define CONFIG_SPL_BSS_MAX_SIZE		0x20000
#define CONFIG_SPL_STACK		0x4fe00000
#ifdef CONFIG_SPL_LOAD_FIT_ADDRESS
#undef CONFIG_SPL_LOAD_FIT_ADDRESS
#endif
#define CONFIG_SPL_LOAD_FIT_ADDRESS	0x42000000

#define CONFIG_SYS_MALLOC_LEN		(32 << 20)
#define CONFIG_SYS_CBSIZE		1024

#ifdef CONFIG_SUPPORT_USBPLUG
#define CONFIG_SYS_TEXT_BASE		0x40000000
#else
#define CONFIG_SYS_TEXT_BASE		0x40200000
#endif

#define CONFIG_SYS_INIT_SP_ADDR		0x40600000
#define CONFIG_SYS_LOAD_ADDR		0x40700800
#define CONFIG_SYS_BOOTM_LEN		(64 << 20)	/* 64M */
#undef COUNTER_FREQUENCY

#define GICD_BASE			0x21201000
#define GICC_BASE			0x21202000

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

/* firmware key */
#define OTP_FW_ENC_KEY_ADDR		(0x24 * 4)
#define OTP_FW_ENC_KEY_SIZE		(0x04 * 4)

#define OTP_BACK_FW_ENC_KEY_ADDR	(0x28 * 4)
#define OTP_BACK_FW_ENC_KEY_SIZE	(0x04 * 4)

#define OTP_OEM_KEY0_ADDR		(0x30 * 4)
#define OTP_OEM_KEY0_SIZE		(0x08 * 4)

#define OTP_OEM_KEY1_ADDR		(0x38 * 4)
#define OTP_OEM_KEY1_SIZE		(0x08 * 4)

#define OTP_OEM_KEY2_ADDR		(0x40 * 4)
#define OTP_OEM_KEY2_SIZE		(0x08 * 4)

#define OTP_OEM_KEY3_ADDR		(0x48 * 4)
#define OTP_OEM_KEY3_SIZE		(0x08 * 4)

#define CONFIG_BOUNCE_BUFFER
/* For most, U-Boot no need to use 0-1G space. */
#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define SDRAM_MAX_SIZE			0xc0000000ULL	/* max 3G */
#define CONFIG_SYS_NONCACHED_MEMORY	(1 << 20)	/* 1M */

/* spl thunderboot */
#define SPL_RESV_MEM_SIZE		(2 << 20)	/* 2M */
#define KERNEL_ADDR1_R			0x00200000
#define KERNEL_ADDR1_AARCH32_R		0x00208000

/* env used only in U-Boot */
#ifndef CONFIG_SPL_BUILD
/* usb mass storage */
#define CONFIG_USB_FUNCTION_MASS_STORAGE
#define CONFIG_ROCKUSB_G_DNL_PID	0x110f

#ifdef CONFIG_ARM64
/*
 * Memory layout:
 *
 *     kernel:          2-32M
 *     compress kernel: 32-48M
 *     ramdisk:         48-131M (40MB+ ramdisk-as-rootfs on 256MB AOV board)
 *     fdt:             131M-132M
 *     optee:           132M-162M
 *     ramdisk:         162M~   (256MB+ board)
 */
/* memory size <= 3GB (0~1G is not available) */
#define ENV_MEM_LAYOUT_SETTINGS \
	"scriptaddr=0x40600000\0"	\
	"pxefile_addr_r=0x40700000\0"	\
	"fdt_addr_r=0x48300000\0"	\
	"kernel_addr_r=0x40200000\0"	\
	"kernel_addr_aarch32_r=0x40208000\0"	\
	"kernel_addr_c=0x42080000\0"	\
	"ramdisk_addr_low_r=0x43000000\0" \
	"ramdisk_addr_r=0x4a200000\0" \

/*
 * 1. memory size > 3GB (max 4G size and 0~1G is available)
 * 2. uboot and atf run at 1G+, while kernel and ramdisk run at 0-1G
 */
#define ENV_MEM_LAYOUT_SETTINGS1 \
	"scriptaddr1=0x00600000\0"	\
	"pxefile_addr1_r=0x00700000\0"	\
	"fdt_addr1_r=0x08300000\0"	\
	"kernel_addr1_r=__stringify(KERNEL_ADDR1_R)\0"	\
	"kernel_addr1_aarch32_r=__stringify(KERNEL_ADDR1_AARCH32_R)\0"	\
	"kernel_addr1_c=0x02080000\0"	\
	"ramdisk_addr1_r=0x03000000\0"
#endif

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	ENV_MEM_LAYOUT_SETTINGS \
	ENV_MEM_LAYOUT_SETTINGS1 \
	"partitions=" PARTS_RKIMG \
	ROCKCHIP_DEVICE_SETTINGS \
	RKIMG_DET_BOOTDEV \
	BOOTENV

#undef RKIMG_BOOTCOMMAND
#ifdef CONFIG_FIT_SIGNATURE
#define RKIMG_BOOTCOMMAND		\
	"boot_fit;"
#else
#define RKIMG_BOOTCOMMAND		\
	"boot_fit;"			\
	"boot_android ${devtype} ${devnum};"
#endif
#endif /* !CONFIG_SPL_BUILD */

#if defined(CONFIG_USB_HOST) || defined(CONFIG_SPL_USB_HOST_SUPPORT)
/* rockchip ohci host driver */
#define CONFIG_USB_OHCI_NEW
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1
#endif

#define CONFIG_PREBOOT
#define CONFIG_LIB_HW_RAND

#endif /* __CONFIG_RV1126B_COMMON_H */
