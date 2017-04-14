/*
 * Configuation settings for the rk33xx chip platform.
 *
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK33PLAT_CONFIG_H
#define __RK33PLAT_CONFIG_H

#include <asm/arch/io.h>


/* gic and rk timer version */
#if defined(CONFIG_RKCHIP_RK3368) || defined(CONFIG_RKCHIP_RK3366) || defined(CONFIG_RKCHIP_RK322XH)
	#define CONFIG_GICV2
	#define CONFIG_RKTIMER_V2
#elif defined(CONFIG_RKCHIP_RK3399)
	#define CONFIG_GICV3
	#define CONFIG_RKTIMER_V3
#else
	#error "PLS config rk chip for GIC and TIMER version!"
#endif

/* gic base */
#if defined(CONFIG_GICV2)
	#define GICD_BASE		RKIO_GICD_PHYS
	#define GICC_BASE		RKIO_GICC_PHYS
#elif defined(CONFIG_GICV3)
	#define GICC_BASE		RKIO_GICC_PHYS
	#define GICD_BASE		RKIO_GICD_PHYS
	#define GICR_BASE		RKIO_GICR_PHYS
#endif /* CONFIG_GICV2 */

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY		CONFIG_SYS_CLK_FREQ_CRYSTAL


/*
 * uboot ram config.
 */
#include <linux/sizes.h>
#define CONFIG_RAM_PHY_START		0x00000000
#define CONFIG_RAM_PHY_SIZE		SZ_128M
#define CONFIG_RAM_PHY_END		(CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE)


/* reserve iomap memory. */
#define CONFIG_MAX_MEM_ADDR		RKIO_IOMEMORYMAP_START


/*
 * 		define uboot loader addr.
 * notice: CONFIG_SYS_TEXT_BASE must be an immediate,
 * so if CONFIG_RAM_PHY_START is changed, also update CONFIG_SYS_TEXT_BASE define.
 *
 * Resersed 2M space(0 - 2M) for Runtime ARM Firmware bin, such as bl30/bl31/bl32 and so on.
 *
 */
#ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
       #define CONFIG_SYS_TEXT_BASE    0x00200000 /* Resersed 2M space Runtime Firmware bin. */
#else
       #define CONFIG_SYS_TEXT_BASE    0x00000000
#endif


/* kernel load to the running address */
#define CONFIG_KERNEL_RUNNING_ADDR	(CONFIG_SYS_TEXT_BASE + SZ_512K)


/*
 * rk plat default configs.
 */
#include <configs/rk_default_config.h>

/* el3 switch to el1 disable */
#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
#define CONFIG_SWITCH_EL3_TO_EL1
#endif

/* icache enable when start to kernel */
#define CONFIG_ICACHE_ENABLE_FOR_KERNEL

#define CONFIG_OF_BOARD_SETUP

/* undef some module for rk chip */
#if defined(CONFIG_RKCHIP_RK3368)
	#define CONFIG_RK_MCU
	#define CONFIG_SECUREBOOT_CRYPTO
	#define CONFIG_SECUREBOOT_SHA256
	#undef CONFIG_RK_TRUSTOS

	#undef CONFIG_RK_UMS_BOOT_EN
	#undef CONFIG_RK_PL330_DMAC
#endif

#if defined(CONFIG_RKCHIP_RK3366)
	#define CONFIG_RKTIMER_INCREMENTER

	#undef CONFIG_RK_MCU
	#define CONFIG_SECUREBOOT_CRYPTO
	#define CONFIG_SECUREBOOT_SHA256
	#undef CONFIG_RK_TRUSTOS

	#undef CONFIG_MERGER_TRUSTIMAGE

	#undef CONFIG_RK_UMS_BOOT_EN

	#undef CONFIG_RK_PL330_DMAC
	#undef CONFIG_LCD
	#undef CONFIG_PM_SUBSYSTEM
#endif


#if defined(CONFIG_RKCHIP_RK3399)
	#define CONFIG_SECUREBOOT_SHA256
	#define CONFIG_RKTIMER_INCREMENTER
	#define CONFIG_RK_SDHCI_BOOT_EN
	#undef CONFIG_RK_SDMMC_BOOT_EN
	#undef CONFIG_RK_FLASH_BOOT_EN
	#undef CONFIG_RK_UMS_BOOT_EN

	#undef CONFIG_RK_MCU
	#undef CONFIG_PERILP_MCU
	#undef CONFIG_PMU_MCU
	#undef CONFIG_RK_PL330_DMAC
	#if (defined(CONFIG_CMD_ROCKUSB) || defined(CONFIG_CMD_FASTBOOT))
		#undef CONFIG_RK_UDC
		#define CONFIG_RK_DWC3_UDC
	#endif
	#define CONFIG_RK_GPIO_EXT_FUNC
	#define CONFIG_CHARGE_LED
	#define CONFIG_POWER_FUSB302
