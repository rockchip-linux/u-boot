/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _NANDFLASH_BOOT_H
#define _NANDFLASH_BOOT_H


extern uint32 lMemApiInit(uint32 BaseAddr);
extern void LMemApiReadId(uint8 chipSel, void *pbuf);
extern void LMemApiFlashInfo(void *pbuf);
extern uint32 LMemApiReadPba(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
extern uint32 LMemApiWritePba(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
extern uint32 LMemApiReadLba(uint8 ChipSel, uint32 LBA ,void *pbuf, uint32 nSec);
extern uint32 LMemApiWriteLba(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec, uint32 mode);
extern uint32 LMemApiGetCapacity(uint8 ChipSel);
extern uint32 LMemApiSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 LMemApiSysDataStore(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 LMemApiErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern uint32 LMemApiLowFormat(void);
uint32 LMemApiEraseData(uint8 cs, uint32 lba, uint32 n_sec);
extern uint32 FtlDeInit(void);
extern uint32 FlashDeInit(void);

#endif /* _NANDFLASH_BOOT_H */

