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

#define     PALTFORM    RK30XX

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
#define     RK_SDMMC_BOOT_EN


#define __packed	__attribute__((packed))
#define __align(x)	__attribute__ ((aligned(x)))

//模块配置
#ifdef RK_SPI_BOOT_EN
	#define DRIVERS_SPI
#endif

//系统配置
#define     MAX_COMBINATION_KEY     6

//库头文件
#define Assert(cond,msg,num)
#define min(X, Y)				\
	({ typeof(X) __x = (X);			\
		typeof(Y) __y = (Y);		\
		(__x < __y) ? __x : __y; })

#define max(X, Y)				\
	({ typeof(X) __x = (X);			\
		typeof(Y) __y = (Y);		\
		(__x > __y) ? __x : __y; })

#define MIN(x, y)  min(x, y)
#define MAX(x, y)  max(x, y)


#ifdef DEBUG_FLASH
#define	PRINTF  printf
#else
#define	PRINTF(...)
#endif

#define PRINT_I		PRINTF
#define PRINT_D		PRINTF
#define PRINT_E		PRINTF
#define PRINT_W		PRINTF
#define RkPrintf	PRINTF

#include <common.h>
#include <asm/arch/rkplat.h>

//平台无关头文件
#include "storage.h"

//平台相关头文件
#include "chipDepend.h"

//系统相关头文件
#include "parameter.h"



#ifdef  DRIVERS_SPI
#include "../common/spi/SpiFlash.h"
#include "../common/spi/SpiBoot.h"
#endif

#ifdef CONFIG_PL330_DMA
#include <api_pl330.h>
#endif

#define USE_RECOVER		// cmy: 禁止Recover功能(自动修复kernel/boot/recovery)
#define USE_RECOVER_IMG


extern uint32 SecureBootEn;
extern uint32 SecureBootCheckOK;
extern uint32 g_BootRockusb;
extern uint32 SecureBootLock;
extern uint32 SecureBootLock_backup;


// by cmy
#define SYS_LOADER_REBOOT_FLAG   0x5242C300  //高24是TAG,低8位是标记
#define SYS_KERNRL_REBOOT_FLAG   0xC3524200  //高24是TAG,低8位是标记
#define SYS_LOADER_ERR_FLAG      0X1888AAFF

enum {
	BOOT_NORMAL = 0,
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
	BOOT_CHARGING,
	BOOT_MAX         /* MAX VALID BOOT TYPE.*/
};

uint32 RkldTimerGetTick( void );
int ftl_memcmp(void *str1, void *str2, unsigned int count);
void* ftl_memcpy(void* pvTo, const void* pvForm, unsigned int size);
int ftl_memcmp(void *str1, void *str2, unsigned int count);

#endif

/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/

