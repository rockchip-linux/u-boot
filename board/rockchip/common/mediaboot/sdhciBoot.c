/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "../config.h"
#include "sdhciBoot.h"

#include "../sdhci/mmc.h"
#include "../sdhci/sdhci.h"

#define SDHCI_EMMC_DEV_ID		0
#define EMMC_BOOT_PART_SIZE		1024
#define SD_CARD_BOOT_PART_OFFSET	64
#define SD_CARD_FW_PART_OFFSET		8192
#define SD_CARD_SYS_PART_OFFSET		8064


static uint32 Sdhci_Data[(1024 * 8 * 4 / 4)] __attribute__((aligned(ARCH_DMA_MINALIGN)));
static uint32 UserCapSize = 0;

int32 sdhci_block_read(uint32 blk_start, uint32 blk_cnt, void *pbuf)
{
	uint32 n;

	n = block_mmc_read(SDHCI_EMMC_DEV_ID, blk_start, blk_cnt, pbuf);

	return (n == blk_cnt) ? 0 : -1;
}

int32 sdhci_block_write(uint32 blk_start, uint32 blk_cnt, void *pbuf)
{
	uint32 n;

	n = block_mmc_write(SDHCI_EMMC_DEV_ID, blk_start, blk_cnt, pbuf);

	return (n == blk_cnt) ? 0 : -1;
}


uint32 SdhciInit(uint32 ChipSel)
{
	if (0 != mmc_init()) {
		printf("SdhciInit%d Error!\n", ChipSel);
		return -1;
	}

	/* Total block size */
	UserCapSize = mmc_get_capacity();

	/* Read id blk data */
	block_mmc_read(SDHCI_EMMC_DEV_ID, SD_CARD_BOOT_PART_OFFSET, 4, gIdDataBuf);

	debug("Total block = 0x%08x, FwPartOffset = 0x%08x\n", UserCapSize, (uint32)SD_CARD_BOOT_PART_OFFSET);

	return 0;
}


uint32 SdhciBootWritePBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec)
{
	uint32 i;
	uint16 len;
	uint32 pageSizeLimit;
	uint32 *pDataBuf = pbuf;

	if ((PBA + nSec) >= EMMC_BOOT_PART_SIZE * 5)
		return 0;

	PBA = PBA + SD_CARD_BOOT_PART_OFFSET;
	pageSizeLimit = 32;
	for (len = 0; len < nSec; len += pageSizeLimit) {
		for (i = 0; i < (MIN(nSec, pageSizeLimit)); i++)
			memcpy(Sdhci_Data + i * 128, pDataBuf + (len + i) * 132, 512);
		if (sdhci_block_write(PBA + len, (MIN(nSec, pageSizeLimit)), Sdhci_Data) != 0)
			return -1;
	}

	return 0;
}

uint32 SdhciBootReadPBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec)
{
	uint32 i;
	uint16 len;
	uint32 pageSizeLimit;
	uint32 *pDataBuf = pbuf;

	if ((PBA + nSec) >= EMMC_BOOT_PART_SIZE * 5)
		return 0;
	PBA = PBA + SD_CARD_BOOT_PART_OFFSET;

	pageSizeLimit = 32;
	for (len = 0; len < nSec; len += pageSizeLimit) {
		if (sdhci_block_read(PBA + len, (MIN(nSec, pageSizeLimit)), Sdhci_Data) != 0)
			return -1;
		for (i = 0; i < (MIN(nSec, pageSizeLimit)); i++)
			memcpy(pDataBuf + (len + i) * 132, Sdhci_Data + (i) * 128, 512);
	}

	return 0;
}


uint32 SdhciBootWriteLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec, uint32 mode)
{
	return sdhci_block_write(LBA + SD_CARD_FW_PART_OFFSET, nSec, pbuf);
}


uint32 SdhciBootReadLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec)
{
	return sdhci_block_read(LBA + SD_CARD_FW_PART_OFFSET, nSec, pbuf);
}


void SdhciReadID(uint8 ChipSel, void *buf)
{
	uint8 *pbuf = buf;

	pbuf[0] = 'E';
	pbuf[1] = 'M';
	pbuf[2] = 'M';
	pbuf[3] = 'C';
	pbuf[4] = ' ';
}

void SdhciReadFlashInfo(void *buf)
{
	pFLASH_INFO pInfo = (pFLASH_INFO)buf;

	pInfo->BlockSize = EMMC_BOOT_PART_SIZE;
	pInfo->ECCBits = 0;
	pInfo->FlashSize = UserCapSize;
	pInfo->PageSize = 4;
	pInfo->AccessTime = 40;
	pInfo->ManufacturerName = 0;
	pInfo->FlashMask = 0;
	if (pInfo->FlashSize)
		pInfo->FlashMask = 1;
}

uint32 SdhciGetCapacity(uint8 ChipSel)
{
	return UserCapSize - SD_CARD_FW_PART_OFFSET;
}

uint32 SdhciSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf)
{
	return sdhci_block_read(Index + SD_CARD_SYS_PART_OFFSET, 1, Buf);
}

uint32 SdhciSysDataStore(uint8 ChipSel, uint32 Index, void *Buf)
{
	return sdhci_block_write(Index + SD_CARD_SYS_PART_OFFSET, 1, Buf);
}

uint32 SdhciGetFwOffset(uint8 ChipSel)
{
	return SD_CARD_FW_PART_OFFSET;
}

uint32 SdhciGetSysOffset(uint8 ChipSel)
{
	return SD_CARD_SYS_PART_OFFSET;
}


uint32 SdhciBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod)
{
	return 0;
}

