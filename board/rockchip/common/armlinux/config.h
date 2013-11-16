/********************************************************************************
*********************************************************************************
		COPYRIGHT (c)   2001-2012 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    config.h
Author:		    YIFENG ZHAO
Created:        2012-02-07
Modified:       ZYF
Revision:		1.00
********************************************************************************
********************************************************************************/
#ifndef _CONFIG_H
#define _CONFIG_H

//#define FPGA_EMU

//平台配置
#define     RK28XX   0x0
#define     RK2828   0x2
#define     RK29XX   0x10
#define     RK292X   0x12
#define     RK30XX   0x20
#define     RK31XX   0x30
#define     RK32XX   0x40
#define     RK_ALL   0xFF

#include    "../platform/rk30/paltConfig.h" 

//#define     LINUX_LOADER
#define     SECURE_BOOT_ENABLE
#define     SECURE_BOOT_ENABLE_ALWAY
#define     SECURE_BOOT_LOCK
//#define     ERASE_DRM_KEY_EN
//#define   SECURE_BOOT_TEST
#define     MALLOC_DISABLE
//#define   INSTANT_BOOT_EN
#define     LOAD_OEM_DATA
//#define     RK_LOADER_FOR_FT
#define     DRIVERS_UART
#define     DRIVERS_NAND   
#define 	RK28_FPGA		0
#define     RK_SDMMC_BOOT_EN
//#define     L2CACHE_ENABLE
//define		DRIVERS_MMU
#define __packed __attribute__((packed))
#define __align(x) __attribute__ ((aligned(x)))

//模块配置
#ifdef RK_SPI_BOOT_EN
#define DRIVERS_SPI
#endif
#ifdef RK_SDMMC_BOOT_EN
#define DRIVERS_SDMMC
#endif
#ifdef RK_FLASH_BOOT_EN 
#define DRIVERS_FLASH
#endif

//系统配置
#define     MAX_COMBINATION_KEY     6

//库头文件
//#include    <stdio.h>
//#include    <string.h>
//#include    <stdlib.h>
//#include    <stdarg.h>

//平台无关头文件
#include    "../common/typedef.h"
#include    "../common/storage.h"
#include    "../common/debug.h"

//平台相关头文件
#include    "../platform/paltform.h"		//标准输入/输出

//系统相关头文件
#include    "parameter.h"


#if 0
#ifdef  DRIVERS_FLASH
#include    "FlashBoot.h"
#include 	"flash.h"	//FLASH头文件
#include 	"ftl.h"	//FTL头文件
#include    "LoaderFlash.h"
#include    "FTL_OSDepend.h"
#endif
#endif

#ifdef  DRIVERS_SDMMC
#include    "../common/emmc/sdmmcBoot.h"
#endif

#ifdef  DRIVERS_SPI
#include    "../common/spi/SpiFlash.h"
#include    "../common/spi/SpiBoot.h"
#endif

#ifdef CONFIG_PL330_DMA
#include    <api_pl330.h>
#endif


#define	BOOT_ONLY           //定义只生成中间件引导代码, 不定义是完整可测试的文件系统
#define DEBUG				// 定义DEBUG模式，将打印信息输出到串口

#define USE_RECOVER		// cmy: 禁止Recover功能(自动修复kernel/boot/recovery)
#define USE_RECOVER_IMG

//#define DRIVERS_USB_APP

#ifndef BOOT_ONLY
#define	IN_SYSTEM           //嵌入到系统软件时定义
#define	OS_FILE             //定义OS下的文件系统
#endif


#include 			"../common/dma.h"
#include			"Mainloop.h"	//测试头文件
#include            "../common/rockusb/dwc_otg_regs.h"
#include 		    "../common/rockusb/rockusb.h"  //固件升级头文件
#include            "../platform/rk30/gpio_oper.h"
#include            "bootloader.h"
#include			"parameter.h"	// Loader参数文件
#include			"../common/rk_dbg.h"		// debug file
#include            "../common/A8MMU/hw_mm.h" // MMU
#include            "../common/A8MMU/hwapi_Cache.h" // MMU


// by cmy
#define PARAMETER_NUM			8			// parameter文件的备份个数
#define PARAMETER_OFFSET		1024			// 每个parameter的偏移量
#define SYS_LOADER_REBOOT_FLAG   0x5242C300  //高24是TAG,低8位是标记
#define SYS_KERNRL_REBOOT_FLAG   0xC3524200  //高24是TAG,低8位是标记

enum {
    BOOT_NORMAL=                  0,
    BOOT_LOADER,     /* enter loader rockusb mode */
    BOOT_MASKROM,    /* enter maskrom rockusb mode*/
    BOOT_RECOVER,    /* enter recover */
    BOOT_NORECOVER,  /* do not enter recover */
    BOOT_WINCE,      /* FOR OTHER SYSTEM */
    BOOT_WIPEDATA,   /* enter recover and wipe data. */
    BOOT_WIPEALL,    /* enter recover and wipe all data. */
    BOOT_CHECKIMG,   /* check firmware img with backup part(in loader mode)*/
    BOOT_FASTBOOT,   
    BOOT_SECUREBOOT_DISABLE,  
    BOOT_MAX         /* MAX VALID BOOT TYPE.*/
};

uint32 RkldTimerGetTick( void );
void loader_tag_set_timer( __u32 t0 , __u32 t1);
extern	void UsbIsr(void);

#ifdef DEBUG_FLASH
#define     PRINTF  printf
#else
#define     PRINTF(...)
#endif

#endif

/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/

