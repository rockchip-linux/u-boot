/****************************************************************
//    CopyRight(C) 2008 by Rock-Chip Fuzhou
//      All Rights Reserved
//文件名:hw_SDPlatAdapt.h
//描述:RK28 SD/MMC driver Platform adaptation head file
//作者:hcy
//创建日期:2008-11-08
//更改记录:
//当前版本:1.00
$Log: hw_SDPlatAdapt.h,v $
Revision 1.1  2011/03/29 09:20:48  Administrator
1、支持三星8GB EMMC
2、支持emmc boot 1和boot 2都写入IDBLOCK数据

Revision 1.1  2011/01/18 07:20:31  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 11:55:41  Administrator
*** empty log message ***

Revision 1.1.1.1  2009/08/18 06:43:27  Administrator
no message

Revision 1.1.1.1  2009/08/14 08:02:01  Administrator
no message

****************************************************************/
#ifdef DRIVERS_SDMMC

#ifndef _SDPAM_H_
#define _SDPAM_H_

#define SDMMC_NO_PLATFORM    0    //指示当前是不是只用SDMMC的代码，其他模块的代码都不用，1:只有SDMMC模块，0:用到其他模块

#if SDMMC_NO_PLATFORM
#define SDPAM_MAX_AHB_FREQ   166   //MHz
#else
#define SDPAM_MAX_AHB_FREQ   200//FREQ_HCLK_MAX
#endif

/****************************************************************/
//对外函数声明
/****************************************************************/
void   SDPAM_FlushCache(void *adr, uint32 size);
void   SDPAM_CleanCache(void *adr, uint32 size);
uint32 SDPAM_GetAHBFreq(void);
void   SDPAM_SDCClkEnable(SDMMC_PORT_E nSDCPort, bool enable);
void   SDPAM_SDCReset(SDMMC_PORT_E nSDCPort);
void   SDPAM_SetMmcClkDiv(SDMMC_PORT_E nSDCPort, uint32 div);
bool   SDPAM_DMAStart(SDMMC_PORT_E nSDCPort, uint32 dstAddr, uint32 srcAddr, uint32 size, bool rw, pFunc CallBack);
bool   SDPAM_DMAStop(SDMMC_PORT_E nSDCPort, bool rw);
bool   SDPAM_INTCRegISR(SDMMC_PORT_E nSDCPort, pFunc Routine);
bool   SDPAM_INTCEnableIRQ(SDMMC_PORT_E nSDCPort);
bool   SDPAM_IOMUX_SetSDPort(SDMMC_PORT_E nSDCPort, HOST_BUS_WIDTH_E width);
bool   SDPAM_IOMUX_PwrEnGPIO(SDMMC_PORT_E nSDCPort);
bool   SDPAM_IOMUX_DetGPIO(SDMMC_PORT_E nSDCPort);
void   SDPAM_ControlPower(SDMMC_PORT_E nSDCPort, bool enable);
bool   SDPAM_IsCardPresence(SDMMC_PORT_E nSDCPort);

int32 eMMC_changemode(uint8 mode);


#endif //end of #ifndef _SDPAM_H_

#endif //end of #ifdef DRIVERS_SDMMC
