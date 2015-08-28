/*
 * Configuation settings for the rk33xx chip platform.
 *
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK33PLAT_CONFIG_H
#define __RK33PLAT_CONFIG_H

#include <asm/arch/io.h>


/* rk gic400 is GICV2 */
#define CONFIG_GICV2
#define GICD_BASE			RKIO_GICD_PHYS
#define GICC_BASE			RKIO_GICC_PHYS


/* Generic Timer Definitions */
#define COUNTER_FREQUENCY		CONFIG_SYS_CLK_FREQ_CRYSTAL


/*
 * uboot ram config.
 */
#include <linux/sizes.h>
#define CONFIG_RAM_PHY_START		0x00000000
#define CONFIG_RAM_PHY_SIZE		SZ_128M
#define CONFIG_RAM_PHY_END		(CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE)

/*
 * bl32 trust os ram config, trust os depend on config CONFIG_RK_TRUSTOS.
 */
#define CONFIG_RAM_SOS_START		CONFIG_RAM_PHY_END
#define CONFIG_RAM_SOS_SIZE		SZ_64M
#define CONFIG_RAM_SOS_END		(CONFIG_RAM_SOS_START + CONFIG_RAM_SOS_SIZE)


/* reserve iomap memory. */
#define CONFIG_MAX_MEM_ADDR		0xFF000000


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

/* do_nonsec_virt_switch when enter kernel */
#define CONFIG_ARMV8_SWITCH_TO_EL1

/* icache enable when start to kernel */
#define CONFIG_ICACHE_ENABLE_FOR_KERNEL


/* undef some module for rk chip */
#if defined(CONFIG_RKCHIP_RK3368)
	#define CONFIG_RK_MCU
	#define CONFIG_SECUREBOOT_CRYPTO
	#undef CONFIG_RK_TRUSTOS

	#undef CONFIG_RK_UMS_BOOT_EN
	#undef CONFIG_RK_PL330
	#undef CONFIG_RK_DMAC
#endif

/* fpga board configure */
#ifdef CONFIG_FPGA_BOARD
	#define CONFIG_BOARD_DEMO
	#define CONFIG_RK_IO_TOOL

	#define CONFIG_SKIP_RELOCATE_UBOOT
	#define CONFIG_SYS_ICACHE_OFF
	#define CONFIG_SYS_DCACHE_OFF
	#define CONFIG_SYS_L2CACHE_OFF
	#define CONFIG_RKTIMER_INCREMENTER

	#undef CONFIG_RK_MCU
	#undef CONFIG_SECUREBOOT_CRYPTO
	#undef CONFIG_MERGER_MINILOADER
	#undef CONFIG_MERGER_TRUSTIMAGE
	#undef CONFIG_RK_SDCARD_BOOT_EN
	#undef CONFIG_RK_FLASH_BOOT_EN
	#undef CONFIG_RK_UMS_BOOT_EN
	#undef CONFIG_LCD
	#undef CONFIG_POWER
	#undef CONFIG_POWER_RK
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
#if defined(CONFIG_SECOND_LEVEL_BOOTLOADER) && defined(CONFIG_SECUREBOOT_CRYPTO)
#define CONFIG_SECURE_RSA_KEY_IN_RAM
#define CONFIG_SECURE_RSA_KEY_ADDR	(CONFIG_RKNAND_API_ADDR + SZ_2K)
#endif /* CONFIG_SECUREBOOT_CRYPTO */


/* mod it to enable console commands.	*/
#define CONFIG_BOOTDELAY		0

/* mmc using dma */
#define CONFIG_RK_MMC_DMA
#define CONFIG_RK_MMC_IDMAC	/* internal dmac */
#undef CONFIG_RK_MMC_DDR_MODE	/* mmc using ddr mode */

/* more config for rockusb */
#ifdef CONFIG_CMD_ROCKUSB

/* support rockusb timeout check */
#define CONFIG_ROCKUSB_TIMEOUT_CHECK	1

/* rockusb VID/PID should the same as maskrom */
#define CONFIG_USBD_VENDORID			0x2207
#if defined(CONFIG_RKCHIP_RK3368)
	#define CONFIG_USBD_PRODUCTID_ROCKUSB	0x330A
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
 *	RKUSB_UMS_BOOT_FROM_OTG
 *	RKUSB_UMS_BOOT_FROM_HOST1
 *	RKUSB_UMS_BOOT_FROM_HOST2
 *
 * First define the host controller here
 */


/* Check UMS Boot Host define */
#define RKUSB_UMS_BOOT_CNT (defined(RKUSB_UMS_BOOT_FROM_OTG) + \
			    defined(RKUSB_UMS_BOOT_FROM_HOST1) + \
			    defined(RKUSB_UMS_BOOT_FROM_HOST2))

#if (RKUSB_UMS_BOOT_CNT == 0)
	#error "PLS Select a USB host controller!"
#elif (RKUSB_UMS_BOOT_CNT > 1)
	#error "Only one USB host controller can be selected!"
#else
	#define CONFIG_CMD_USB
	#define CONFIG_USB_STORAGE
	#define CONFIG_PARTITIONS
#endif


/*
 * USB Host support, default no using
 * please first check plat if you want to using usb host
 */
#if defined(RKUSB_UMS_BOOT_FROM_HOST1)
	#define CONFIG_USB_EHCI
	#define CONFIG_USB_EHCI_RK
#elif defined(RKUSB_UMS_BOOT_FROM_HOST2) || defined(RKUSB_UMS_BOOT_FROM_OTG)
	#define CONFIG_USB_DWC_HCD
#endif
#endif /* CONFIG_RK_UMS_BOOT_EN */


/* more config for display */
#ifdef CONFIG_LCD

#define CONFIG_RK33_FB

#ifdef CONFIG_RK_HDMI
#define CONFIG_RK_HDMIV2
#endif

#ifdef CONFIG_PRODUCT_BOX
#define CONFIG_RK1000_TVE
#undef CONFIG_GM7122_TVE
#endif

#define CONFIG_RK32_DSI

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

#ifdef CONFIG_CHARGE_DEEP_SLEEP
#define CONFIG_CHARGE_TIMER_WAKEUP
#endif
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

#endif /* __RK33PLAT_CONFIG_H */
