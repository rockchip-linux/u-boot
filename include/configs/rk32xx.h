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


/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_ROCKCHIP		1	/* in a ROCKCHIP core */


#include <asm/arch/cpu.h>		/* get chip and board defs */

#define CONFIG_RKCHIPTYPE	CONFIG_RK3288


/* for rk common fold */
#define HAVE_VENDOR_COMMON_LIB	y


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


/*
 * cache config
 */
//#define CONFIG_SYS_ICACHE_OFF
//#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_SYS_L2CACHE_OFF
#define CONFIG_SYS_ARM_CACHE_WRITETHROUGH


/* irq config */
#define CONFIG_USE_IRQ

/* enable imprecise aborts check, default disable */
//#define CONFIG_IMPRECISE_ABORTS_CHECK


/*
 * Enabling relocation of u-boot by default
 * Relocation can be skipped if u-boot is copied to the TEXT_BASE
 */
#undef CONFIG_SKIP_RELOCATE_UBOOT	/* to a proper address, init done */


/*
 * select serial console configuration
 */
#define CONFIG_BAUDRATE			115200
/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }


/* input clock of PLL: has 24MHz input clock at rk30xx */
#define CONFIG_SYS_CLK_FREQ_CRYSTAL	24000000
#define CONFIG_SYS_CLK_FREQ		CONFIG_SYS_CLK_FREQ_CRYSTAL
#define CONFIG_SYS_HZ			1000	/* decrementer freq: 1 ms ticks */


/*
 * Supported U-boot commands
 */
/* Declare no flash (NOR/SPI) */
#define CONFIG_SYS_NO_FLASH		1       /* It should define before config_cmd_default.h */

/* Command definition */
#include <config_cmd_default.h>

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


/* dts support */
#define CONFIG_OF_LIBFDT


/* mod it to enable console commands.	*/
#define CONFIG_BOOTDELAY		0


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

#define CONFIG_EXTRA_ENV_SETTINGS	"verify=n\0initrd_high=0xffffffff=n\0"


/* env config */
#define CONFIG_ENV_IS_IN_RK_STORAGE	1 /* Store ENV in rk storage only */

#define CONFIG_ENV_OFFSET		0
#define CONFIG_ENV_SIZE			0x200
#define CONFIG_CMD_SAVEENV

#define CONFIG_SYS_MMC_ENV_DEV		0

//#define CONFIG_SILENT_CONSOLE		1
#define CONFIG_LCD_CONSOLE_DISABLE	/* lcd not support console putc and puts */
#define CONFIG_SYS_CONSOLE_IS_IN_ENV


/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT		"rk30boot # "
#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		1024	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS		16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_SYS_RAMBOOT
#define CONFIG_SYS_VSNPRINTF


/* rk plat config include */
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	#include "rkplat/rk32plat.h"
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	#include "rkplat/rk30plat.h"

	#undef CONFIG_PM_SUBSYSTEM
	#undef CONFIG_RK_PWM
	#undef CONFIG_UBOOT_CHARGE
	#undef CONFIG_CMD_CHARGE_ANIM
	#undef CONFIG_CHARGE_DEEP_SLEEP
	#undef CONFIG_POWER_FG_ADC
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3126)
	#include "rkplat/rk30plat.h"

	#undef CONFIG_RK_PWM_REMOTE
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3128)
	#include "rkplat/rk30plat.h"

	#undef CONFIG_UBOOT_CHARGE
	#undef CONFIG_CMD_CHARGE_ANIM
	#undef CONFIG_CHARGE_DEEP_SLEEP
	#undef CONFIG_POWER_FG_ADC
#else
	#error: "PLS config chip for rk plat!"
#endif


/*
 * SDRAM Memory Map
 * Even though we use two CS all the memory
 * is mapped to one contiguous block
 */
#define CONFIG_NR_DRAM_BANKS		1

#define PHYS_SDRAM_1            	CONFIG_SYS_SDRAM_BASE /* OneDRAM Bank #0 */
#define PHYS_SDRAM_1_SIZE       	(CONFIG_RAM_PHY_END - CONFIG_RAM_PHY_START) /* 128 MB in Bank #0 */

/* DRAM Base */
#define CONFIG_SYS_SDRAM_BASE		CONFIG_RAM_PHY_START		/* Physical start address of SDRAM. */
#define CONFIG_SYS_SDRAM_SIZE   	PHYS_SDRAM_1_SIZE

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
 * 1MB = 0x100000, 0x100000 = 1024 * 1024
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (1 << 20))


#endif /* __CONFIG_H */

