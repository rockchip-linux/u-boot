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
uint32 SDPAM_GetAHBFreq(SDMMC_PORT_E nSDCPort);
void   SDPAM_SDCClkEnable(SDMMC_PORT_E nSDCPort, uint32 enable);
void   SDPAM_SDCReset(SDMMC_PORT_E nSDCPort);
void   SDPAM_SetMmcClkDiv(SDMMC_PORT_E nSDCPort, uint32 div);
#if EN_SD_DMA
bool SDPAM_DMAInit(SDMMC_PORT_E nSDCPort);
bool SDPAM_DMAStart(SDMMC_PORT_E nSDCPort, uint32 dstAddr, uint32 srcAddr, uint32 size, bool rw, pFunc cb_f);
bool SDPAM_DMAStop(SDMMC_PORT_E nSDCPort, bool rw);
#endif
uint32   SDPAM_INTCRegISR(SDMMC_PORT_E nSDCPort, pFunc Routine);
uint32   SDPAM_INTCEnableIRQ(SDMMC_PORT_E nSDCPort);
uint32   SDPAM_IOMUX_SetSDPort(SDMMC_PORT_E nSDCPort, HOST_BUS_WIDTH_E width);
uint32   SDPAM_IOMUX_PwrEnGPIO(SDMMC_PORT_E nSDCPort);
uint32   SDPAM_IOMUX_DetGPIO(SDMMC_PORT_E nSDCPort);
void   SDPAM_ControlPower(SDMMC_PORT_E nSDCPort, uint32 enable);
uint32   SDPAM_IsCardPresence(SDMMC_PORT_E nSDCPort);

int32 eMMC_changemode(uint8 mode);


#endif //end of #ifndef _SDPAM_H_

#endif //end of #ifdef DRIVERS_SDMMC
