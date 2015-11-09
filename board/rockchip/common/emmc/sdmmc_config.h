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
********************************************************************/
#ifndef _EMMC_CONFIG_H
#define _EMMC_CONFIG_H

#include    "../config.h"

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

#endif

