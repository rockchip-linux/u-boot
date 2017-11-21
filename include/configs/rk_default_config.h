/*
 * Default Configuration for Rockchip Platform
 *
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK_DEFAULT_CONFIG_H
#define __RK_DEFAULT_CONFIG_H


/* Display CPU and Board Info */
#define CONFIG_ARCH_CPU_INIT
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SETUP_MEMORY_TAGS	/* enable memory tag 		*/
#define CONFIG_CMDLINE_TAG		/* enable passing of ATAGs	*/
#define CONFIG_CMDLINE_EDITING		/* add command line history	*/
#define CONFIG_INITRD_TAG		/* Required for ramdisk support */
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_ARCH_EARLY_INIT_R


/* rk board common fold */
#define HAVE_VENDOR_COMMON_LIB		y


/* enable genreal board */
#define CONFIG_SYS_GENERIC_BOARD


/*
 * Supported U-boot commands
 */
/* Declare no flash (NOR/SPI) */
#define CONFIG_SYS_NO_FLASH		1       /* It should define before config_cmd_default.h */

/* Command definition */
#include <config_cmd_default.h>

#define CONFIG_SHA256
#define CONFIG_LIB_RAND

#undef CONFIG_GZIP
#undef CONFIG_ZLIB
#undef CONFIG_SOURCE
#undef CONFIG_PARTITIONS

/* config for cmd_bootm */
#undef CONFIG_BOOTM_NETBSD
#undef CONFIG_BOOTM_RTEMS
#undef CONFIG_BOOTM_OSE
#undef CONFIG_BOOTM_PLAN9
#undef CONFIG_BOOTM_VXWORKS
#undef CONFIG_LYNXKDI
#undef CONFIG_INTEGRITY
#undef CONFIG_CMD_ELF
#undef CONFIG_CMD_BOOTD

/* Disabled commands */
#undef CONFIG_CMD_FPGA          	/* FPGA configuration Support   */
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_XIMG

#undef CONFIG_CMD_ITEST
#undef CONFIG_CMD_SOURCE
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_CONSOLE
#undef CONFIG_CMD_CACHE
#undef CONFIG_CMD_MEMORY
#undef CONFIG_CMD_ECHO
#undef CONFIG_CMD_REGINFO
#undef CONFIG_CMDLINE_EDITING
#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_IMI
#undef CONFIG_CMD_EDITENV
#undef CONFIG_CMD_RUN
#undef CONFIG_CMD_SETGETDCR


/*
 * cache config
 */
#undef CONFIG_SYS_ICACHE_OFF
#undef CONFIG_SYS_DCACHE_OFF
#define CONFIG_SYS_L2CACHE_OFF
#define CONFIG_SYS_ARM_CACHE_WRITETHROUGH


/* irq config, arch64 gic no using CONFIG_USE_IRQ */
#if !defined(CONFIG_ARM64)
	#define CONFIG_USE_IRQ
#endif

/* enable imprecise aborts check, default disable */
#undef CONFIG_IMPRECISE_ABORTS_CHECK


/*
 * Enabling relocation of u-boot by default
 * Relocation can be skipped if u-boot is copied to the TEXT_BASE
 */
#undef CONFIG_SKIP_RELOCATE_UBOOT	/* to a proper address, init done */


/*
 * select serial console configuration
 */
#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			115200
#endif
/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200, 1500000 }


/* input clock of PLL: has 24MHz input clock at rk30xx */
#define CONFIG_SYS_CLK_FREQ_CRYSTAL	24000000
#define CONFIG_SYS_CLK_FREQ		CONFIG_SYS_CLK_FREQ_CRYSTAL
#define CONFIG_SYS_HZ			1000	/* decrementer freq: 1 ms ticks */


/*
 * Environment setup
 */
/* use preboot to detect key press for fastboot */
/* if you want to enable bootdelay, define CONFIG_PREBOOT as:
 * 	#define CONFIG_PREBOOT "setenv bootdelay 3"
 * else just do as:
 *	#define CONFIG_PREBOOT
 */
#define CONFIG_PREBOOT
#define CONFIG_CMD_BOOTRK
#define CONFIG_BOOTCOMMAND		"bootrk"

#ifdef CONFIG_ARM64
#define CONFIG_EXTRA_ENV_SETTINGS	"verify=n\0initrd_high=0xffffffffffffffff=n\0"
#else
#define CONFIG_EXTRA_ENV_SETTINGS	"verify=n\0initrd_high=0xffffffff=n\0"
#endif

/* env config */
#define CONFIG_ENV_IS_IN_RK_STORAGE	1 /* Store ENV in rk storage only */

