/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File    :   driver.h
Desc    :   包含驱动层的所有驱动头文件，
            驱动.c文件中应只包含这一个头文件。
Author  :   rk28 driver
Date    :   2008-11-18
Notes   :   
$Log: drivers.h,v $
Revision 1.1  2011/03/29 09:20:48  Administrator
1、支持三星8GB EMMC
2、支持emmc boot 1和boot 2都写入IDBLOCK数据

Revision 1.1  2011/01/07 03:28:04  Administrator
*** empty log message ***

********************************************************************/
#ifndef _EMMC_CONFIG_H
#define _EMMC_CONFIG_H

#include    "../../armlinux/config.h"

#define     DRIVER_ONLY

#include    "hw_SDConfig.h"
#include    "hw_SDCommon.h"
#include    "hwapi_SDOsAdapt.h"
#include    "hwapi_SDController.h"
#include    "hw_SDController.h"
#include    "hw_SDPlatAdapt.h"
#include    "hw_SD.h"
#include    "hw_MMC.h"
#include    "hw_SDM.h"
#include    "hwapi_SDM.h"

/********************************************************************
**                 api head files                                   *
********************************************************************/
//#include    "api_drivers.h"

#endif

