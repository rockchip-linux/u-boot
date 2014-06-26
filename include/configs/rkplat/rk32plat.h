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

#ifndef __RK32PLAT_H__
#define __RK32PLAT_H__


/* enable thume build */
//#define CONFIG_SYS_THUMB_BUILD

//#define CONFIG_SECOND_LEVEL_BOOTLOADER



/*
 *			Uboot memory map
 *
 * CONFIG_SYS_TEXT_BASE is the default address which maskrom loader uboot code.
 * CONFIG_RKNAND_API_ADDR is the address which maskrom loader miniloader code.
 *
 * For RK3288:
 *	kernel load address: CONFIG_SDRAM_PHY_START + 32M, size 16M,
 *	miniloader code load address: CONFIG_SDRAM_PHY_START + 48M, size 8M,
 *	total reverse memory is CONFIG_LMB_RESERVE_MEMORY_SIZE.
 *
 *|---------------------------------------------------------------------------|
 *|START  -  KERNEL LOADER  -  NAND LOADER  -     LMB     -    Uboot    -  END|
 *|SDRAM  -    START 32M    -   START 48M   -  START 56M  -  START 80M  - 128M|
 *|       -     kernel      -   nand code   -     fdt     -   uboot/ramdisk   |
 *|---------------------------------------------------------------------------|
 */
#include <linux/sizes.h>
#define CONFIG_RAM_PHY_START		0x00000000
#define CONFIG_RAM_PHY_SIZE		SZ_128M
#define CONFIG_RAM_PHY_END		(CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE)

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
	#define CONFIG_SYS_TEXT_BASE    0x00200000 //Resersed 2M space for packed nand bin.
#else
	#define CONFIG_SYS_TEXT_BASE    0x00000000
#endif

#define CONFIG_KERNEL_LOAD_ADDR 	(CONFIG_RAM_PHY_START + SZ_32M) //32M

/* rk nand api function code address */
#define CONFIG_RKNAND_API_ADDR		(CONFIG_RAM_PHY_START + SZ_32M + SZ_16M) //48M


/* rk mtd block size */
#define RK_BLK_SIZE			512


/* 
 * allow to flash loader when check sign failed. should undef this in release version.
 */
#define CONFIG_ENABLE_ERASEKEY


/* rk quick check sum */
#define CONFIG_QUICK_CHECKSUM


/* rk resouce module */
#define CONFIG_RESOURCE_PARTITION

#define CONFIG_MAX_PARTITIONS		16
#define CONFIG_OF_FROM_RESOURCE
#define CONFIG_BRIGHTNESS_DIM		48


/* rockusb */
#define CONFIG_CMD_ROCKUSB

/* for fastboot */
#define CONFIG_USBD_VENDORID		0x2207
#define CONFIG_USBD_PRODUCTID		0x0006
#define CONFIG_USBD_MANUFACTURER	"Rockchip"
#define CONFIG_USBD_PRODUCT_NAME	"rk30xx"

/* Fastboot product name */
#define FASTBOOT_PRODUCT_NAME		"fastboot"


/* for rockchip idblock.c setup_space. */
#define CONFIG_RK_GLOBAL_BUFFER_SIZE			(SZ_4M)
#define CONFIG_RK_EXTRA_BUFFER_SIZE			(CONFIG_RK_GLOBAL_BUFFER_SIZE + SZ_32M)


/*
 * Another macro may also be used or instead used to take care of the case
 * where fastboot is started at boot (to be incorporated) based on key press
 */
#define CONFIG_CMD_FASTBOOT

#define CONFIG_FASTBOOT_LOG
#define CONFIG_FASTBOOT_LOG_SIZE			(SZ_2M)

//CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE should be larger than our boot/recovery image size.
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE		(CONFIG_RK_EXTRA_BUFFER_SIZE - CONFIG_RK_GLOBAL_BUFFER_SIZE)
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH	(CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE >> 1)


/*
 * rockchip Hardware drivers
 */
/* dma config */
//#define CONFIG_RK_PL330			/* rk dma pl330 */
//#define CONFIG_RK_DMAC			/* rk dmac */

/* mmc using dma */
#define CONFIG_RK_MMC_DMA
#define CONFIG_RK_MMC_IDMAC		/* emmc/sdio using idmac */


/* rk clock module */
#define CONFIG_RK_CLOCK


/* rk usb module */
#define CONFIG_USB_DEVICE
#define CONFIG_RK_UDC

#define CONFIG_USB_DWC_HCD
//#define CONFIG_USB_EHCI
//#define CONFIG_USB_EHCI_RK
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE


/* rk gpio module */
#define CONFIG_RK_GPIO


/* rk iomux module */
#define CONFIG_RK_IOMUX


/* rk spi module */
//#define CONFIG_RK_SPI


/* rk serial module */
#define	CONFIG_RK_UART
#define CONFIG_UART_NUM			UART_CH2
#define CONFIG_RKUART2USB_FORCE		/* uart2usb force */


/* rk i2c module */
#define CONFIG_RK_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED 100000


/* rk pm management module */
#define CONFIG_PM_SUBSYSTEM


/* LCDC console */
#define CONFIG_LCD

#ifdef CONFIG_LCD

#define CONFIG_RK_PWM
#define CONFIG_RK_FB

#define CONFIG_LCD_LOGO
#define CONFIG_LCD_BMP_RLE8
#define CONFIG_CMD_BMP

//#define CONFIG_COMPRESS_LOGO_RLE8	/* CONFIG_COMPRESS_LOGO_RLE8 or CONFIG_COMPRESS_LOGO_RLE16 */

#define CONFIG_BMP_16BPP
#define CONFIG_SYS_WHITE_ON_BLACK
#define LCD_BPP				LCD_COLOR16

#define CONFIG_LCD_MAX_WIDTH		4096
#define CONFIG_LCD_MAX_HEIGHT		2048

//#define CONFIG_UBOOT_CHARGE
//#define CONFIG_CMD_CHARGE_ANIM
//#define CONFIG_CHARGE_DEEP_SLEEP

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
#define CONFIG_RK_3288_FB
#define CONFIG_RK_3288_DSI_UBOOT
#endif

#endif /* CONFIG_LCD */


/********************************** charger and pmic driver ********************************/
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_RICOH619
//#define CONFIG_POWER_RK_SAMPLE
#define CONFIG_POWER_RK808
#define CONFIG_POWER_ACT8846
#define CONFIG_SCREEN_ON_VOL_THRESD	3550
#define CONFIG_SYSTEM_ON_VOL_THRESD	3650
#define CONFIG_POWER_FG_CW201X


/********************************** battery driver ********************************/
//#define CONFIG_BATTERY_BQ27541
//#define CONFIG_BATTERY_RICOH619
//#define CONFIG_BATTERY_RK_SAMPLE

#endif /* __RK32PLAT_H__ */

