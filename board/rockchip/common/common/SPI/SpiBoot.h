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
#ifndef _SPI_BOOT_H
#define _SPI_BOOT_H



extern void SpiReadFlashInfo(void *buf);
extern uint32 SpiBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern uint32 SpiBootReadPBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern uint32 SpiBootWritePBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern uint32 SpiBootReadLBA(uint8 ChipSel,uint32 LBA ,void *pbuf, uint16 nSec);
extern uint32 SpiBootWriteLBA(uint8 ChipSel, uint32 LBA, void *pbuf , uint16 nSec  ,uint16 mode);
#endif

