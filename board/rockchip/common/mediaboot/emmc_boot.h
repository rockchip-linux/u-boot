/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * Configuation settings for the rk3xxx chip platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _EMMC_BOOT_H
#define _EMMC_BOOT_H


#define EMMC_BOOT_PART_SIZE         1024
#define SD_CARD_BOOT_PART_OFFSET    64
#define SD_CARD_FW_PART_OFFSET      8192
#define SD_CARD_SYS_PART_OFFSET     8064


extern uint32 emmcInit(uint32 unused);
extern void emmcReadID(uint8 ChipSel, void *buf);
extern void emmcReadFlashInfo(void *buf);
extern uint32 emmcBootReadPBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec );
extern uint32 emmcBootWritePBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec );
extern uint32 emmcBootReadLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint16 nSec);
extern uint32 emmcBootWriteLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint16 nSec, uint16 mode);
extern uint32 emmcBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern void emmcCheckIdBlock(void);
extern uint32 emmcGetCapacity(uint8 ChipSel);
extern uint32 emmcSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 emmcSysDataStore(uint8 ChipSel, uint32 Index, void *Buf);

#endif /* _EMMC_BOOT_H */

