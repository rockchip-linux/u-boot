/*
 * Configuation settings for the rk312x chip platform.
 *
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
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
 */
#define CONFIG_SYS_TEXT_BASE    	0x60000000

/*
 * rk plat default configs.
 */
#include <configs/rk_default_config.h>

/* undef some module for rk chip */
#if defined(CONFIG_RKCHIP_RK3036)
	#define CONFIG_MERGER_TRUSTOS
	#undef CONFIG_RK_UMS_BOOT_EN
	#undef CONFIG_RK_POWER

	#undef CONFIG_CMD_NET
	#undef CONFIG_RK_GMAC
#endif /* CONFIG_RKCHIP_RK3036 */

#if defined(CONFIG_RKCHIP_RK3126)
	#undef CONFIG_RK_UMS_BOOT_EN

	#undef CONFIG_CMD_NET
	#undef CONFIG_RK_GMAC
	#define CONFIG_MERGER_TRUSTOS
	#define CONFIG_OPTEE_CLIENT
	#define CONFIG_OPTEE_V1
#endif /* CONFIG_RKCHIP_RK3126 */

#if defined(CONFIG_RKCHIP_RK3128)
	#define CONFIG_SECUREBOOT_CRYPTO

	#undef CONFIG_RK_UMS_BOOT_EN

	#undef CONFIG_CMD_NET
	#undef CONFIG_RK_GMAC
#endif /* CONFIG_RKCHIP_RK3128 */

#if defined(CONFIG_RKCHIP_RK322X)
	#define CONFIG_MERGER_TRUSTOS
	#define CONFIG_OPTEE_CLIENT
	#define CONFIG_OPTEE_V1
	#define CONFIG_SECUREBOOT_CRYPTO
	#define CONFIG_SECUREBOOT_SHA256
	#define CONFIG_RK_PSCI
	#define CONFIG_RK_DCF

	#undef CONFIG_RK_UMS_BOOT_EN
	#undef CONFIG_PM_SUBSYSTEM

	#undef CONFIG_CMD_NET
	#undef CONFIG_RK_GMAC
#endif /* CONFIG_RKCHIP_RK322X */


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

/* efuse version */
#ifdef CONFIG_RK_EFUSE
#if defined(CONFIG_RKCHIP_RK322X)
	#define CONFIG_RKEFUSE_V2
#else
	#define CONFIG_RKEFUSE_V1
#endif
#endif

/* mmc using dma */
#define CONFIG_RK_MMC_DMA
#if defined(CONFIG_RKCHIP_RK322X)
#define CONFIG_RK_MMC_IDMAC            /* internal dmac */
#else
#define CONFIG_RK_MMC_EDMAC		/* external mac */
#endif
#undef CONFIG_RK_MMC_DDR_MODE		/* mmc using ddr mode */

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
#if defined(CONFIG_RKCHIP_RK3036)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x301A
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x310C
#elif defined(CONFIG_RKCHIP_RK322X)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB   0x320B
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
 * PLS define a host controller from:
 *	RKUSB_UMS_BOOT_FROM_DWC2_OTG
 *	RKUSB_UMS_BOOT_FROM_DWC2_HOST
 *	RKUSB_UMS_BOOT_FROM_EHCI_HOST1
 *	RKUSB_UMS_BOOT_FROM_EHCI_HOST2
 *	RKUSB_UMS_BOOT_FROM_EHCI_HOST3
 *
 * First define the host controller here
 */
#undef RKUSB_UMS_BOOT_FROM_DWC2_OTG
#undef RKUSB_UMS_BOOT_FROM_DWC2_HOST
#undef RKUSB_UMS_BOOT_FROM_EHCI_HOST1
#undef RKUSB_UMS_BOOT_FROM_EHCI_HOST2
#undef RKUSB_UMS_BOOT_FROM_EHCI_HOST3


/* Check UMS Boot Host define */
#define RKUSB_UMS_BOOT_CNT (defined(RKUSB_UMS_BOOT_FROM_DWC2_OTG) + \
			    defined(RKUSB_UMS_BOOT_FROM_DWC2_HOST) + \
			    defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST1) + \
			    defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST2) + \
			    defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST3))

