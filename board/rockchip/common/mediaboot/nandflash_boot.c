/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "../config.h"
#include "nandflash_boot.h"


static pLOADER_MEM_API_T gp_loader_api = NULL;

void LMemApiReadId(uint8 chipSel, void *pbuf)
{
	if(gp_loader_api->ReadId) {
		gp_loader_api->ReadId(chipSel, pbuf);
	}
}

void LMemApiFlashInfo(void *pbuf)
{
	if(gp_loader_api->ReadInfo) {
		gp_loader_api->ReadInfo(pbuf);
	}
}

uint32 LMemApiLowFormat(void)
{
	int ret = FTL_ERROR;

	if(gp_loader_api->LowFormat) {
		gp_loader_api->LowFormat();
		ret = FTL_OK;
	}

	return ret;
}

uint32 LMemApiErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod)
{
	int ret = FTL_ERROR;

	if(gp_loader_api->Erase) {
		gp_loader_api->Erase(ChipSel, blkIndex, nblk, mod);
		ret = FTL_OK;
	}

	return ret;
}

uint32 LMemApiReadPba(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec)
{
	int ret = FTL_ERROR;

	if(gp_loader_api->ReadPba) {
		ret = gp_loader_api->ReadPba(0, PBA, pbuf, nSec);
	}

	return ret;
}

uint32 LMemApiWritePba(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec)
{
	int ret = FTL_ERROR;

	if(gp_loader_api->WritePba) {
		ret = gp_loader_api->WritePba(0, PBA, pbuf, nSec);
	}

	return ret;
}

uint32 LMemApiReadLba(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec)
{
	int ret = FTL_ERROR;

	if(gp_loader_api->ReadLba) {
		ret = gp_loader_api->ReadLba(ChipSel, LBA , nSec, pbuf);
	}
	return ret;
}

uint32 LMemApiWriteLba(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec, uint32 mode)
{
	int ret = FTL_ERROR;

	if(gp_loader_api->WriteLba) {
		ret = gp_loader_api->WriteLba(ChipSel, LBA, nSec, pbuf);
	}

	return ret;
}

uint32 FtlDeInit(void)
{
	uint32 ret = FTL_ERROR;

	if(gp_loader_api->ftl_deinit) {
		ret = gp_loader_api->ftl_deinit();
	}

	return ret;
}

uint32 FlashDeInit(void)
{
	uint32 ret = FTL_ERROR;

	if(gp_loader_api->flash_deinit) {
		ret = gp_loader_api->flash_deinit();
	}

	return ret;
}

uint32 LMemApiGetCapacity(uint8 ChipSel)
{
	uint32 ret = FTL_ERROR;

	if(gp_loader_api->GetCapacity) {
		ret = gp_loader_api->GetCapacity(gpMemFun->id);
	}

	return ret;
}

uint32 LMemApiSysDataLoad(uint8 ChipSel, uint32 Index, void *Buf)
{
	uint32 ret = FTL_ERROR;

	ftl_memset(Buf, 0, 512);
	if(gp_loader_api->SysDataLoad) {
		ret = gp_loader_api->SysDataLoad(gpMemFun->id, Index, Buf);
	}

	return ret;
}

uint32 LMemApiSysDataStore(uint8 ChipSel, uint32 Index, void *Buf)
{
	uint32 ret = FTL_ERROR;

	if(gp_loader_api->SysDataStore) {
		ret = gp_loader_api->SysDataStore(gpMemFun->id, Index, Buf);
	}

	return ret;
}

uint32 LMemApiEraseData(uint8 cs, uint32 lba, uint32 n_sec)
{
	uint32 ret = FTL_ERROR;

	if (gp_loader_api->erase_data)
		ret = gp_loader_api->erase_data(gpMemFun->id, lba, n_sec);

	return ret;
}

#define RKNANDC_MAX_FREQ	(150 * MHZ)
uint32 lMemApiInit(uint32 BaseAddr)
{
	debug("Try to init Nand Flash.\n");

	gp_loader_api = (pLOADER_MEM_API_T)((unsigned long)*((uint32 *)CONFIG_RKNAND_API_ADDR)); // get api table
	if((gp_loader_api->tag & 0xFFFF0000) == 0x4e460000) {
		// nand and emmc support
		if((gp_loader_api->id == 1) || (gp_loader_api->id == 2)) {
			rkclk_set_nandc_freq_from_gpll(0, RKNANDC_MAX_FREQ);
			return 0;
		} else {
			return -1;
		}
	} else {
		return -1;
		//error
	}
}

