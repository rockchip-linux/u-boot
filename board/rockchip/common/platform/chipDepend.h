/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _CHIP_DEPEND_H
#define _CHIP_DEPEND_H

extern void DRVDelayUs(uint32 count);
extern void DRVDelayMs(uint32 count);

extern void CacheFlushDRegion(uint32 adr, uint32 size);
extern void CacheInvalidateDRegion(uint32 adr, uint32 size);

extern void ISetLoaderFlag(uint32 flag);
extern uint32 IReadLoaderFlag(void);

extern uint32 GetMmcCLK(uint32 nSDCPort);
extern void EmmcPowerEn(char En);
extern void SDCReset(uint32 sdmmcId);
extern int SCUSelSDClk(uint32 sdmmcId, uint32 div);
extern int32 SCUSetSDClkFreq(uint32 sdmmcId, uint32 freq);
extern int32 SCUSetTuning(uint32 sdmmcId, uint32 degree, uint32 DelayNum);
extern void sdmmcGpioInit(uint32 ChipSel);
extern void FW_NandDeInit(void);

extern void rkplat_uart2UsbEn(uint32 en);

#endif /* _CHIP_DEPEND_H */
