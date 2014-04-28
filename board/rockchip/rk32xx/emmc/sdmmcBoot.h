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

extern uint32 SdmmcInit(uint32 sdcId);
extern void SdmmcReadID(uint8 chip, void *buf);

extern void SdmmcReadFlashInfo(void *buf);
extern uint32  SdmmcBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern uint32  SdmmcBootReadPBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern uint32  SdmmcBootWritePBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern void SdmmcCheckIdBlock(void);
extern uint32 SdmmcBootReadLBA(uint8 ChipSel,uint32 LBA , void *pbuf, uint16 nSec );
extern uint32 SdmmcBootWriteLBA(uint8 ChipSel,uint32 LBA, void *pbuf ,uint16 nSec  , uint16 mode);
extern uint32 SdmmcGetCapacity(uint8 ChipSel);

extern uint32 SdmmcSysDataLoad(uint8 ChipSel,uint32 Index,void *Buf);
extern uint32 SdmmcSysDataStore(uint8 ChipSel,uint32 Index,void *Buf);
extern uint32 EmmcSetBootPart(uint32 ChipSel,uint32 BootPart,uint32 AccessPart);
extern uint32 SdmmcGetFwOffset(uint8 ChipSel);
extern uint32 SdmmcGetSysOffset(uint8 ChipSel);
#endif