#endif

#if defined(CONFIG_RKCHIP_RK322XH)
	#undef CONFIG_RK_MCU

	#define CONFIG_SECUREBOOT_CRYPTO
	#define CONFIG_SECUREBOOT_SHA256
	#undef CONFIG_RK_TRUSTOS

	#undef CONFIG_RK_FLASH_BOOT_EN
	#undef CONFIG_RK_UMS_BOOT_EN

	#undef CONFIG_RK_PL330_DMAC
	#undef CONFIG_MERGER_MINILOADER
#endif

/* fpga board configure */
#ifdef CONFIG_FPGA_BOARD
	#define DEBUG
	#define CONFIG_BOARD_DEMO
	#define CONFIG_RK_IO_TOOL

	#define CONFIG_SKIP_RELOCATE_UBOOT
	#define CONFIG_SYS_ICACHE_OFF
	#define CONFIG_SYS_DCACHE_OFF
	#define CONFIG_SYS_L2CACHE_OFF
	#define CONFIG_RKTIMER_INCREMENTER

	#undef CONFIG_RK_PL330_DMAC
	#undef CONFIG_RK_MCU
	#undef CONFIG_SECUREBOOT_CRYPTO
	#undef CONFIG_MERGER_MINILOADER
	#undef CONFIG_MERGER_TRUSTIMAGE
	#undef CONFIG_RK_SDCARD_BOOT_EN
	#undef CONFIG_RK_FLASH_BOOT_EN
	#undef CONFIG_RK_UMS_BOOT_EN
	#undef CONFIG_LCD
	#undef CONFIG_RK_POWER
	#undef CONFIG_PM_SUBSYSTEM
	#undef CONFIG_RK_CLOCK
	#undef CONFIG_RK_IOMUX
	#undef CONFIG_RK_I2C
	#undef CONFIG_RK_KEY
#endif

/* if uboot as first level loader, no start mcu. */
#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
	#undef CONFIG_RK_MCU
#endif

/* ARMv8 RSA key in ram, MiniLoader copy RSA KEY to fixed address */
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
	#define CONFIG_RKEFUSE_V2

	/* store the content of efuse non-secure */
	#define CONFIG_EFUSE_NS_INFO_ADDR	(CONFIG_RAM_PHY_START + SZ_1M + SZ_1K * 60)
#endif


/* sdhci config */
#ifdef CONFIG_RK_SDHCI_BOOT_EN
	/* general sdhci driver */
	#undef CONFIG_MMC
	#undef CONFIG_GENERIC_MMC
	#undef CONFIG_PARTITIONS
	#undef CONFIG_SDHCI
	#undef CONFIG_RK_SDHCI

	/* rk arasan sdhci driver */
	#define CONFIG_RK_AR_SDHCI
#endif


/* mmc using dma */
#define CONFIG_RK_MMC_DMA
#define CONFIG_RK_MMC_IDMAC	/* internal dmac */

#if defined(CONFIG_RKCHIP_RK322XH)
	#define CONFIG_RK_MMC_DDR_MODE	/* mmc using ddr mode */
#else
	#undef CONFIG_RK_MMC_DDR_MODE	/* mmc using ddr mode */
#endif

#if (defined(CONFIG_CMD_ROCKUSB) || defined(CONFIG_CMD_FASTBOOT))
	#define CONFIG_USBD_MANUFACTURER	"Rockchip"
	#define CONFIG_USBD_PRODUCT_NAME	"rk30xx"
#endif

/* more config for rockusb */
#ifdef CONFIG_CMD_ROCKUSB

/* support rockusb timeout check */
#define CONFIG_ROCKUSB_TIMEOUT_CHECK	1

/* rockusb VID/PID should the same as maskrom */
#define CONFIG_USBD_VENDORID			0x2207
#if defined(CONFIG_RKCHIP_RK3368)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x330A
#elif defined(CONFIG_RKCHIP_RK3366)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x330B
#elif defined(CONFIG_RKCHIP_RK3399)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x330C
#elif defined(CONFIG_RKCHIP_RK322XH)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x320C
#else
	#error "PLS config rk chip for rockusb PID!"
#endif

#endif /* CONFIG_CMD_ROCKUSB */


/* more config for fastboot */
#ifdef CONFIG_CMD_FASTBOOT

