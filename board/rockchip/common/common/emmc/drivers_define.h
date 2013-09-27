/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File    :   drivers_define.h
Desc    :   包含驱动层的所有驱动编译开关头文件，
Author  :   rk28 driver
Date    :   2008-11-18
Notes   :   
$Log: drivers_define.h,v $
Revision 1.1  2011/01/14 10:03:10  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 03:28:04  Administrator
*** empty log message ***

********************************************************************/
#ifndef _DRIVERS_DEFINE_H
#define _DRIVERS_DEFINE_H
#define     DRIVER_ONLY
/********************************************************************
以下定是是模块开关使能, 有定义使能相应模块
********************************************************************/
#define     DRIVERS_GRF         //定义通用寄存器文件(General Register File)模块使能, 主要是IOMUX和IO配置
#define     DRIVERS_SCU         //定义SCU模块使能, 主要是时钟域和电源域的管理
#define     DRIVERS_INTC        //定义中断控制模块使能
#define     DRIVERS_SDRAM       //定义SDRAM模块使能
#define     DRIVERS_TIMER       //定义定时器模块使能
#define     DRIVERS_WDT         //定义看门狗模块使能
#define     DRIVERS_RTC         //定义RTC模块使能
#define     DRIVERS_DMA         //定义DW_DMA模块使能
#define     DRIVERS_SDMMC       //定义SD卡和MMC卡模块使能
#define     DRIVERS_EFUSE       //定义eFUSE模块使能

#define     DRIVERS_UART        //定义串口模块使能, 包括红外和MODEM模式
#define     DRIVERS_I2C         //定义I2C模块使能, 包括主从模式
#define     DRIVERS_I2S         //定义I2S模块使能
#define     DRIVERS_SPDIF
#define     DRIVERS_PWM         //定义PWM模块使能
#define     DRIVERS_ADC         //定义ADC模块使能
#define     DRIVERS_GPIO        //定义GPIO模块使能
#define     DRIVERS_SPI         //定义SPI模块使能, 包括主从模式

#define     DRIVERS_LCD         //定义LCD屏驱动模块使能
#define     DRIVERS_LCDC        //定义LCD控制器模块使能, 与具体的屏无关 
#define     DRIVERS_MMU         //定义MMU模块使能
#define     DRIVERS_CACHE       //定义CACHE模块使能

#define     DRIVERS_VIP         //定义视频输入处理(Video Input Processor)模块使能
#define     DRIVERS_NAND        //定义NAND FLASH模块使能

#define     DRIVERS_USB_APP     //定义USB应用层模块使能, 包括DEVICE和HOST的类驱动

#define     SDMMC_TEST          //


//#define     USBHOSTEN
#endif
