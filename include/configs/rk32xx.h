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

#ifndef __CONFIG_H
#define __CONFIG_H


/* config rockchip chip type plat form */
#define CONFIG_RK3066		1
#define CONFIG_RK3066B		2
#define CONFIG_RK3168		3
#define CONFIG_RK3188		4
#define CONFIG_RK3188B		5
#define CONFIG_RK3188T		6
#define CONFIG_RK3026		7
#define CONFIG_RK2928		8
#define CONFIG_RK3288		9
#define CONFIG_RK3299		10

#define CONFIG_RKCHIPTYPE	CONFIG_RK3288


/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_ROCKCHIP		1	/* in a ROCKCHIP core */

//#define CONFIG_SECOND_LEVEL_BOOTLOADER 

#define HAVE_VENDOR_COMMON_LIB y

/* Display CPU and Board Info */
#define CONFIG_ARCH_CPU_INIT
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SETUP_MEMORY_TAGS	/* enable memory tag 		*/
#define CONFIG_CMDLINE_TAG		/* enable passing of ATAGs	*/
#define CONFIG_CMDLINE_EDITING		/* add command line history	*/
#define CONFIG_INITRD_TAG		/* Required for ramdisk support */
#define CONFIG_BOARD_LATE_INIT


/*
 * cache config
 */
//#define CONFIG_SYS_ICACHE_OFF
//#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_SYS_L2CACHE_OFF
#define CONFIG_SYS_ARM_CACHE_WRITETHROUGH


/* irq config */
#define CONFIG_USE_IRQ


/*
 * Enabling relocation of u-boot by default
 * Relocation can be skipped if u-boot is copied to the TEXT_BASE
 */
#undef CONFIG_SKIP_RELOCATE_UBOOT	/* to a proper address, init done */


/*
 * Size of malloc() pool
 * 1MB = 0x100000, 0x100000 = 1024 * 1024
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (1 << 20))


/*
 * select serial console configuration
 */
#define CONFIG_SERIAL2			1	/* use SERIAL2 */
#define CONFIG_BAUDRATE			115200
/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }


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
 *|       -     kernel      -   nand code   - ramdisk/fdt -    uboot    -     |
 *|---------------------------------------------------------------------------|
 */
#define RAM_PHY_SIZE		0x08000000
#define RAM_PHY_START		0x00000000
#define RAM_PHY_END             (RAM_PHY_START + RAM_PHY_SIZE)

//define uboot loader addr.
#ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
	//2m offset for packed nand bin.
	#define CONFIG_SYS_TEXT_BASE    0x00200000
#else
	#define CONFIG_SYS_TEXT_BASE    0x00000000
#endif

// rk nand api function code address 
#define CONFIG_RKNAND_API_ADDR		(RAM_PHY_START + 0x3000000)//48M


/* input clock of PLL: has 24MHz input clock at rk30xx */
#define CONFIG_SYS_CLK_FREQ_CRYSTAL	24000000
#define CONFIG_SYS_CLK_FREQ		CONFIG_SYS_CLK_FREQ_CRYSTAL
#define CONFIG_SYS_HZ			1000	/* decrementer freq: 1 ms ticks */

/* Declare no flash (NOR/SPI) */
#define CONFIG_SYS_NO_FLASH		1       /* It should define before config_cmd_default.h */


#define CONFIG_CMD_CACHE	/* icache, dcache		 */
#define CONFIG_CMD_REGINFO	/* Register dump		 */


// mod it to enable console commands.
#define CONFIG_BOOTDELAY 	0


/*
 * Environment setup
 */
/* use preboot to detect key press for fastboot */
#define CONFIG_PREBOOT
#define CONFIG_BOOTCOMMAND		"booti"

#define CONFIG_EXTRA_ENV_SETTINGS	"verify=n\0"


/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT		"rk30boot # "
#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS		16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_SYS_RAMBOOT
#define CONFIG_SYS_VSNPRINTF

/*
 * SDRAM Memory Map
 * Even though we use two CS all the memory
 * is mapped to one contiguous block
 */
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM_1            	CONFIG_SYS_SDRAM_BASE /* OneDRAM Bank #0 */
#define PHYS_SDRAM_1_SIZE       	(RAM_PHY_END - RAM_PHY_START) /* 128 MB in Bank #0 */

/* DRAM Base */
#define CONFIG_SYS_SDRAM_BASE		RAM_PHY_START		/* Physical start address of SDRAM. */
#define CONFIG_SYS_SDRAM_SIZE   	PHYS_SDRAM_1_SIZE

/* Default load address */
#define CONFIG_SYS_LOAD_ADDR		CONFIG_SYS_SDRAM_BASE

//sp addr before relocate.
#define CONFIG_SYS_INIT_SP_ADDR		RAM_PHY_END

/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#ifdef CONFIG_USE_IRQ
#  define CONFIG_STACKSIZE_IRQ	0x10000
#  define CONFIG_STACKSIZE_FIQ	0x1000
#endif


#define CONFIG_ENV_IS_IN_RK_STORAGE    1 /* Store ENV in rk storage only */

#define CONFIG_ENV_OFFSET 0
#define CONFIG_ENV_SIZE	        0x200
#define CONFIG_CMD_SAVEENV

//#define CONFIG_SILENT_CONSOLE 1
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

