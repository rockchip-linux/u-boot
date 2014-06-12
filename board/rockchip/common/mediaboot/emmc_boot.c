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

#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>

#include "../config.h"
#include "emmc_boot.h"

#undef ALIGN
#define ALIGN(x) __attribute__ ((aligned(x)))

//TODO 
static struct mmc *storage_mmc;
static int storage_device = -1;

ALIGN(64)uint32	emmc_SpareBuf[(32*8*4/4)];
ALIGN(64)uint32	emmc_Data[(1024*8*4/4)];


uint32 emmcInit(uint32 unused)
{
	struct mmc *mmc;

	if (storage_device < 0) {
		if (get_mmc_num() > 0)
			storage_device = 0;
		else {
			printf("No MMC device available\n");
			return -1;
		}
	}
	mmc = find_mmc_device(storage_device);
	if (!mmc) {
		printf("no mmc device at slot %x\n", storage_device);
		return 1;
	}
	if (mmc_init(mmc)) {
		printf("MMC init failed\n");
		return -1;
	}
	storage_mmc = mmc;

	return 0;
}


uint32 emmcBootReadPBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec)
{
	uint32 i;
	uint16 len;
	uint8 pageSizeLimit;
	int n;
	uint32 *pDataBuf = pbuf;

	pageSizeLimit = DATA_LEN/128;
	memset(emmc_SpareBuf, 0xFF, SPARE_LEN);
	if(PBA + nSec >= EMMC_BOOT_PART_SIZE*5) 
		PBA &= (EMMC_BOOT_PART_SIZE-1);
	PBA = PBA + SD_CARD_BOOT_PART_OFFSET;

	for (len=0; len<nSec; len+=pageSizeLimit)
	{
		n = storage_mmc->block_dev.block_read(storage_device, PBA,
							(MIN(nSec, pageSizeLimit)), emmc_Data);
		if(n != (MIN(nSec, pageSizeLimit)))
			return -1;
		for (i=0; i<(MIN(nSec,pageSizeLimit)); i++)
		{
			ftl_memcpy(pDataBuf+(len+i)*132, emmc_Data+(i)*128, 512);
			ftl_memcpy(pDataBuf+(len+i)*132+128, emmc_SpareBuf+(i)*4, 16);
		}
	}
	flush_cache((ulong)pbuf, nSec*512);

	return 0;
}


uint32 emmcBootWritePBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec)
{
	uint32 i;
	uint16 len;
	uint8 pageSizeLimit;
	int n;
	uint32 *pDataBuf = pbuf;

	pageSizeLimit = DATA_LEN/128;
	if(PBA + nSec >= EMMC_BOOT_PART_SIZE*5) 
		return 0;
	PBA = PBA + SD_CARD_BOOT_PART_OFFSET;

	for (len=0; len<nSec; len+=pageSizeLimit)
	{
		for (i=0; i<(MIN(nSec, pageSizeLimit)); i++)
		{
			ftl_memcpy(emmc_Data+i*128, pDataBuf+(len+i)*132, 512);
			ftl_memcpy(emmc_SpareBuf+i*4, pDataBuf+(len+i)*132+128, 16);
		}
		n = storage_mmc->block_dev.block_write(storage_device, PBA,
							(MIN(nSec, pageSizeLimit)), emmc_Data);
		if(n != (MIN(nSec, pageSizeLimit)))
			return -1;
	}

	return 0;
}


uint32 emmcBootReadLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint16 nSec)
{
	int n;

	n = storage_mmc->block_dev.block_read(storage_device, LBA + SD_CARD_FW_PART_OFFSET,
						nSec, pbuf);
	flush_cache((ulong)pbuf, nSec*512);
	return (n == nSec) ? 0 : -1;
}


uint32 emmcBootWriteLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint16 nSec, uint16 mode)
{
	int n;

	n = storage_mmc->block_dev.block_write(storage_device, LBA + SD_CARD_FW_PART_OFFSET,
						nSec, pbuf);
	return (n == nSec) ? 0 : -1;
}


uint32 emmcGetCapacity(uint8 ChipSel)
{
	return (storage_mmc->capacity / 512 - SD_CARD_FW_PART_OFFSET);
}


uint32 emmcSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf)
{
	int n;

	memset(Buf, 0, 512);
	n = storage_mmc->block_dev.block_read(storage_device, SD_CARD_SYS_PART_OFFSET+Index,
						1, Buf);
	/* flush cache after read */
	flush_cache((ulong)Buf, 512); /* FIXME */
	return (n == 1) ? 0 : -1;
}


uint32 emmcSysDataStore(uint8 ChipSel, uint32 Index, void *Buf)
{
	int n;

	n = storage_mmc->block_dev.block_write(storage_device, SD_CARD_SYS_PART_OFFSET+Index, 1, Buf);
	return (n == 1) ? 0 : -1;
}


void emmcReadFlashInfo(void *pbuf)
{
	pFLASH_INFO pInfo = (pFLASH_INFO)pbuf;

	pInfo->BlockSize = EMMC_BOOT_PART_SIZE;
	pInfo->ECCBits = 0;
	pInfo->FlashSize =  storage_mmc->capacity/512; 
	pInfo->PageSize = 4;
	pInfo->AccessTime = 40;
	pInfo->ManufacturerName = 0;
	pInfo->FlashMask = 0;
	pInfo->FlashMask = 1;
}


uint32 emmcBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod)
{
	int Status = FTL_OK;
	return Status;
}


void emmcReadID(uint8 ChipSel, void *buf)
{
	uint8 * pbuf = buf;
	pbuf[0] = 'E';
	pbuf[1] = 'M';
	pbuf[2] = 'M';
	pbuf[3] = 'C';
	pbuf[4] = ' ';
}
