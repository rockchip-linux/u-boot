/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/


#ifndef __CONFIG_H
#define __CONFIG_H
#include "rkchip.h"
/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_ROCKCHIP		1	/* in a ROCKCHIP core */

#define SECOND_LEVEL_BOOTLOADER

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

#define CONFIG_USE_RK30IRQ
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

//define uboot loader addr.
#ifdef SECOND_LEVEL_BOOTLOADER
//2m offset for packed nand bin.
#define CONFIG_SYS_TEXT_BASE    0x60200000
#define RK_FLASH_BOOT_EN
#else
#define CONFIG_SYS_TEXT_BASE    0x60000000
#define RK_SDMMC_BOOT_EN
#endif

/*
 * Hardware drivers
 */
/* base definition of ram addr & size */
//size should be 2^x.(like 64m/128m/256m/512m...)
#define RAM_PHY_SIZE            0x04000000
#define RAM_PHY_START           0x60000000
#define RAM_PHY_END             (RAM_PHY_START + RAM_PHY_SIZE)

#define CONFIG_RKNAND_API_ADDR  (RAM_PHY_START + 4)

/* uart config */
#define	CONFIG_RK30_UART
#define CONFIG_UART_NUM   		UART_CH2

/* input clock of PLL: has 24MHz input clock at rk30xx */
#define CONFIG_SYS_CLK_FREQ_CRYSTAL	24000000
#define CONFIG_SYS_CLK_FREQ		CONFIG_SYS_CLK_FREQ_CRYSTAL
#define CONFIG_SYS_HZ			1000	/* decrementer freq: 1 ms ticks */

/* Declare no flash (NOR/SPI) */
#define CONFIG_SYS_NO_FLASH		1       /* It should define before config_cmd_default.h */


#define CONFIG_CMD_CACHE	/* icache, dcache		 */
#define CONFIG_CMD_REGINFO	/* Register dump		 */
//#define CONFIG_CMD_MTDPARTS	/* mtdparts command line support */


/*
 * Environment setup
 */
/* use preboot to detect key press for fastboot */
#define CONFIG_PREBOOT
#define CONFIG_BOOTCOMMAND "booti"

#define CONFIG_EXTRA_ENV_SETTINGS  "verify=n\0"


/*
 * Miscellaneous configurable options
 */
#undef CONFIG_SYS_LONGHELP		/* undef to save memory */
#undef CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT	"rk30boot # "
#define CONFIG_SYS_CBSIZE	256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_USE_IRQ
#define CONFIG_SYS_RAMBOOT

/* DRAM Base */
#define CONFIG_SYS_SDRAM_BASE		RAM_PHY_START		/* Physical start address of SDRAM. */

//sp addr before relocate.
#define CONFIG_SYS_INIT_SP_ADDR     RAM_PHY_END

//#define CONFIG_SYS_ICACHE_OFF
//#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_SYS_L2CACHE_OFF
#define CONFIG_SYS_ARM_CACHE_WRITETHROUGH
/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#ifdef CONFIG_USE_IRQ
#  define CONFIG_STACKSIZE_IRQ	0x10000
#  define CONFIG_STACKSIZE_FIQ	0x1000
#endif


/*
 * SDRAM Memory Map
 * Even though we use two CS all the memory
 * is mapped to one contiguous block
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1            CONFIG_SYS_SDRAM_BASE /* OneDRAM Bank #0 */
#define PHYS_SDRAM_1_SIZE       (RAM_PHY_END - RAM_PHY_START) /* 128 MB in Bank #0 */
#define CONFIG_SYS_SDRAM_SIZE   PHYS_SDRAM_1_SIZE

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

#define CONFIG_ENV_IS_IN_RK_STORAGE    1 /* Store ENV in rk storage only */

#define CONFIG_ENV_OFFSET 0

#define CONFIG_ENV_SIZE	        0x200
#define CONFIG_CMD_SAVEENV

#define RK_BLK_SIZE             512

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

#ifdef CONFIG_CMD_FASTBOOT
#define CONFIG_RK_UDC
#define CONFIG_USB_DEVICE

#endif //CONFIG_CMD_FASTBOOT

/* PL330 DMA */
#define CONFIG_PL330_DMA //enable pl330 dma

#ifdef CONFIG_PL330_DMA   
#define SDMMC_USE_DMA  //for emmc use dma trans
#endif
/* SPI */
//#define CONFIG_RK_SPI

/* LCDC console */
#define CONFIG_LCD
#define CONFIG_RK_FB
#define CONFIG_LCD_LOGO

#define CONFIG_COMPRESS_LOGO_RLE8// CONFIG_COMPRESS_LOGO_RLE16

#define CONFIG_BMP_16BPP
#define CONFIG_SYS_WHITE_ON_BLACK
#define LCD_BPP			LCD_COLOR16
//#define CONFIG_RK3066SDK
//#define CONFIG_RK3188SDK

#define CONFIG_SYS_CONSOLE_IS_IN_ENV
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
#endif

/*rk616 config*/
//#define CONFIG_RK616
#ifdef CONFIG_RK616
#define CONFIG_RK616_LVDS //lvds or mipi
#define CONFIG_RK616_LCD_CHN 0
#endif /*CONFIG_RK616*/ 
#endif /*CONFIG_RK_FB*/
//#define CONFIG_RK_I2C
#ifdef CONFIG_RK_I2C
#define CONFIG_HARD_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED 100000
#define CONFIG_SYS_I2C_SLAVE 0x32
#endif
//#define CONFIG_BQ27541_I2C_ADDR  0x55

/********************************** charger and pmic driver ********************************/
//#define CONFIG_POWER_RICOH619
#define CONFIG_POWER_RK_SAMPLE

/********************************** battery driver ********************************/
//#define CONFIG_BATTERY_BQ27541
//#define CONFIG_BATTERY_RICOH619
#define CONFIG_BATTERY_RK_SAMPLE  //battery driver


#undef CONFIG_GZIP
#undef CONFIG_ZLIB
#undef CONFIG_CMD_BOOTM
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

#define CONFIG_CMD_BMP
//#define CONFIG_CMD_CHARGE_ANIM
#define CONFIG_LCD_BMP_RLE8

#define CONFIG_QUICK_CHECKSUM

#define CONFIG_RK_I2C
#define CONFIG_I2C_MULTI_BUS

//allow to flash loader when check sign failed. should undef this in release version.
#define CONFIG_ENABLE_ERASEKEY

#endif /* __CONFIG_H */
