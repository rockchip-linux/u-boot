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

/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_ROCKCHIP		1	/* in a ROCKCHIP core */
#define CONFIG_RK30XX		1	/* which is in a RK30XX Family */

#define CONFIG_RK3066		1
#define CONFIG_RK3188		2

#define CONFIG_RKCHIPTYPE	CONFIG_RK3188

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
#define CONFIG_SYS_GBL_DATA_SIZE	128	/* size in bytes for */
						/* initial data */

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL2			1	/* use SERIAL2 */
#define CONFIG_BAUDRATE			115200

/*
 * Hardware drivers
 */
/* used for MMU */
#define RAM_PHY_START			0x60000000
#define RAM_PHY_END			0x68000000

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

/*
 * memtest setup
 */
/* DRAM Base */
#define CONFIG_SYS_SDRAM_BASE		0x60000000		/* Physical start address of SDRAM. */
#define CONFIG_SYS_INIT_SP_ADDR 	(0x60400000 - 0x8000)	

#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x1000000)

/* Default load address */
#define CONFIG_SYS_LOAD_ADDR		CONFIG_SYS_SDRAM_BASE

//#define CONFIG_SYS_ICACHE_OFF
//#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_SYS_L2CACHE_OFF
#define CONFIG_SYS_ARM_CACHE_WRITETHROUGH
/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(256 << 10)	/* 256 KiB */
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
#define PHYS_SDRAM_1		CONFIG_SYS_SDRAM_BASE	/* OneDRAM Bank #0 */
#define PHYS_SDRAM_1_SIZE	(128 << 20)		/* 128 MB in Bank #0 */
#define CONFIG_SYS_SDRAM_SIZE PHYS_SDRAM_1_SIZE

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }


/*
 * Monitor config
 */
#define CONFIG_SYS_MONITOR_BASE		0x60000000	/* Physical start address of boot monitor code */
							/* be same as the text base address CONFIG_SYS_TEXT_BASE */
#define CONFIG_SYS_MONITOR_LEN		(256 << 10)	/* 256 KiB */



#define CONFIG_ENV_IS_IN_RK_EMMC	1		/* Store ENV in emmc only */
#define CONFIG_ENV_ADDR		CONFIG_SYS_MONITOR_BASE

/* sys data(blk 8064), 0-2 was used in boot.c, so we use blk 3.*/
#define CONFIG_ENV_OFFSET       (3 << 9)

#define CONFIG_ENV_SIZE	        0x200
#undef  CONFIG_CMD_SAVEENV
#define CONFIG_CMD_SAVEENV      1

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
#define CONFIG_FASTBOOT_TRANSFER_BUFFER     0x68000000 //128M
//TODO: mod addr of buffer.
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE    (SZ_512M + SZ_128M)
/* Fastboot product name */
#define FASTBOOT_PRODUCT_NAME   "fastboot"

#ifdef CONFIG_CMD_FASTBOOT
#define CONFIG_RK_UDC
#define CONFIG_USB_DEVICE

#endif //CONFIG_CMD_FASTBOOT

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
#define CONFIG_FB_ADDR \
    (CONFIG_FASTBOOT_TRANSFER_BUFFER + CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE)
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#ifdef CONFIG_RK_FB
#if  (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
#define CONFIG_RK_3066_FB
#else
#define CONFIG_RK_3188_FB
#define CONFIG_VCC_LCDC_1_8   //vcc lcdc switch to 1.8v
#endif
#endif
#define CONFIG_RK_I2C
#define CONFIG_HARD_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED 100000
#define CONFIG_SYS_I2C_SLAVE 0x32


#define         CHIP_RK3066     0
#define         CHIP_RK3066B    1
#define         CHIP_RK3168     2
#define         CHIP_RK3188     3
#define         CHIP_RK3188B    4

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

#define CONFIG_RK_I2C
#define CONFIG_I2C_MULTI_BUS

#endif /* __CONFIG_H */
