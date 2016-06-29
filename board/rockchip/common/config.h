/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _RK_CONFIG_H
#define _RK_CONFIG_H

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define	SECURE_BOOT_ENABLE
#define	SECURE_BOOT_ENABLE_ALWAY
#define	SECURE_BOOT_LOCK


/* secure config */
#ifdef CONFIG_SECUREBOOT_CRYPTO
	#define SECUREBOOT_CRYPTO_EN
#endif

#if defined(SECUREBOOT_CRYPTO_EN)
	#define SECURE_EFUSE_BASE_ADDR	RKIO_SECUREEFUSE_BASE
	#define CRYPTO_BASE_ADDR	RKIO_CRYPTO_BASE
#endif /* SECUREBOOT_CRYPTO_EN */


/* rk sdmmc boot config */
#ifdef CONFIG_RK_SDHCI_BOOT_EN
	#define RK_SDHCI_BOOT_EN
#endif
#ifdef CONFIG_RK_SDMMC_BOOT_EN
	#define RK_SDMMC_BOOT_EN
#endif
#ifdef CONFIG_RK_SDCARD_BOOT_EN
	#define RK_SDCARD_BOOT_EN
#endif

#if defined(RK_SDMMC_BOOT_EN) || defined(RK_SDCARD_BOOT_EN)
	#define DRIVERS_SDMMC
	#define EMMC_NOT_USED_BOOT_PART
#endif

/* loader save information for kernel in ram */
#define BOOTINFO_RAM_BASE	RKIO_BOOTINFO_BASE

#define NANDC_BASE_ADDR         RKIO_NANDC_BASE
#define SDMMC_BASE_ADDR		RKIO_SDMMC_BASE
#define SDIO_BASE_ADDR		RKIO_SDIO_BASE
#define EMMC_BASE_ADDR		RKIO_EMMC_BASE

#define RKPLAT_IRQ_SDMMC	RKIRQ_SDMMC
#define RKPLAT_IRQ_SDIO		RKIRQ_SDIO
#define RKPLAT_IRQ_EMMC		RKIRQ_EMMC


/* rk nand flash boot config */
#ifdef CONFIG_RK_FLASH_BOOT_EN
	#define RK_FLASH_BOOT_EN
#endif


/* rk ums boot config */
#ifdef CONFIG_RK_UMS_BOOT_EN
	#define RK_UMS_BOOT_EN
#endif


#define __packed	__attribute__((packed))
#define __align(x)	__attribute__ ((aligned(x)))

#ifndef __GNUC__
#define PACKED1		__packed
#define PACKED2
#else
#define PACKED1
#define PACKED2		__attribute__((packed))
#endif


/* 库头文件 */
#define Assert(cond, msg, num)
#define min(X, Y)				\
	({ typeof(X) __x = (X);			\
		typeof(Y) __y = (Y);		\
		(__x < __y) ? __x : __y; })

#define max(X, Y)				\
	({ typeof(X) __x = (X);			\
		typeof(Y) __y = (Y);		\
		(__x > __y) ? __x : __y; })

#define MIN(x, y)  min(x, y)
#define MAX(x, y)  max(x, y)

#ifdef DEBUG_FLASH
#define	PRINTF		printf
#else
#define	PRINTF(...)
#endif

#define PRINT_I		PRINTF
#define PRINT_D		PRINTF
#define PRINT_E		printf /* for print error information */
#define PRINT_W		PRINTF
#define RkPrintf	PRINTF


#include <common.h>
#include <asm/arch/rkplat.h>

/* 平台相关头文件 */
#include "platform/ftl_std.h"
#include "platform/chipDepend.h"

/* 系统相关头文件 */
#include "platform/rsa.h"
#include "platform/sha.h"

#include "rkloader/parameter.h"
#include "rkloader/rkimage.h"
#include "rkloader/idblock.h"
#include "rkloader/rkloader.h"
#include "rkloader/key.h"

#ifdef CONFIG_RK_EFUSE
#include "SecureBoot/efuse.h"
#endif
#ifdef CONFIG_SECUREBOOT_CRYPTO
#include "SecureBoot/crypto.h"
#endif
#include "SecureBoot/SecureBoot.h"
#include "SecureBoot/SecureVerify.h"

#include "storage/storage.h"

#if defined(CONFIG_RK_SDMMC_BOOT_EN) || defined(CONFIG_RK_SDCARD_BOOT_EN)
#include "emmc/sdmmc_config.h"
#include "mediaboot/sdmmcBoot.h"
#endif

#ifdef CONFIG_RK_SDHCI_BOOT_EN
#include "mediaboot/sdhciBoot.h"
#endif

#ifdef CONFIG_RK_FLASH_BOOT_EN
#include "mediaboot/nandflash_boot.h"
#endif

#ifdef CONFIG_RK_UMS_BOOT_EN
#include "mediaboot/UMSBoot.h"
#endif

#define SYS_LOADER_REBOOT_FLAG		0x5242C300  /* 高24是TAG,低8位是标记 */
#define SYS_KERNRL_REBOOT_FLAG		0xC3524200  /* 高24是TAG,低8位是标记 */
#define SYS_LOADER_ERR_FLAG		0X1888AAFF

enum {
	BOOT_NORMAL = 0,
	BOOT_LOADER,     /* enter loader rockusb mode */
	BOOT_MASKROM,    /* enter maskrom rockusb mode */
	BOOT_RECOVER,    /* enter recover */
	BOOT_NORECOVER,  /* do not enter recover */
	BOOT_WINCE,      /* FOR OTHER SYSTEM */
	BOOT_WIPEDATA,   /* enter recover and wipe data. */
	BOOT_WIPEALL,    /* enter recover and wipe all data. */
	BOOT_CHECKIMG,   /* check firmware img with backup part(in loader mode) */
	BOOT_FASTBOOT,
	BOOT_SECUREBOOT_DISABLE,
	BOOT_CHARGING,
	BOOT_MAX         /* MAX VALID BOOT TYPE. */
};


void P_RC4(unsigned char *buf, unsigned short len);
void P_RC4_ext(unsigned char *buf, unsigned short len);
uint32 CRC_32CheckBuffer(unsigned char *aData, uint32 aSize);

#endif /* _RK_CONFIG_H */
