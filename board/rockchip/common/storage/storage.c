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

#define     IN_STORAGE
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>

#include "../config.h"
#include "storage.h"

static MEM_FUN_T nullFunOp =
{
	-1,
	BOOT_FROM_NULL,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

#ifdef RK_FLASH_BOOT_EN
static MEM_FUN_T NandFunOp =
{
	0,
	BOOT_FROM_FLASH,
	0,
	lMemApiInit,
	LMemApiReadId,
	LMemApiReadPba,
	LMemApiWritePba,
	LMemApiReadLba,
	LMemApiWriteLba,
	LMemApiErase,
	LMemApiFlashInfo,
	NULL,
	LMemApiLowFormat,
	NULL,
	NULL,
	LMemApiGetCapacity,
	LMemApiSysDataLoad,
	LMemApiSysDataStore,
};
#endif

#ifdef RK_SDMMC_BOOT_EN
static MEM_FUN_T emmcFunOp =
{
	2,
	BOOT_FROM_EMMC,
	0,
	SdmmcInit,
	SdmmcReadID,
	SdmmcBootReadPBA,
	SdmmcBootWritePBA,
	SdmmcBootReadLBA,
	SdmmcBootWriteLBA,
	SdmmcBootErase,
	SdmmcReadFlashInfo,
	SdmmcCheckIdBlock,
	NULL,
	NULL,
	NULL,
	SdmmcGetCapacity,
	SdmmcSysDataLoad,
	SdmmcSysDataStore,
};
#endif

#ifdef RK_SDCARD_BOOT_EN
MEM_FUN_T sd0FunOp = 
{
	0,
	BOOT_FROM_SD0,
	0,
	SdmmcInit,
	SdmmcReadID,
	SdmmcBootReadPBA,
	SdmmcBootWritePBA,
	SdmmcBootReadLBA,
	SdmmcBootWriteLBA,
	SdmmcBootErase,
	SdmmcReadFlashInfo,
	NULL,
	NULL,
	NULL,
	NULL,
	SdmmcGetCapacity,
	SdmmcSysDataLoad,
	SdmmcSysDataStore,
};
#endif

static MEM_FUN_T *memFunTab[] = 
{
#ifdef RK_SDCARD_BOOT_EN
	&sd0FunOp,
#endif

#ifdef RK_SDMMC_BOOT_EN
	&emmcFunOp,
#endif

#ifdef RK_FLASH_BOOT_EN
	&NandFunOp,
#endif
};
#define MAX_MEM_DEV	(sizeof(memFunTab)/sizeof(MEM_FUN_T *))


int32 StorageInit(void)
{
	uint32 memdev;

	memset((uint8*)&g_FlashInfo, 0, sizeof(g_FlashInfo));
	for(memdev=0; memdev<MAX_MEM_DEV; memdev++)
	{
		gpMemFun = memFunTab[memdev];
		if(memFunTab[memdev]->Init(memFunTab[memdev]->id) == 0)
		{
			memFunTab[memdev]->Valid = 1;
			StorageReadFlashInfo((uint8*)&g_FlashInfo);
			return 0;
		}
	}

	/* if all media init error, usding null function */
	gpMemFun = &nullFunOp;

	return -1;
}

void FW_ReIntForUpdate(void)
{
	gpMemFun->Valid = 0;
	if(gpMemFun->IntForUpdate)
	{
		gpMemFun->IntForUpdate();
	}
	gpMemFun->Valid = 1;
}

static int FWLowFormatEn = 0;
void FW_SorageLowFormatEn(int en)
{
	FWLowFormatEn = en;
} 

void FW_SorageLowFormat(void)
{
	if(FWLowFormatEn)
	{
		if(gpMemFun->LowFormat)
		{
			gpMemFun->Valid = 0;
			gpMemFun->LowFormat();
			gpMemFun->Valid = 1;
		}
		FWLowFormatEn = 0;
	}
} 

uint32 FW_StorageGetValid(void)
{
	 return gpMemFun->Valid;
}

/***************************************************************************
函数描述:获取 FTL 正常擦除的 block号
入口参数:
出口参数:
调用函数:
***************************************************************************/
uint32 FW_GetCurEraseBlock(void)
{
	uint32 data = 1;

	if(gpMemFun->GetCurEraseBlock)
	{
		data = gpMemFun->GetCurEraseBlock();
	}

	return data;
}

/***************************************************************************
函数描述:获取 FTL 总block数
入口参数:
出口参数:
调用函数:
***************************************************************************/
uint32 FW_GetTotleBlk(void)
{
	uint32 totle = 1;

	if(gpMemFun->GetTotleBlk)
	{
		totle = gpMemFun->GetTotleBlk();
	}

	return totle;
}

int StorageReadPba(uint32 PBA, void *pbuf, uint16 nSec)
{
	int ret = FTL_ERROR;

	if(gpMemFun->ReadPba)
	{
		ret = gpMemFun->ReadPba(gpMemFun->id, PBA, pbuf, nSec);
	}

	return ret;
}

int StorageWritePba(uint32 PBA, void *pbuf, uint16 nSec)
{
	int ret = FTL_ERROR;

	if(gpMemFun->WritePba)
	{
		ret = gpMemFun->WritePba(gpMemFun->id, PBA, pbuf, nSec);
	}

	return ret;
}

int StorageReadLba(uint32 LBA, void *pbuf, uint16 nSec)
{
	int ret = FTL_ERROR;

	if(gpMemFun->ReadLba)
	{
		ret = gpMemFun->ReadLba(gpMemFun->id, LBA, pbuf, nSec);
	}

	return ret;
}

int StorageWriteLba(uint32 LBA, void *pbuf, uint16 nSec, uint16 mode)
{
	int ret = FTL_ERROR;

	if(gpMemFun->WriteLba)
	{
		ret = gpMemFun->WriteLba(gpMemFun->id, LBA, pbuf, nSec, mode);
	}

	return ret;
}

uint32 StorageGetCapacity(void)
{
	uint32 ret = FTL_ERROR;

	if(gpMemFun->GetCapacity)
	{
		ret = gpMemFun->GetCapacity(gpMemFun->id);
	}

	return ret;
}

uint32 StorageSysDataLoad(uint32 Index, void *Buf)
{
	uint32 ret = FTL_ERROR;

	memset(Buf, 0, 512);
	if(gpMemFun->SysDataLoad)
	{
		ret = gpMemFun->SysDataLoad(gpMemFun->id, Index, Buf);
	}

	return ret;
}

uint32 StorageSysDataStore(uint32 Index, void *Buf)
{
	uint32 ret = FTL_ERROR;

	if(gpMemFun->SysDataStore)
	{
		ret = gpMemFun->SysDataStore(gpMemFun->id, Index, Buf);
	}

	return ret;
}


#define UBOOT_SYS_DATA_OFFSET 64
uint32 StorageUbootDataLoad(uint32 Index, void *Buf)
{
	return StorageSysDataLoad(Index + UBOOT_SYS_DATA_OFFSET, Buf);
}

uint32 StorageUbootDataStore(uint32 Index, void *Buf)
{
	return StorageSysDataStore(Index + UBOOT_SYS_DATA_OFFSET, Buf);
}

uint32 UsbStorageSysDataLoad(uint32 offset, uint32 len, uint32 *Buf)
{
	uint32 ret = FTL_ERROR;
	uint32 i;

	for(i=0;i<len;i++)
	{
		StorageSysDataLoad(offset + 2 + i, Buf);
		if(offset + i < 2) 
		{
			Buf[0] = 0x444E4556;
			Buf[1] = 504;
		}
		Buf += 128;
	}

	return ret;
}

uint32 UsbStorageSysDataStore(uint32 offset, uint32 len, uint32 *Buf)
{
	uint32 ret = FTL_ERROR;
	uint32 i;

	for(i=0; i<len; i++)
	{
		Buf[0] = 0x444E4556;
		Buf[1] = 504;
		StorageSysDataStore(offset + 2 + i, Buf);
		Buf += 128;
	}

	return ret;
}

int StorageReadFlashInfo(void *pbuf)
{
	int ret = FTL_ERROR;

	if(gpMemFun->ReadInfo)
	{
		gpMemFun->ReadInfo(pbuf);
		ret = FTL_OK;
	}

	return ret;
}

int StorageEraseBlock(uint32 blkIndex, uint32 nblk, uint8 mod)
{
	int Status = FTL_OK;

	if(gpMemFun->Erase)
	{
		Status = gpMemFun->Erase(0, blkIndex, nblk, mod);
	}

	return Status;
}

uint16 StorageGetBootMedia(void)
{
   	return gpMemFun->flag;
}

uint32 StorageGetSDFwOffset(void)
{
	uint32 offset = 0;

	if(gpMemFun->flag != BOOT_FROM_FLASH)
	{
#if defined(RK_SDMMC_BOOT_EN) || defined(RK_SDCARD_BOOT_EN)
		offset = SdmmcGetFwOffset(gpMemFun->id);
#endif
	}

	return offset;
}

uint32 StorageGetSDSysOffset(void)
{
	uint32 offset = 0;

	if(gpMemFun->flag != BOOT_FROM_FLASH)
	{
#if defined(RK_SDMMC_BOOT_EN) || defined(RK_SDCARD_BOOT_EN)
		offset = SdmmcGetSysOffset(gpMemFun->id);
#endif
	}

	return offset;
}

int StorageReadId(void *pbuf)
{
	int ret = FTL_ERROR;

	if(gpMemFun->ReadId)
	{
		gpMemFun->ReadId(0, pbuf);
		ret = FTL_OK;
	}

	return ret;
}

#ifdef RK_SDCARD_BOOT_EN
uint32 StorageSDCardUpdateMode(void)
{
	if ((StorageGetBootMedia() == BOOT_FROM_SD0) && (SdmmcGetSDCardBootMode() == SDMMC_SDCARD_UPDATE))
	{
		return 1;
	}

	return 0;
}
#endif

