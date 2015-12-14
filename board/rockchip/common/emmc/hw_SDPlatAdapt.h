/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDPAM_H_
#define _SDPAM_H_


/****************************************************************
			对外函数声明
****************************************************************/
void   SDPAM_FlushCache(void *adr, uint32 size);
void   SDPAM_CleanCache(void *adr, uint32 size);
void   SDPAM_InvalidateCache(void *adr, uint32 size);
uint32 SDPAM_GetAHBFreq(SDMMC_PORT_E nSDCPort);
void   SDPAM_SDCClkEnable(SDMMC_PORT_E nSDCPort, uint32 enable);
void   SDPAM_SDCReset(SDMMC_PORT_E nSDCPort);
void   SDPAM_SetMmcClkDiv(SDMMC_PORT_E nSDCPort, uint32 div);
int32  SDPAM_SetSrcFreq(SDMMC_PORT_E nSDCPort, uint32 freqKHz);
int32 SDPAM_SetTuning(SDMMC_PORT_E nSDCPort, uint32 degree, uint32 DelayNum);
#if EN_SD_DMA
bool SDPAM_DMAInit(SDMMC_PORT_E nSDCPort);
bool SDPAM_DMADeInit(SDMMC_PORT_E nSDCPort);
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


#endif /* end of #ifndef _SDPAM_H_ */

#endif /* end of #ifdef DRIVERS_SDMMC */