#define CONFIG_ENV_OFFSET		0
#define CONFIG_ENV_SIZE			0x200
#define CONFIG_CMD_SAVEENV

#undef CONFIG_SILENT_CONSOLE
#define CONFIG_LCD_CONSOLE_DISABLE	/* lcd not support console putc and puts */
#define CONFIG_SYS_CONSOLE_IS_IN_ENV


/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT		"rkboot # "
#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		1024	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS		16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_SYS_RAMBOOT
#define CONFIG_SYS_VSNPRINTF


/*
 *			Uboot memory map
 *
 * CONFIG_SYS_TEXT_BASE is the default address which maskrom loader uboot code.
 * CONFIG_RKNAND_API_ADDR is the address which maskrom loader miniloader code.
 *
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
/* rk kernel load address */
#define CONFIG_KERNEL_LOAD_ADDR 	(CONFIG_RAM_PHY_START + SZ_32M)	/* 32M offset */

/* rk nand api function code address */
#define CONFIG_RKNAND_API_ADDR		(CONFIG_RAM_PHY_START + SZ_32M + SZ_16M) /* 48M offset */

/* rk uboot reserve size */
#define CONFIG_LMB_RESERVE_SIZE		(SZ_32M + SZ_16M + SZ_8M) /* 56M offset */

/* rk ddr information */
#define CONFIG_RK_MAX_DRAM_BANKS	8 /* rk ddr max banks */
#define CONFIG_RKDDR_PARAM_ADDR		(CONFIG_RAM_PHY_START + SZ_32M) /* rk ddr banks address and size */
#define CONFIG_RKTRUST_PARAM_ADDR	(CONFIG_RAM_PHY_START + SZ_32M + SZ_2M) /* rk trust os banks address and size */


/* rk hdmi device information buffer (start: 128M - size: 8K) */
#define CONFIG_RKHDMI_PARAM_ADDR	CONFIG_RAM_PHY_END


/*
 * SDRAM Memory Map
 * Even though we use two CS all the memory
 * is mapped to one contiguous block
 */
#define CONFIG_NR_DRAM_BANKS		1

#define PHYS_SDRAM            		CONFIG_RAM_PHY_START /* OneDRAM Bank #0 */
#define PHYS_SDRAM_SIZE       		(CONFIG_RAM_PHY_END - CONFIG_RAM_PHY_START) /* 128 MB in Bank #0 */

/* DRAM Base */
#define CONFIG_SYS_SDRAM_BASE		CONFIG_RAM_PHY_START		/* Physical start address of SDRAM. */
#define CONFIG_SYS_SDRAM_SIZE   	PHYS_SDRAM_SIZE

/* Default load address */
#define CONFIG_SYS_LOAD_ADDR		CONFIG_SYS_SDRAM_BASE

/* sp addr before relocate. */
#define CONFIG_SYS_INIT_SP_ADDR		CONFIG_RAM_PHY_END


/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#ifdef CONFIG_USE_IRQ
#  define CONFIG_STACKSIZE_IRQ		0x10000
#  define CONFIG_STACKSIZE_FIQ		0x1000
#endif

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + SZ_2M)
/*
* bmp max size
*/
#define CONFIG_MAX_BMP_BLOCKS		(SZ_8M / BLOCK_SIZE)

/* rockchip global buffer. */
#define CONFIG_RK_GLOBAL_BUFFER_SIZE			(SZ_4M)
#define CONFIG_RK_BOOT_BUFFER_SIZE			(SZ_32M + SZ_16M)

/*
 * CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE should be larger than our boot/recovery image size.
 */
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE		CONFIG_RK_BOOT_BUFFER_SIZE
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH	(CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE >> 1)


/*
 * boot mode enable config
 */
#define CONFIG_RK_SDMMC_BOOT_EN
#define CONFIG_RK_SDCARD_BOOT_EN
#undef CONFIG_RK_UMS_BOOT_EN
#ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
#define CONFIG_RK_FLASH_BOOT_EN
#endif /* CONFIG_SECOND_LEVEL_BOOTLOADER */


/*
 * merger bin enable config
 */
#ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
#define CONFIG_MERGER_MINILOADER
#ifdef CONFIG_ARM64
#define CONFIG_MERGER_TRUSTIMAGE
#endif /* CONFIG_ARM64 */
#endif /* CONFIG_SECOND_LEVEL_BOOTLOADER */

/*
 * fdtdec parse enable 64bit format for property 'reg', this compatible 32bit
 */
#define CONFIG_PHYS_64BIT