#define CONFIG_USBD_PRODUCTID_FASTBOOT	0x0006


#define FASTBOOT_PRODUCT_NAME		"fastboot" /* Fastboot product name */

#define CONFIG_FASTBOOT_LOG
#define CONFIG_FASTBOOT_LOG_SIZE	(SZ_2M)

#endif /* CONFIG_CMD_FASTBOOT */

#ifdef CONFIG_RK_DWC3_UDC
	#define CONFIG_USB_DWC3
	#define CONFIG_USB_DWC3_GADGET
#endif


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
 *	RKUSB_UMS_BOOT_FROM_EHCI_HOST2
 *
 * First define the host controller here
 */
#undef RKUSB_UMS_BOOT_FROM_DWC2_OTG
#undef RKUSB_UMS_BOOT_FROM_DWC2_HOST
#undef RKUSB_UMS_BOOT_FROM_EHCI_HOST1
#undef RKUSB_UMS_BOOT_FROM_EHCI_HOST2


/* Check UMS Boot Host define */
#define RKUSB_UMS_BOOT_CNT (defined(RKUSB_UMS_BOOT_FROM_DWC2_OTG) + \
			    defined(RKUSB_UMS_BOOT_FROM_DWC2_HOST) + \
			    defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST1) + \
			    defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST2))

#if (RKUSB_UMS_BOOT_CNT == 0)
	#error "PLS Select a USB host controller!"
#elif (RKUSB_UMS_BOOT_CNT > 1)
	#error "Only one USB host controller can be selected!"
#endif


/*
 * USB Host support, default no using
 * please first check plat if you want to using usb host
 */
#if defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST1) ||\
	defined(RKUSB_UMS_BOOT_FROM_EHCI_HOST2)
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
#if defined(CONFIG_RKCHIP_RK3399)
#define CONFIG_ROCKCHIP_DISPLAY
#define CONFIG_RK322X_FB
#define CONFIG_DIRECT_LOGO
#define CONFIG_OF_BOARD_SETUP
#undef CONFIG_RK_TVE
#undef CONFIG_RK_VOP_DUAL_ANY_FREQ_PLL
#endif

#if defined(CONFIG_RKCHIP_RK322XH)
#define CONFIG_RK_VOP_FULL_FB
#define CONFIG_DIRECT_LOGO
#define CONFIG_RK_HDMI
#define CONFIG_RK3036_TVE
#undef CONFIG_LOGO_HASH_CHECK
#undef CONFIG_RK_TVE
#endif
#if defined(CONFIG_RKCHIP_RK3368)
#define CONFIG_ROCKCHIP_DISPLAY
#define CONFIG_RK33_FB
#endif

#ifdef CONFIG_ROCKCHIP_DISPLAY
#define CONFIG_ROCKCHIP_VOP
#define CONFIG_ROCKCHIP_MIPI_DSI
#define CONFIG_ROCKCHIP_DW_MIPI_DSI
#define CONFIG_ROCKCHIP_LVDS
#define CONFIG_ROCKCHIP_ANALOGIX_DP
#define CONFIG_ROCKCHIP_DW_HDMI
#define CONFIG_ROCKCHIP_PANEL
#define CONFIG_I2C_EDID
#define CONFIG_RK_VOP_DUAL_ANY_FREQ_PLL
#endif

#ifdef CONFIG_RK_HDMI
#define CONFIG_RK_HDMIV2
#endif

#ifdef CONFIG_RK_TVE
#define CONFIG_RK1000_TVE
#undef CONFIG_GM7122_TVE
#endif

#if !defined(CONFIG_RKCHIP_RK322XH)
#define CONFIG_RK32_DSI
#define CONFIG_RK3399_EDP
#endif

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

#ifdef CONFIG_CHARGE_DEEP_SLEEP
#define CONFIG_CHARGE_TIMER_WAKEUP
#endif
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
#define CONFIG_POWER_CHARGER
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
#ifdef CONFIG_POWER_CHARGER
#define CONFIG_CHARGER_BQ25700
#endif

#ifdef CONFIG_POWER_FG
#define CONFIG_POWER_FG_CW201X
#endif /* CONFIG_POWER_FG */

/******** battery driver ********/
#ifdef CONFIG_POWER_BAT
#undef CONFIG_BATTERY_RK_SAMPLE
#undef CONFIG_BATTERY_BQ27541
#undef CONFIG_BATTERY_RICOH619
#define CONFIG_BATTERY_EC
#endif /* CONFIG_POWER_BAT */

#endif /* CONFIG_RK_POWER */

#endif /* __RK33PLAT_CONFIG_H */
