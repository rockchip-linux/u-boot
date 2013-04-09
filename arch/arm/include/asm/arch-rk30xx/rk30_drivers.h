/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File    :   driver.h
Author  :   rk30 driver
Date    :   2008-11-18
Notes   :   

********************************************************************/
#ifndef _RK30_DRIVERS_H
#define _RK30_DRIVERS_H

/********************************************************************
以下定是是模块开关使能, 有定义使能相应模块
********************************************************************/

//#define     DRIVERS_TIMER       //定义定时器模块使能
//#define     DRIVERS_SCU         //定义SCU模块使能, 主要是时钟域和电源域的管理
//#define     DRIVERS_GPIO        //定义GPIO模块使能
//#define     DRIVERS_SPI         //定义SPI模块使能, 包括主从模式
//#define     DRIVERS_I2C         //定义I2C模块使能, 包括主从模式
//#define     DRIVERS_GRF         //定义通用寄存器文件(General Register File)模块使能, 主要是IOMUX和IO配置
//#define     DRIVERS_ADC        // 定义ADC 模块使能
/********************************************************************
**                 common head files                                *
********************************************************************/
#include    "rk30_debug.h"
#include    "rk30_typedef.h"
#include    "rk30_memmap.h"
#include    "rk30_drivers_delay.h"

#endif   /* _RK30_DRIVERS_H */
