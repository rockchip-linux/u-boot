/*********************************************************************************
*     Copyright (C),2004-2005,  Fuzhou Rockchip Co.,Ltd.
*         All Rights Reserved
*          V1.00
* FileName :  Hw_define.h
* Author :  lzy
* Description:   包含硬件系统的相关定义和宏定义
* History  :
*   <author>  <time>    <version>    <desc>
*    lzy     07/8/23    1.0    ORG
$Log: hw_define.h,v $
Revision 1.1  2011/01/14 10:03:10  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 03:28:04  Administrator
*** empty log message ***

*********************************************************************************/
#ifndef _HW_DEFINE_H
#define _HW_DEFINE_H
//#include "..\include\bool.h"
/*******************************************/

// 注意: 此处芯片编译开关已经移动到system\include\bool.h

/***********************         chip define              *************************/

#define RK29_FPGA 0
#define RK29_SDKDEMO 1
#define RK2800_SDKDEMO 2
#define BOARDTYPE RK29_SDKDEMO

/***********************************************************************/
#define SDRAM_2x32x4     (1)   // 32M
#define SDRAM_4x32x4     (2)   // 64M
#define SDRAM_8x32x4     (3)   // 128M
#define SDRAM_16x32x4    (4)   // 256M

#define SDRAM            (1)
#define MOBILE_SDRAM     (2)

#define SDRAM_TYPE   SDRAM
#define SDRAM_SIZE   SDRAM_2x32x4
/************************        Codec define            ****************************/
#define WM8750_CODEC     (1)
#define WM8987_CODEC     (2)
#define WM8988_CODEC     (3)
#define WM8758_CODEC     (4)
#define RK1000_CODEC     (5)
#define ES8388_CODEC     (6)


#define CODEC_TYPE       RK1000_CODEC

/**********************          RK1000 define        *****************************/
#if  ((CODEC_TYPE == RK1000_CODEC))
#define RK1000_ENABLE
#endif

/***********************         rtc define              *************************/
#define   INTERNAL_RTC        0
#define   PT7C4337            1
#define   HYM8563             2

#define    RTC_TYPE        HYM8563 

#endif
