
#ifndef __CONFIG_H
#define __CONFIG_H
/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_ROCKCHIP		1	/* in a ROCKCHIP core */
#define CONFIG_RK30XX		1	/* which is in a RK30XX Family */

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
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_DCACHE_OFF
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
#define CONFIG_UART_NUM   		UART_CH1

/* input clock of PLL: has 24MHz input clock at rk30xx */
#define CONFIG_SYS_CLK_FREQ_CRYSTAL	24000000
#define CONFIG_SYS_CLK_FREQ		CONFIG_SYS_CLK_FREQ_CRYSTAL
#define CONFIG_SYS_HZ			1000	/* decrementer freq: 1 ms ticks */

/* Declare no flash (NOR/SPI) */
#define CONFIG_SYS_NO_FLASH		1       /* It should define before config_cmd_default.h */

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
#define CONFIG_BOOTDELAY		3	/* autoboot after 3 seconds */
#define CONFIG_ZERO_BOOTDELAY_CHECK

#define CONFIG_BOOTCOMMAND "print;bootm 0x60407FC0"
#define CONFIG_BOOTARGS "console=ttyFIQ0 androidboot.console=ttyFIQ0 init=/init"

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

#define CONFIG_ENV_IS_NOWHERE		1
#define CONFIG_ENV_SIZE			(256 << 10)	/* 256 KiB, 0x40000 */
#define CONFIG_ENV_ADDR			(1 << 20)	/* 1 MB, 0x100000 */


/* MTD Support (mtdparts command, UBI support) */
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS		/* Needed for UBI support. */


#endif /* __CONFIG_H */
