/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian.pop@leadtechdesign.com>
 * Lead Tech Design <www.leadtechdesign.com>
 *
 * Configuation settings for the AT91SAM9261EK board.
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

//#define CONFIG_AT91_LEGACY
/* ARM asynchronous clock */
//#define CONFIG_SYS_AT91_MAIN_CLOCK	18432000	/* 18.432 MHz crystal */



/****************set SDK ARMfreq/AHB/APB******************/
#define CONFIG_SYS_RK28_ARMFreq   266
#define CONFIG_SYS_RK28_hclkDiv   HCLK_DIV2
#define CONFIG_SYS_RK28_pclkDiv	  PCLK_DIV2
/***************************************************************/



/***************************************************************/
#define CONFIG_UART_NUM   UART_CH1
#define CONFIG_ENC28J60_CS   0x00

/***************************************************************/
#define CONFIG_RK28_UART
#define CONFIG_RK28_SPI
#define CONFIG_RK28_GPIO



#define CONFIG_SYS_HZ		1000

#define CONFIG_ARCH_CPU_INIT
#undef CONFIG_USE_IRQ			/* we don't need IRQ/FIQ stuff	*/

#define CONFIG_CMDLINE_TAG	1	/* enable passing of ATAGs	*/
#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_INITRD_TAG	1

#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SKIP_RELOCATE_UBOOT

#define CONFIG_SYS_MAX_FLASH_BANKS		1
#define CONFIG_ENV_IS_NOWHERE		1
#define CONFIG_SYS_NO_FLASH	        1
#define CONFIG_ENV_SIZE                0x4200
#define CONFIG_ENV_OFFSET              0x4200
#define CONFIG_SYS_MAX_FLASH_SECT		256


#define CONFIG_BOOTCOMMAND "tftp 60100000 uImage;bootm 60100000"
//#define CONFIG_ETHADDR	00.aa.bb.cc.dd.ee  //add by oolong
//#define CONFIG_IPADDR   192.168.1.254		//add by oolong
//#define CONFIG_SERVERIP 192.168.1.109		//add by oolong
#define CFG_AUTOLOAD	"tftp 20400000 ulmage9261;tftp 21100000 ramdisk9261.gz;bootm 20400000"
/*
 * Hardware drivers
 */

#define CONFIG_BOOTDELAY	3

/*
 * BOOTP options
 */
//#define CONFIG_BOOTP_BOOTFILESIZE	1
//#define CONFIG_BOOTP_BOOTPATH		1
//#define CONFIG_BOOTP_GATEWAY		1
//#define CONFIG_BOOTP_HOSTNAME		1

/*
 * Command line configuration.
 */
//#include <config_cmd_default.h>
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_IMI
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_SOURCE

//#define CONFIG_CMD_PING		1
//#define CONFIG_CMD_DHCP		1
//#define CONFIG_CMD_NET          1
//#define CONFIG_CMD_NAND		0
//#define CONFIG_CMD_USB		1

/* SDRAM */
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM			0x60000000
#define PHYS_SDRAM_SIZE			0x08000000	/* 64 megs */








/* Ethernet */
//#define CONFIG_NET_MULTI		1
//#define CONFIG_DRIVER_DM9000		1
//#define CONFIG_DM9000_BASE		0x30000000
//#define DM9000_IO			CONFIG_DM9000_BASE
//#define DM9000_DATA			(CONFIG_DM9000_BASE + 4)
//#define CONFIG_DM9000_USE_16BIT		1
//#define CONFIG_DM9000_NO_SROM		1
//#define CONFIG_NET_RETRY_COUNT		20
//#define CONFIG_RESET_PHY_R		1
//#define CONFIG_TFTP_BLOCKSIZE		512

/* USB */
//#define CONFIG_USB_ATMEL
//#define CONFIG_USB_OHCI_NEW		1
//#define CONFIG_DOS_PARTITION		1
//#define CONFIG_SYS_USB_OHCI_CPU_INIT		1
//#define CONFIG_SYS_USB_OHCI_REGS_BASE		0x00500000	/* AT91SAM9261_UHP_BASE */
//#ifdef CONFIG_AT91SAM9G10EK
//#define CONFIG_SYS_USB_OHCI_SLOT_NAME		"at91sam9g10"
//#else
//#define CONFIG_SYS_USB_OHCI_SLOT_NAME		"at91sam9261"
//#endif
//#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	2
//#define CONFIG_USB_STORAGE		1
//#define CONFIG_CMD_FAT			1
//#define CONFIG_USE_IRQ

#define CONFIG_SYS_LOAD_ADDR			0x22000000	/* load address */

#define CONFIG_SYS_MEMTEST_START		PHYS_SDRAM
#define CONFIG_SYS_MEMTEST_END			0x23e00000


/* CONFIG_SYS_USE_NANDFLASH */
/* bootstrap + u-boot + env + linux in dataflash on CS0 */
//#define CONFIG_CMD_SAVEENV
//#define CONFIG_ENV_IS_IN_NAND  		0

//#define CONFIG_ENV_OFFSET_REDUND		0x2a0000		//0x80000
//#define CONFIG_ENV_OFFSET				0x280000		/* Offset of Environment Sector */
//#define CONFIG_ENV_SIZE				0x4000	/* Total Size of Environment Sector, 1 block =512 kB*/
//#define CONFIG_SYS_NAND_BASE          0x100AE000
//#define  	CONFIG_SYS_MAX_NAND_DEVICE  		1
//#define CONFIG_BOOTCOMMAND	"cp.b 0xC0042000 0x22000000 0x210000; bootm"
#if 1
#define CONFIG_BOOTARGS		"root=/dev/ram0  rw "			\
				"mem=256M@0x60000000 "		\
				"console=ttyS1,115200n8 "		\
				"initrd=0x62000000,60M "		\
				"init=/init "					\
				"ramdisk_size=62000"
#else
#define CONFIG_BOOTARGS		"root=/dev/ram0  rw "			\
				"mem=128M@0x60000000  "		\
				"console=ttyS0,115200n8n "		\
				"initrd=0x62000000,24M "		\
				"init=/init "
#endif




#define CONFIG_BAUDRATE		115200
#define CONFIG_SYS_BAUDRATE_TABLE	{115200 , 19200, 38400, 57600, 9600 }

#define CONFIG_SYS_PROMPT		"U-Boot> "
#define CONFIG_SYS_CBSIZE		1024 //256
#define CONFIG_SYS_MAXARGS		16
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
//#define CONFIG_SYS_LONGHELP		1
#define CONFIG_CMDLINE_EDITING	1

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		ROUND(3 * CONFIG_ENV_SIZE + 128*1024, 0x1000)
#define CONFIG_SYS_GBL_DATA_SIZE	128	/* 128 bytes for initial data */

#define CONFIG_STACKSIZE	(32*1024)	/* regular stack */
#define CONFIG_STACKSIZE_IRQ		(512*1024)   // (4 * 1024) /* Unsure if to big or to small*/
#define CONFIG_STACKSIZE_FIQ		(512*1024)   //(4 * 1024) /* Unsure if to big or to small*/

//#ifdef CONFIG_USE_IRQ
//#error CONFIG_USE_IRQ not supported
//#endif

#endif
