/*
 * Configuation settings for the rk312x chip platform.
 *
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK32PLAT_CONFIG_H
#define __RK32PLAT_CONFIG_H


/*
 * uboot ram config.
 */
#include <linux/sizes.h>
#define CONFIG_RAM_PHY_START		0x00000000
#define CONFIG_RAM_PHY_SIZE		SZ_128M
#define CONFIG_RAM_PHY_END		(CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE)

/* reserve iomap memory. */
#define CONFIG_MAX_MEM_ADDR		0xFF000000


/*
 * 		define uboot loader addr.
 * notice: CONFIG_SYS_TEXT_BASE must be an immediate,
 * so if CONFIG_RAM_PHY_START is changed, also update CONFIG_SYS_TEXT_BASE define.
 *
 * if uboot as first level, CONFIG_SYS_TEXT_BASE = CONFIG_RAM_PHY_START
 * if uboot ad second level, CONFIG_SYS_TEXT_BASE = CONFIG_RAM_PHY_START + SZ_2M
 *    Resersed 2M space for packed nand bin.
 *
 */
#ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
	#define CONFIG_SYS_TEXT_BASE    0x00200000 /* Resersed 2M space for packed nand bin. */
#else
	#define CONFIG_SYS_TEXT_BASE    0x00000000
#endif


/*
 * rk plat default configs.
 */
#include <configs/rk_default_config.h>

/* undef some module for rk chip */
#undef CONFIG_SYS_ICACHE_OFF
#undef CONFIG_SYS_DCACHE_OFF
#undef CONFIG_IMPRECISE_ABORTS_CHECK
#undef CONFIG_MERGER_MINILOADER
#undef CONFIG_RK_IO_TOOL
#undef CONFIG_RK_SPI

#if defined(CONFIG_RKCHIP_RK3288)
	#undef CONFIG_RK_PL330
	#undef CONFIG_RK_DMAC
	#undef CONFIG_RK_DEVICEINFO
	#undef CONFIG_RK_PWM_REMOTE
#endif

/* mod it to enable console commands.	*/
#define CONFIG_BOOTDELAY		0

/* mmc using dma */
#define CONFIG_RK_MMC_DMA
#define CONFIG_RK_MMC_IDMAC	/* internal dmac */


/* usb host */
#if 0
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_USB_DWC_HCD
#undef CONFIG_USB_EHCI
#undef CONFIG_USB_EHCI_RK
#endif


/* more config for display */
#ifdef CONFIG_LCD

#define CONFIG_RK_3288_FB

#ifdef CONFIG_RK_HDMI
#define CONFIG_RK_3288_HDMI
#endif

#define CONFIG_RK_3288_DSI

#undef CONFIG_UBOOT_CHARGE

#endif /* CONFIG_LCD */


/* more config for charge */
#ifdef CONFIG_UBOOT_CHARGE

#define CONFIG_CMD_CHARGE_ANIM
#define CONFIG_CHARGE_DEEP_SLEEP

#endif /* CONFIG_UBOOT_CHARGE */


/* more config for power */
#if defined(CONFIG_POWER) && defined(CONFIG_POWER_RK)

#define CONFIG_POWER_I2C
#define CONFIG_SCREEN_ON_VOL_THRESD	0
#define CONFIG_SYSTEM_ON_VOL_THRESD	0

/********************************** charger and pmic driver ********************************/
#undef CONFIG_POWER_RK_SAMPLE
#define CONFIG_POWER_RICOH619
#define CONFIG_POWER_RK808
#define CONFIG_POWER_RK818
#define CONFIG_POWER_ACT8846

#define CONFIG_POWER_FG_CW201X

/********************************** battery driver ********************************/
#undef CONFIG_BATTERY_RK_SAMPLE
#undef CONFIG_BATTERY_BQ27541
#undef CONFIG_BATTERY_RICOH619

#endif /* CONFIG_POWER */

#endif /* __RK32PLAT_CONFIG_H */