#if (RKUSB_UMS_BOOT_CNT == 0)
	#error "PLS Select a USB host controller!"
#elif (RKUSB_UMS_BOOT_CNT > 1)
	#error "Only one USB host controller can be selected!"
#endif


/*
 * USB Host support, default no using
 * please first check plat if you want to using usb host
 */
#if defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST1) || defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST2) \
		|| defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST3)
	/* ehci host */
	#define CONFIG_USB_EHCI
	#define CONFIG_USB_EHCI_RK
#elif defined(RKUSB_UMS_BOOT_FROM_DWC2_HOST) || defined(RKUSB_UMS_BOOT_FROM_DWC2_OTG)
	/* dwc2 host or otg */
	#define CONFIG_USB_DWC_HCD
#endif


/* enable usb config for usb host */
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_PARTITIONS
#endif /* CONFIG_RK_UMS_BOOT_EN */


/* more config for display */
#ifdef CONFIG_LCD
#define CONFIG_ROCKCHIP_DISPLAY
#ifdef CONFIG_ROCKCHIP_DISPLAY
#define CONFIG_ROCKCHIP_LVDS
#define CONFIG_ROCKCHIP_VOP
#define CONFIG_ROCKCHIP_MIPI_DSI
#define CONFIG_ROCKCHIP_DW_MIPI_DSI
#define CONFIG_ROCKCHIP_ANALOGIX_DP
#define CONFIG_ROCKCHIP_PANEL
#define CONFIG_DRM_ROCKCHIP_DW_HDMI
#define CONFIG_ROCKCHIP_DRM_TVE
#if defined(CONFIG_RKCHIP_RK322X)
	#define CONFIG_RKCHIP_INNO_HDMI_PHY
#endif
#define CONFIG_I2C_EDID
#define CONFIG_OF_BOARD_SETUP
#endif

#if defined(CONFIG_RKCHIP_RK322X)
#define CONFIG_RK322X_FB
#define CONFIG_DIRECT_LOGO
#else
#define CONFIG_RK3036_FB
#endif

#ifdef CONFIG_RK_HDMI
#if defined(CONFIG_RKCHIP_RK322X)
#define CONFIG_RK_HDMIV2
#else
#define CONFIG_RK3036_HDMI
#endif
#endif /* CONFIG_RK_HDMI */

#ifdef CONFIG_RK_TVE
#define CONFIG_RK3036_TVE
#endif /* CONFIG_RK_TVE */

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
#define CONFIG_RK32_DSI
#endif /* CONFIG_RKCHIP_RK3126 */

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
#undef CONFIG_CHARGE_DEEP_SLEEP

#endif /* CONFIG_UBOOT_CHARGE */


/* more config for power */
#ifdef CONFIG_RK_POWER

#define CONFIG_POWER
#define CONFIG_POWER_I2C

/* if box product, undefine fg and battery */
#ifndef CONFIG_PRODUCT_BOX
#define CONFIG_POWER_PMIC
#define CONFIG_POWER_FG
#define CONFIG_POWER_BAT
#endif
/* CONFIG_PRODUCT_BOX */

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
#define CONFIG_POWER_ACT8931
#define CONFIG_POWER_RT5025
#define CONFIG_POWER_RT5036
#endif /* CONFIG_POWER_PMIC */

#ifdef CONFIG_RKCHIP_RK3128
#define CONFIG_POWER_RK818
#endif
/******** charger driver ********/
#ifdef CONFIG_POWER_FG
#define CONFIG_POWER_FG_CW201X
#define CONFIG_POWER_FG_ADC
#endif /* CONFIG_POWER_FG */

/******** battery driver ********/
#ifdef CONFIG_POWER_BAT
#undef CONFIG_BATTERY_RK_SAMPLE
#undef CONFIG_BATTERY_BQ27541
#undef CONFIG_BATTERY_RICOH619
#endif /* CONFIG_POWER_BAT */

#endif /* CONFIG_RK_POWER */

#endif /* __RK30PLAT_CONFIG_H */
