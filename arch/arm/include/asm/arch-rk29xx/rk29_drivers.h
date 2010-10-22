/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File    :   driver.h
Desc    :   包含驱动层的所有驱动头文件，
            驱动.c文件中应只包含这一个头文件。
Author  :   rk28 driver
Date    :   2008-11-18
Notes   :   

********************************************************************/
#ifndef _RK29_DRIVERS_H
#define _RK29_DRIVERS_H

/********************************************************************
以下定是是模块开关使能, 有定义使能相应模块
********************************************************************/

//#define     DRIVERS_TIMER       //定义定时器模块使能
//#define     DRIVERS_SCU         //定义SCU模块使能, 主要是时钟域和电源域的管理
#define     DRIVERS_UART        //定义串口模块使能, 包括红外和MODEM模式
//#define     DRIVERS_GPIO        //定义GPIO模块使能
//#define     DRIVERS_SPI         //定义SPI模块使能, 包括主从模式
//#define     DRIVERS_I2C         //定义I2C模块使能, 包括主从模式
//#define     DRIVERS_GRF         //定义通用寄存器文件(General Register File)模块使能, 主要是IOMUX和IO配置
//#define     DRIVERS_ADC        // 定义ADC 模块使能
/********************************************************************
**                 common head files                                *
********************************************************************/
#include    "rk29_debug.h"
#include    "rk29_typedef.h"
#include    "rk29_memmap.h"
#include    "rk29_drivers_delay.h"

/********************************************************************
**                  drivers head files                      *
********************************************************************/
//#include    "rk28_timer.h"
//#include    "rk28_grf.h"
//#include    "rk28_scu.h"
//#include    "rk28_spi.h"
//#include    "rk28_i2c.h"
//#include    "rk28_gpio.h"
#include    "rk29_uart.h"
//#include    "rk29_api_scu.h"
//#include    "rk28_adc.h"

#endif   /* _RK29_DRIVERS_H */
