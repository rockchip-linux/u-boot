/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _SDHCI_BOOT_H
#define _SDHCI_BOOT_H

extern uint32 SdhciInit(uint32 ChipSel);
extern void SdhciReadID(uint8 ChipSel, void *buf);
extern void SdhciReadFlashInfo(void *buf);
extern uint32 SdhciBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern uint32 SdhciBootReadPBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
extern uint32 SdhciBootWritePBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
extern uint32 SdhciBootReadLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec);
extern uint32 SdhciBootWriteLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec, uint32 mode);
extern uint32 SdhciGetCapacity(uint8 ChipSel);

extern uint32 SdhciSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 SdhciSysDataStore(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 SdhciGetFwOffset(uint8 ChipSel);
extern uint32 SdhciGetSysOffset(uint8 ChipSel);
int32 sdhci_block_write(uint32 blk_start, uint32 blk_cnt, void *pbuf);
int32 sdhci_block_read(uint32 blk_start, uint32 blk_cnt, void *pbuf);

#endif /* _SDHCI_BOOT_H */
