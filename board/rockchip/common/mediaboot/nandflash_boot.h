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

#ifndef _NANDFLASH_BOOT_H
#define _NANDFLASH_BOOT_H


extern uint32 lMemApiInit(uint32 BaseAddr);
extern void LMemApiReadId(uint8 chipSel, void *pbuf);
extern void LMemApiFlashInfo(void *pbuf);
extern uint32 LMemApiReadPba(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec);
extern uint32 LMemApiWritePba(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec);
extern uint32 LMemApiReadLba(uint8 ChipSel, uint32 LBA ,void *pbuf, uint16 nSec);
extern uint32 LMemApiWriteLba(uint8 ChipSel, uint32 LBA, void *pbuf, uint16 nSec, uint16 mode);
extern uint32 LMemApiGetCapacity(uint8 ChipSel);
extern uint32 LMemApiSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 LMemApiSysDataStore(uint8 ChipSel, uint32 Index, void *Buf);
extern uint32 LMemApiErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern uint32 LMemApiLowFormat(void);

extern uint32 FtlDeInit(void);
extern uint32 FlashDeInit(void);

#endif /* _NANDFLASH_BOOT_H */

