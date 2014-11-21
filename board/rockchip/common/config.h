/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * Configuation settings for the rk3xxx chip platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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


#ifdef CONFIG_SECUREBOOT_CRYPTO
	#define SECUREBOOT_CRYPTO_EN
#endif

#if defined(SECUREBOOT_CRYPTO_EN)
	#if defined(CONFIG_RKCHIP_RK3128)
		#define EFUSE_BASE_ADDR		RKIO_EFUSE_PHYS
		#define CRYPTO_BASE_ADDR	RKIO_CRYPTO_PHYS
	#elif defined(CONFIG_RKCHIP_RK3288)
		#define EFUSE_BASE_ADDR		RKIO_EFUSE_1024BITS_PHYS
		#define CRYPTO_BASE_ADDR	RKIO_CRYPTO_PHYS
	#else
		#error: "PLS config chip for efuse and crypto base address!"
	#endif
#endif /* SECUREBOOT_CRYPTO_EN */


/* rk sdmmc boot config */
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

#if defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#define NANDC_BASE_ADDR         RKIO_NANDC_PHYS

	#define SDMMC_BASE_ADDR		RKIO_SDMMC_PHYS
	#define SDIO_BASE_ADDR		RKIO_SDIO_PHYS
	#define EMMC_BASE_ADDR		RKIO_EMMC_PHYS

	#define RKPLAT_IRQ_SDMMC	IRQ_SDMMC
	#define RKPLAT_IRQ_SDIO		IRQ_SDIO
	#define RKPLAT_IRQ_EMMC		IRQ_EMMC
#elif defined(CONFIG_RKCHIP_RK3288)
	#define NANDC_BASE_ADDR         RKIO_NANDC0_PHYS

	#define SDMMC_BASE_ADDR		RKIO_SDMMC_PHYS
	#define SDIO_BASE_ADDR		RKIO_SDIO0_PHYS
	#define EMMC_BASE_ADDR		RKIO_EMMC_PHYS

	#define RKPLAT_IRQ_SDMMC	IRQ_SDMMC
	#define RKPLAT_IRQ_SDIO		IRQ_SDIO0
	#define RKPLAT_IRQ_EMMC		IRQ_EMMC
#else
	#error: "PLS config chip for mmc irq and mmc base address!"
#endif

/* rk nand flash boot config */
#ifdef CONFIG_RK_FLASH_BOOT_EN
	#define RK_FLASH_BOOT_EN
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


//库头文件
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
#define PRINT_E		PRINTF
#define PRINT_W		PRINTF
#define RkPrintf	PRINTF


#include <common.h>
#include <asm/arch/rkplat.h>

//平台相关头文件
#include "platform/ftl_std.h"
#include "platform/chipDepend.h"

//系统相关头文件
#include "platform/rsa.h"
#include "platform/sha.h"

#include "rkloader/parameter.h"
#include "rkloader/rkimage.h"
#include "rkloader/idblock.h"
#include "rkloader/rkloader.h"
#include "rkloader/key.h"

#ifdef CONFIG_SECUREBOOT_CRYPTO
#include "SecureBoot/crypto.h"
#include "SecureBoot/efuse.h"
#endif
#include "SecureBoot/SecureBoot.h"
#include "SecureBoot/SecureVerify.h"

#include "storage/storage.h"

#if defined(CONFIG_RK_SDMMC_BOOT_EN) || defined(CONFIG_RK_SDCARD_BOOT_EN)
#include "emmc/sdmmc_config.h"
#include "mediaboot/sdmmcBoot.h"
#endif

#ifdef CONFIG_RK_FLASH_BOOT_EN
#include "mediaboot/nandflash_boot.h"
#endif

// by cmy
#define SYS_LOADER_REBOOT_FLAG		0x5242C300  //高24是TAG,低8位是标记
#define SYS_KERNRL_REBOOT_FLAG		0xC3524200  //高24是TAG,低8位是标记
#define SYS_LOADER_ERR_FLAG		0X1888AAFF

enum {
	BOOT_NORMAL = 0,
	BOOT_LOADER,     /* enter loader rockusb mode */
	BOOT_MASKROM,    /* enter maskrom rockusb mode*/
	BOOT_RECOVER,    /* enter recover */
	BOOT_NORECOVER,  /* do not enter recover */
	BOOT_WINCE,      /* FOR OTHER SYSTEM */
	BOOT_WIPEDATA,   /* enter recover and wipe data. */
	BOOT_WIPEALL,    /* enter recover and wipe all data. */
	BOOT_CHECKIMG,   /* check firmware img with backup part(in loader mode)*/
	BOOT_FASTBOOT,   
	BOOT_SECUREBOOT_DISABLE,  
	BOOT_CHARGING,
	BOOT_MAX         /* MAX VALID BOOT TYPE.*/
};


void P_RC4(unsigned char * buf, unsigned short len);
void P_RC4_ext(unsigned char * buf, unsigned short len);
uint32 CRC_32CheckBuffer(unsigned char * aData, uint32 aSize);

#endif /* _RK_CONFIG_H */

