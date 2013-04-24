/*
 * (C) Copyright 2013-2013
 * peter <superpeter.cai@gmail.com>
 *
 * Configuation settings for the rk30xx board.
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

/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_ROCKCHIP		1	/* in a ROCKCHIP core */
#define CONFIG_RK30XX		1	/* which is in a RK30XX Family */
#define HAVE_VENDOR_COMMON_LIB y
/* Get CPU defs */
#include <asm/arch/cpu.h>		/* get chip and board defs */

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
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_RKEMMC
/* Command definition */
#include <config_cmd_default.h>

/* Disabled commands */
#undef CONFIG_CMD_FPGA          /* FPGA configuration Support   */
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_XIMG

/* Enabled commands */
#define CONFIG_CMD_CACHE	/* icache, dcache		 */
#define CONFIG_CMD_REGINFO	/* Register dump		 */
#define CONFIG_CMD_MTDPARTS	/* mtdparts command line support */


/*
 * Environment setup
 */
#define CONFIG_BOOTDELAY    0
/* use preboot to detect key press for fastboot */
#define CONFIG_PREBOOT
#define CONFIG_BOOTCOMMAND "booti"

#define CONFIG_EXTRA_ENV_SETTINGS  "verify=n\0"


/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
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

#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_L2CACHE_OFF

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


#ifndef CONFIG_SYS_RAMBOOT
#  define CONFIG_ENV_IS_IN_FLASH	1
#    define CONFIG_ENV_ADDR		0x280000	/* Offset of Environment Sector */
#    define CONFIG_ENV_SECT_SIZE	(512 << 10)	/* 512 KiB, 0x80000 */
#    define CONFIG_ENV_SIZE		CONFIG_ENV_SECT_SIZE
#else
#define CONFIG_ENV_IS_NOWHERE		1		/* Store ENV in memory only */
#  define CONFIG_ENV_ADDR		CONFIG_SYS_MONITOR_BASE
#  define CONFIG_ENV_SIZE		0x200
#endif /* CONFIG_SYS_RAMBOOT */


/* MTD Support (mtdparts command, UBI support) */
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS		/* Needed for UBI support. */


/* for fastboot */

#define CONFIG_USBD_VENDORID        0x18d1
#define CONFIG_USBD_PRODUCTID       0x2c10
#define CONFIG_USBD_MANUFACTURER    "Rockchip"
#define CONFIG_USBD_PRODUCT_NAME    "rk30xx"

/* Another macro may also be used or instead used to take care of the case
 * where fastboot is started at boot (to be incorporated) based on key press
 */
#define CONFIG_CMD_FASTBOOT
#define CONFIG_FASTBOOT_TRANSFER_BUFFER     (0)
//TODO: mod addr of buffer.
#define CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE    (SZ_512M - SZ_16M)
/* Fastboot product name */
#define FASTBOOT_PRODUCT_NAME   "fastboot"
/* Address of the kernel's ramconsole so we can dump it.  This is
 * used by the 'fastboot oem kmsg' command.  It needs to be done
 * early in fastboot (before large amount of transfer buffer is used,
 * since they overlap).
 */
#define CONFIG_FASTBOOT_RAMCONSOLE_START (0)
//TODO: mod addr to dump kmsg.

#ifdef CONFIG_CMD_FASTBOOT
//just for combile*************
#define CONFIG_SYS_FIFO_BASE 0
#define CONFIG_SYS_USBD_BASE 0
#define CONFIG_SYS_PLUG_BASE 0
#define CONFIG_DW_UDC
#define CONFIG_MUSB_GADGET
#define MUSB_BASE 0
#define CONFIG_USB_OMAP3
#define CONFIG_MUSB_PIO_ONLY
//TODO:need implete low level usb & otg.
//just for combile*************
//
#define CONFIG_USB_DEVICE
#undef CONFIG_CMD_SAVEENV
//TODO:need implete
#define  EP0_MAX_PACKET_SIZE     64
//TODO:move to otg(udc)'s header
#endif //CONFIG_CMD_FASTBOOT

#endif /* __CONFIG_H */