/* rk mtd block size */
#define RK_BLK_SIZE             512

/* rockusb */
#define CONFIG_CMD_ROCKUSB

/* for fastboot */
#define CONFIG_USBD_VENDORID        0x2207
#define CONFIG_USBD_PRODUCTID       0x0006
#define CONFIG_USBD_MANUFACTURER    "Rockchip"
#define CONFIG_USBD_PRODUCT_NAME    "rk30xx"

/* Another macro may also be used or instead used to take care of the case
 * where fastboot is started at boot (to be incorporated) based on key press
 */
#define CONFIG_CMD_FASTBOOT
#define CONFIG_FASTBOOT_LOG
#define CONFIG_FASTBOOT_LOG_SIZE                    (SZ_2M)
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH   (SZ_16M)
//CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE should be at least 2*CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH,
//and larger than our boot/recovery image size.
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE        (CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH << 1)

//for board/rockchip/rk30xx/rkloader.c setup_space.
#define CONFIG_RK_EXTRA_BUFFER_SIZE                 (SZ_4M)

/* Fastboot product name */
#define FASTBOOT_PRODUCT_NAME   "fastboot"

#define CONFIG_RK_UDC
#define CONFIG_USB_DEVICE

/*
 * Hardware drivers
 */
#define CONFIG_RK_GPIO

/* SPI */
//#define CONFIG_RK_SPI

/* uart config */
#define	CONFIG_RK_UART
#define CONFIG_RKUSB2UART_FORCE
#define CONFIG_UART_NUM 	UART_CH2

/* i2c */
#define CONFIG_RK_I2C
#ifdef CONFIG_RK_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED 100000
#endif

#define CONFIG_USB_DWC_HCD
//#define CONFIG_USB_EHCI
//#define CONFIG_USB_EHCI_RK
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE

/* PL330 DMA */
//#define CONFIG_PL330_DMA //enable pl330 dma


/* LCDC console */
#define CONFIG_LCD

#ifdef CONFIG_LCD
#define CONFIG_RK_PWM
#define CONFIG_RK_FB
#define CONFIG_LCD_LOGO
//#define CONFIG_CMD_CHARGE_ANIM
#define CONFIG_LCD_BMP_RLE8
#define CONFIG_CMD_BMP

//#define CONFIG_COMPRESS_LOGO_RLE8// CONFIG_COMPRESS_LOGO_RLE16

#define CONFIG_BMP_16BPP
#define CONFIG_SYS_WHITE_ON_BLACK
#define LCD_BPP			LCD_COLOR16

#define CONFIG_LCD_MAX_WIDTH	4096
#define CONFIG_LCD_MAX_HEIGHT	2048


//#define CONFIG_RK3066SDK
//#define CONFIG_RK3188SDK
#define CONFIG_RK3288SDK

#ifdef CONFIG_RK_FB
#if  (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
#define CONFIG_RK_3066_FB
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)
#define CONFIG_RK_3188_FB
//#define CONFIG_VCC_LCDC_1_8   //vcc lcdc switch to 1.8v
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
#define CONFIG_RK_3168_FB
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
#define CONFIG_RK_3026_FB
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
#define CONFIG_RK_3288_FB
#endif

/*rk616 config*/
//#define CONFIG_RK616
#ifdef CONFIG_RK616
#define CONFIG_RK616_LVDS //lvds or mipi
#define CONFIG_RK616_LCD_CHN 0
#endif /*CONFIG_RK616*/ 
#endif /*CONFIG_RK_FB*/
#endif


/********************************** charger and pmic driver ********************************/
//#define CONFIG_POWER_RICOH619
//#define CONFIG_POWER_RK_SAMPLE
#define CONFIG_POWER_RK808

/********************************** battery driver ********************************/
//#define CONFIG_BATTERY_BQ27541
//#define CONFIG_BATTERY_RICOH619
#define CONFIG_BATTERY_RK_SAMPLE  //battery driver


#if 0
#undef CONFIG_GZIP
#undef CONFIG_ZLIB
#undef CONFIG_CMD_BOOTD
#undef CONFIG_CMD_ITEST
#undef CONFIG_SOURCE
#undef CONFIG_CMD_SOURCE
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_CONSOLE
#undef CONFIG_CMD_CACHE
#undef CONFIG_CMD_MEMORY
#undef CONFIG_PARTITIONS
#undef CONFIG_CMD_ECHO
#undef CONFIG_CMD_REGINFO
#undef CONFIG_CMDLINE_EDITING
#endif
#define CONFIG_LMB
#define CONFIG_OF_LIBFDT
#define CONFIG_OF_CONTROL
#define CONFIG_SYS_BOOT_RAMDISK_HIGH

#define CONFIG_RESOURCE_PARTITION

#define CONFIG_QUICK_CHECKSUM


//allow to flash loader when check sign failed. should undef this in release version.
#define CONFIG_ENABLE_ERASEKEY


#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_READ
#define CONFIG_MMC
//#define CONFIG_PARTITIONS
#define CONFIG_CMD_MMC
#define CONFIG_SYS_MMC_ENV_DEV 0

#define CONFIG_CMD_BOOTI
#define CONFIG_KERNEL_LOAD_ADDR 		SZ_32M

#endif /* __CONFIG_H */

