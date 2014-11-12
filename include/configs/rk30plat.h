/*
 * Configuation settings for the rk312x chip platform.
 *
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK30PLAT_CONFIG_H
#define __RK30PLAT_CONFIG_H


/*
 * uboot ram config.
 */
#include <linux/sizes.h>
#define CONFIG_RAM_PHY_START		0x60000000
#define CONFIG_RAM_PHY_SIZE		SZ_128M
#define CONFIG_RAM_PHY_END		(CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE)

/* reserve iomap memory. */
#define CONFIG_MAX_MEM_ADDR		0xFFFFFFFF


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
	#define CONFIG_SYS_TEXT_BASE    0x60200000 //Resersed 2M space for packed nand bin.
#else
	#define CONFIG_SYS_TEXT_BASE    0x60000000
#endif


/*
 * rk plat default configs.
 */
#include <configs/rk_default_config.h>

/* undef some module for rk chip */
#if defined(CONFIG_RKCHIP_RK3036)
	#undef CONFIG_RK_SPI
	#undef CONFIG_PM_SUBSYSTEM
	#undef CONFIG_RK_PWM
	#undef CONFIG_POWER
	#undef CONFIG_POWER_RK
#endif /* CONFIG_RKCHIP_RK3036 */

#if defined(CONFIG_RKCHIP_RK3126)
	#undef CONFIG_RK_SPI
	#undef CONFIG_RK_PWM_REMOTE
	#undef CONFIG_RK_DEVICEINFO
	#undef CONFIG_RK_HDMI
#endif /* CONFIG_RKCHIP_RK3126 */

#if defined(CONFIG_RKCHIP_RK3128)
	#define CONFIG_SECUREBOOT_CRYPTO
	#undef CONFIG_RK_SPI
#endif /* CONFIG_RKCHIP_RK3128 */


/* mod it to enable console commands.	*/
#define CONFIG_BOOTDELAY		0

/* mmc using dma */
#define CONFIG_RK_MMC_DMA
#define CONFIG_RK_MMC_EDMAC		/* external mac */


/* more config for rockusb */
#ifdef CONFIG_CMD_ROCKUSB

/* support rockusb timeout check */
#define CONFIG_ROCKUSB_TIMEOUT_CHECK	1

/* rockusb VID/PID should the same as maskrom */
#define CONFIG_USBD_VENDORID			0x2207
#if defined(CONFIG_RKCHIP_RK3036)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x301A
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x310C
#else
	#error "PLS config rk chip for rockusb PID!"
#endif

#endif /* CONFIG_CMD_ROCKUSB */


/* more config for fastboot */
#ifdef CONFIG_CMD_FASTBOOT

#define CONFIG_USBD_PRODUCTID_FASTBOOT	0x0006
#define CONFIG_USBD_MANUFACTURER	"Rockchip"
#define CONFIG_USBD_PRODUCT_NAME	"rk30xx"

#define FASTBOOT_PRODUCT_NAME		"fastboot" /* Fastboot product name */

#define CONFIG_FASTBOOT_LOG
#define CONFIG_FASTBOOT_LOG_SIZE	(SZ_2M)

#endif /* CONFIG_CMD_FASTBOOT */


/*
 * USB Host support, default no using
 * please first check plat if you want to using usb host
 */
#if 0
/* dwc otg */
#define CONFIG_USB_DWC_HCD
/* echi */
#undef CONFIG_USB_EHCI
#undef CONFIG_USB_EHCI_RK

#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#endif


/* more config for display */
#ifdef CONFIG_LCD

#define CONFIG_RK3036_FB

#undef CONFIG_RK_HDMI
#ifdef CONFIG_RK_HDMI
#define CONFIG_RK3036_HDMI
#endif

#if defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK3128)
#undef CONFIG_RK3036_TVE
#endif /* CONFIG_RKCHIP_RK3036 */

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
#define CONFIG_RK32_DSI
#endif /* CONFIG_RKCHIP_RK3126 */

#undef CONFIG_UBOOT_CHARGE

#else

#undef CONFIG_RK_FB
#undef CONFIG_RK_PWM
#undef CONFIG_RK_HDMI
#undef CONFIG_CMD_BMP
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
#define CONFIG_POWER_ACT8931
#define CONFIG_POWER_RT5025
#define CONFIG_POWER_RT5036

#define CONFIG_POWER_FG_CW201X
#define CONFIG_POWER_FG_ADC

/********************************** battery driver ********************************/
#undef CONFIG_BATTERY_RK_SAMPLE
#undef CONFIG_BATTERY_BQ27541
#undef CONFIG_BATTERY_RICOH619

#endif /* CONFIG_POWER */

#endif /* __RK30PLAT_CONFIG_H */