/*
 * boot image: rk mode and ota mode check
 * rk mode - crc32 check kernel and boot image
 * ota mode - sha check boot image
 */
#undef CONFIG_BOOTRK_RK_IMAGE_CHECK
#undef CONFIG_BOOTRK_OTA_IMAGE_CHECK

/* 
 * allow to flash loader when check sign failed. should undef this in release version.
 */
#undef CONFIG_ENABLE_ERASEKEY


/* rk quick check sum */
#define CONFIG_QUICK_CHECKSUM


/* rk io command tool */
#undef CONFIG_RK_IO_TOOL


/* rockusb */
#define CONFIG_CMD_ROCKUSB


/* fastboot */
#define CONFIG_CMD_FASTBOOT


/* rk mtd block size */
#define RK_BLK_SIZE			512

/* rk mtd block size */
#define CONFIG_MAX_PARTITIONS		32


/* fdt and rk resource support */
#define CONFIG_RESOURCE_PARTITION	/* rk resource parttion */
#define CONFIG_OF_LIBFDT		/* fdt support */
#define CONFIG_OF_FROM_RESOURCE		/* fdt from resource */


#ifndef CONFIG_PRODUCT_BOX
/* rk pm management module */
#define CONFIG_PM_SUBSYSTEM
#endif


/* LCDC console */
#define CONFIG_LCD


#ifdef CONFIG_PRODUCT_BOX
/* rk deviceinfo partition */
#define CONFIG_RK_DEVICEINFO

/* rk pwm remote ctrl */
#define CONFIG_RK_PWM_REMOTE
#endif


/*
 * rockchip Hardware drivers
 */
/* rk serial module */
#define	CONFIG_RK_UART
#ifndef CONFIG_UART_NUM
#define CONFIG_UART_NUM			UART_CH2
#endif
#define CONFIG_RKUART2USB_FORCE		/* uart2usb force */

/* usb device */
#if defined(CONFIG_CMD_ROCKUSB) || defined(CONFIG_CMD_FASTBOOT)
#define CONFIG_USB_DEVICE
#define CONFIG_RK_UDC
#endif

/* rk clock module */
#define CONFIG_RK_CLOCK

/* rk gpio module */
#define CONFIG_RK_GPIO

/* rk iomux module */
#define CONFIG_RK_IOMUX

/* rk i2c module */
#define CONFIG_RK_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED	100000

/* rk spi module */
#undef CONFIG_RK_SPI

/* rk key module */
#define CONFIG_RK_KEY

/* rk pwm module */
#define CONFIG_RK_PWM

/* rk efuse module */
#define CONFIG_RK_EFUSE

/* rk dma config */
#define CONFIG_RK_PL330_DMAC	/* rk pl330 dmac */

/* Ethernet support */
#undef CONFIG_RK_GMAC

/* rk power config */
#define CONFIG_RK_POWER

/* rk display module */
#ifdef CONFIG_LCD

#define CONFIG_RK_FB
#ifndef CONFIG_PRODUCT_BOX
#define CONFIG_RK_PWM_BL
#endif

#ifdef CONFIG_PRODUCT_BOX
#define CONFIG_RK_HDMI
#define CONFIG_RK_TVE
#endif

#define CONFIG_LCD_LOGO
#define CONFIG_LCD_BMP_RLE8
#define CONFIG_CMD_BMP

/* CONFIG_COMPRESS_LOGO_RLE8 or CONFIG_COMPRESS_LOGO_RLE16 */
#undef CONFIG_COMPRESS_LOGO_RLE8
#undef CONFIG_COMPRESS_LOGO_RLE16

#define CONFIG_BMP_16BPP
#define CONFIG_BMP_24BPP
#define CONFIG_BMP_32BPP
#define CONFIG_SYS_WHITE_ON_BLACK
#define LCD_BPP				LCD_COLOR16

#define CONFIG_LCD_MAX_WIDTH		4096
#define CONFIG_LCD_MAX_HEIGHT		2048

/* rk lcd size at the end of ddr address */
#define CONFIG_RK_FB_DDREND

#ifdef CONFIG_RK_FB_DDREND
/* support load bmp files for kernel logo */
#define CONFIG_KERNEL_LOGO

/* rk lcd total size = fb size + kernel logo size */
#define CONFIG_RK_LCD_SIZE		SZ_32M
#define CONFIG_RK_FB_SIZE		SZ_16M
#endif

#define CONFIG_BRIGHTNESS_DIM		64

#undef CONFIG_UBOOT_CHARGE

#endif /* CONFIG_LCD */


#endif /* __RK_DEFAULT_CONFIG_H */
