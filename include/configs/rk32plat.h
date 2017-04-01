/*
 * Configuation settings for the rk312x chip platform.
 *
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
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
#define CONFIG_MAX_MEM_ADDR		0xFE000000


/*
 * 		define uboot loader addr.
 * notice: CONFIG_SYS_TEXT_BASE must be an immediate,
 * so if CONFIG_RAM_PHY_START is changed, also update CONFIG_SYS_TEXT_BASE define.
 *
 */
#define CONFIG_SYS_TEXT_BASE    	0x00000000


/*
 * rk plat default configs.
 */
#include <configs/rk_default_config.h>

/* config for sha256 image check */
#define CONFIG_SECUREBOOT_SHA256

/* undef some module for rk chip */
#if defined(CONFIG_RKCHIP_RK3288)
	#define CONFIG_MERGER_TRUSTOS
	#define CONFIG_SECUREBOOT_CRYPTO

	#undef CONFIG_RK_UMS_BOOT_EN
	#undef CONFIG_RK_PL330_DMAC

	#undef CONFIG_CMD_NET
	#undef CONFIG_RK_GMAC
#endif

/* if working normal world, secure efuse can't read,
 * MiniLoader copy RSA KEY to sdram for uboot.
 */
#if defined(CONFIG_SECUREBOOT_CRYPTO)
#if defined(CONFIG_SECOND_LEVEL_BOOTLOADER) && defined(CONFIG_NORMAL_WORLD)
	#define CONFIG_SECURE_RSA_KEY_IN_RAM
	#define CONFIG_SECURE_RSA_KEY_ADDR	(CONFIG_RKNAND_API_ADDR + SZ_2K)
#endif /* CONFIG_NORMAL_WORLD && CONFIG_SECOND_LEVEL_BOOTLOADER */
#endif /* CONFIG_SECUREBOOT_CRYPTO */

/* mod it to enable console commands.	*/
#define CONFIG_BOOTDELAY		0
#define CONFIG_DIRECT_LOGO

/* efuse version */
#ifdef CONFIG_RK_EFUSE
	#define CONFIG_RKEFUSE_V2
#endif

/* mmc using dma */
#define CONFIG_RK_MMC_DMA
#define CONFIG_RK_MMC_IDMAC	/* internal dmac */
#undef CONFIG_RK_MMC_DDR_MODE	/* mmc using ddr mode */

/* net command support */
#ifdef CONFIG_CMD_NET
#define CONFIG_CMD_PING
#endif

/* Ethernet support */
#ifdef CONFIG_RK_GMAC
#define CONFIG_DESIGNWARE_ETH		/* GMAC can use designware driver */
#define CONFIG_DW_AUTONEG
#define CONFIG_PHY_REALTEK
#define CONFIG_PHY_ADDR		1
#define CONFIG_RGMII			/* RGMII PHY management		*/
#define CONFIG_PHYLIB
#endif

/* more config for rockusb */
#ifdef CONFIG_CMD_ROCKUSB

/* support rockusb timeout check */
#define CONFIG_ROCKUSB_TIMEOUT_CHECK	1

/* rockusb VID/PID should the same as maskrom */
#define CONFIG_USBD_VENDORID			0x2207
#if defined(CONFIG_RKCHIP_RK3288)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x320A
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


#ifdef CONFIG_RK_UMS_BOOT_EN
/*
 * USB Host support, default no using
 * Please first select USB host controller if you want to use UMS Boot
 * Up to one USB host controller could be selected to enable for booting
 * from USB Mass Storage device.
 *
 * PLS define a host controler from:
 *	RKUSB_UMS_BOOT_FROM_DWC2_OTG
 *	RKUSB_UMS_BOOT_FROM_DWC2_HOST
 *	RKUSB_UMS_BOOT_FROM_EHCI_HOST1
 *
 * First define the host controller here
 */
