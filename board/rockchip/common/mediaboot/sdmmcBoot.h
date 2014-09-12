/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:  api_flash.h
Author:     XUESHAN LIN
Created:    1st Dec 2008
Modified:
Revision:   1.00
********************************************************************************
********************************************************************************/
#ifndef _SDMMC_BOOT_H
#define _SDMMC_BOOT_H

#ifdef RK_SDCARD_BOOT_EN
#define     SDMMC_SDCARD_BOOT		(1)	// SD Card boot
#define     SDMMC_SDCARD_UPDATE		(2)	// SD card update
#endif
extern uint32 SdmmcInit(uint32 sdcId);
extern void SdmmcReadID(uint8 chip, void *buf);

extern void SdmmcReadFlashInfo(void *buf);
extern uint32  SdmmcBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern uint32  SdmmcBootReadPBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
extern uint32  SdmmcBootWritePBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
extern void SdmmcCheckIdBlock(void);
extern uint32 SdmmcBootReadLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec);
extern uint32 SdmmcBootWriteLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec, uint16 mode);
extern uint32 SdmmcGetCapacity(uint8 ChipSel);

extern uint32 SdmmcSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 SdmmcSysDataStore(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 EmmcSetBootPart(uint32 ChipSel, uint32 BootPart, uint32 AccessPart);
extern uint32 SdmmcGetFwOffset(uint8 ChipSel);
extern uint32 SdmmcGetSysOffset(uint8 ChipSel);
extern void _SDC0IST(void);
extern void _SDC2IST(void);
#ifdef RK_SDCARD_BOOT_EN
extern uint32 SdmmcGetSDCardBootMode(void);
extern uint32 BootFromSdCard(uint8 ChipSel);
#endif

#endif /* _SDMMC_BOOT_H */