#undef RKUSB_UMS_BOOT_FROM_DWC2_OTG
#undef RKUSB_UMS_BOOT_FROM_DWC2_HOST
#undef RKUSB_UMS_BOOT_FROM_EHCI_HOST1


/* Check UMS Boot Host define */
#define RKUSB_UMS_BOOT_CNT (defined(RKUSB_UMS_BOOT_FROM_DWC2_OTG) + \
			    defined(RKUSB_UMS_BOOT_FROM_DWC2_HOST) + \
			    defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST1))

#if (RKUSB_UMS_BOOT_CNT == 0)
	#error "PLS Select a USB host controller!"
#elif (RKUSB_UMS_BOOT_CNT > 1)
	#error "Only one USB host controller can be selected!"
#endif


/*
 * USB Host support, default no using
 * please first check plat if you want to using usb host
 */
#if defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST1)
	#define CONFIG_USB_EHCI
	#define CONFIG_USB_EHCI_RK
#elif defined(RKUSB_UMS_BOOT_FROM_DWC2_HOST) || defined(RKUSB_UMS_BOOT_FROM_DWC2_OTG)
	#define CONFIG_USB_DWC_HCD
#endif


/* enable usb config for usb host */
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_PARTITIONS
#endif /* CONFIG_RK_UMS_BOOT_EN */


/* more config for display */
#ifdef CONFIG_LCD

#define CONFIG_RK32_FB

#ifdef CONFIG_RK_HDMI
#define CONFIG_RK_HDMIV2
#endif

#ifdef CONFIG_RK_TVE
#define CONFIG_RK1000_TVE
#undef CONFIG_GM7122_TVE
#endif

#define CONFIG_RK32_DSI

#undef CONFIG_UBOOT_CHARGE

#else

#undef CONFIG_RK_FB
#undef CONFIG_RK_PWM_BL
#undef CONFIG_RK_HDMI
#undef CONFIG_RK_TVE
#undef CONFIG_CMD_BMP
#undef CONFIG_UBOOT_CHARGE

#endif /* CONFIG_LCD */


/* more config for charge */
#ifdef CONFIG_UBOOT_CHARGE

#define CONFIG_CMD_CHARGE_ANIM
#define CONFIG_CHARGE_DEEP_SLEEP

#endif /* CONFIG_UBOOT_CHARGE */


/* more config for power */
#ifdef CONFIG_RK_POWER

#define CONFIG_POWER
#define CONFIG_POWER_I2C

#define CONFIG_POWER_PMIC
/* if box product, undefine fg and battery */
#ifndef CONFIG_PRODUCT_BOX
#define CONFIG_POWER_FG
#define CONFIG_POWER_BAT
#endif /* CONFIG_PRODUCT_BOX */

#define CONFIG_SCREEN_ON_VOL_THRESD	0
#define CONFIG_SYSTEM_ON_VOL_THRESD	0

/******** pwm regulator driver ********/
#define CONFIG_POWER_PWM_REGULATOR

/******** pmic driver ********/
#ifdef CONFIG_POWER_PMIC
#undef CONFIG_POWER_RK_SAMPLE
#define CONFIG_POWER_RICOH619
#define CONFIG_POWER_RK808
#define CONFIG_POWER_RK818
#define CONFIG_POWER_ACT8846
#endif /* CONFIG_POWER_PMIC */

/******** charger driver ********/
#ifdef CONFIG_POWER_FG
#define CONFIG_POWER_FG_CW201X
#endif /* CONFIG_POWER_FG */

/******** battery driver ********/
#ifdef CONFIG_POWER_BAT
#undef CONFIG_BATTERY_RK_SAMPLE
#undef CONFIG_BATTERY_BQ27541
#undef CONFIG_BATTERY_RICOH619
#endif /* CONFIG_POWER_BAT */

#endif /* CONFIG_RK_POWER */

#endif /* __RK32PLAT_CONFIG_H */
