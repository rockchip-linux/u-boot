/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define     IN_STORAGE
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>

#include "../config.h"
#include "storage.h"
#include "../nvme/nvme.h"

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
	NULL, /* LMemApiLowFormat,*/
	NULL,
	NULL,
	LMemApiGetCapacity,
	LMemApiSysDataLoad,
	LMemApiSysDataStore,
	LMemApiEraseData,
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
	SdmmcBootEraseData,
};
#endif

#ifdef RK_SDHCI_BOOT_EN
static MEM_FUN_T emmcFunOp =
{
	3,
	BOOT_FROM_EMMC,
	0,
	SdhciInit,
	SdhciReadID,
	SdhciBootReadPBA,
	SdhciBootWritePBA,
	SdhciBootReadLBA,
	SdhciBootWriteLBA,
	SdhciBootErase,
	SdhciReadFlashInfo,
	NULL,
	NULL,
	NULL,
	NULL,
	SdhciGetCapacity,
	SdhciSysDataLoad,
	SdhciSysDataStore,
	NULL,
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
	NULL,
};
#endif

#ifdef CONFIG_RK_NVME_BOOT_EN
static MEM_FUN_T nvmeFunOp =
{
	0,
	BOOT_FROM_NVME,
	0,
	nvme_init,
	NULL,
	NULL,
	NULL,
	nvme_read,
	nvme_write,
	NULL,
	nvme_read_flash_info,
	NULL,
	NULL,
	NULL,
	NULL,
	nvme_get_capacity,
	NULL,
	NULL,
	NULL,
};

#endif

#ifdef RK_UMS_BOOT_EN
MEM_FUN_T UMSFunOp =
{
	0,
	BOOT_FROM_UMS,
	0,
	UMSInit,
	UMSReadID,
	UMSReadPBA,
	NULL,
	UMSReadLBA,
	NULL,
	NULL,
	UMSReadFlashInfo,
	NULL,
	NULL,
	NULL,
	NULL,
	UMSGetCapacity,
	UMSSysDataLoad,
	NULL,
	NULL,
};
#endif

static MEM_FUN_T *memFunTab[] = 
{
#ifdef RK_UMS_BOOT_EN
	&UMSFunOp,
#endif

#ifdef RK_SDCARD_BOOT_EN
	&sd0FunOp,
#endif

#if defined(RK_SDMMC_BOOT_EN) || defined(RK_SDHCI_BOOT_EN)
	&emmcFunOp,
#endif

#ifdef RK_FLASH_BOOT_EN
	&NandFunOp,
#endif

#ifdef CONFIG_RK_NVME_BOOT_EN
	&nvmeFunOp,
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
			vendor_storage_init();
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
		if(gpMemFun->LowFormat && !SecureBootLock)
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

int StorageReadPba(uint32 PBA, void *pbuf, uint32 nSec)
{
	int ret = FTL_ERROR;

	if(gpMemFun->ReadPba)
	{
		ret = gpMemFun->ReadPba(gpMemFun->id, PBA, pbuf, nSec);
	}

	return ret;
}

int StorageWritePba(uint32 PBA, void *pbuf, uint32 nSec)
{
	int ret = FTL_ERROR;

	if(gpMemFun->WritePba)
	{
		ret = gpMemFun->WritePba(gpMemFun->id, PBA, pbuf, nSec);
	}

	return ret;
}

int StorageReadLba(uint32 LBA, void *pbuf, uint32 nSec)
{
	int ret = FTL_ERROR;

	if(gpMemFun->ReadLba)
	{
		ret = gpMemFun->ReadLba(gpMemFun->id, LBA, pbuf, nSec);
	}

	return ret;
}

int StorageWriteLba(uint32 LBA, void *pbuf, uint32 nSec, uint16 mode)
{
	int ret = FTL_ERROR;

	if(gpMemFun->WriteLba)
	{
		ret = gpMemFun->WriteLba(gpMemFun->id, LBA, pbuf, nSec, mode);
	}

	return ret;
}

int StorageEraseData(uint32 lba, uint32 n_sec)
{
	int ret = FTL_ERROR;

	if (gpMemFun->erase_data)
		ret = gpMemFun->erase_data(gpMemFun->id, lba, n_sec);

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
uint32 StorageUbootSysDataLoad(uint32 Index, void *Buf)
{
	return StorageSysDataLoad(Index + UBOOT_SYS_DATA_OFFSET, Buf);
}

uint32 StorageUbootSysDataStore(uint32 Index, void *Buf)
{
	return StorageSysDataStore(Index + UBOOT_SYS_DATA_OFFSET, Buf);
}

uint32 StorageVendorSysDataLoad(uint32 offset, uint32 len, uint32 *Buf)
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

uint32 StorageVendorSysDataStore(uint32 offset, uint32 len, uint32 *Buf)
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

	if(gpMemFun->Erase && !SecureBootLock)
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
bool StorageSDCardUpdateMode(void)
{
	if ((StorageGetBootMedia() == BOOT_FROM_SD0) && (SdmmcGetSDCardBootMode() == SDMMC_SDCARD_UPDATE))
		return true;

	return false;
}

bool StorageSDCardBootMode(void)
{
	if ((StorageGetBootMedia() == BOOT_FROM_SD0) && (SdmmcGetSDCardBootMode() == SDMMC_SDCARD_BOOT))
		return true;

	return false;
}
#endif

#ifdef RK_UMS_BOOT_EN
bool StorageUMSUpdateMode(void)
{
	if ((StorageGetBootMedia() == BOOT_FROM_UMS) && (UMSGetBootMode() == UMS_UPDATE))
		return true;

	return false;
}

bool StorageUMSBootMode(void)
{
	if ((StorageGetBootMedia() == BOOT_FROM_UMS) && (UMSGetBootMode() == UMS_BOOT))
		return true;

	return false;
}
#endif

static struct vendor_info g_vendor __attribute__((aligned(ARCH_DMA_MINALIGN)));

static int vendor_ops(u8 *buffer, u32 addr, u32 n_sec, int write)
{
	int ret = -1;
	uint16 media = StorageGetBootMedia();
#if defined(RK_SDHCI_BOOT_EN)
	if (media == BOOT_FROM_EMMC) {
		if (write)
			ret = sdhci_block_write(addr + EMMC_VENDOR_PART_START,
						n_sec, buffer);
		else
			ret = sdhci_block_read(addr + EMMC_VENDOR_PART_START,
						n_sec, buffer);
	}
#endif

#if defined(RK_SDMMC_BOOT_EN)
	if (media == BOOT_FROM_EMMC) {
		if (write)
			ret = SDM_Write(gpMemFun->id,
					addr + EMMC_VENDOR_PART_START,
					n_sec, buffer);
		else
			ret = SDM_Read(gpMemFun->id,
				       addr + EMMC_VENDOR_PART_START,
				       n_sec, buffer);
	}
#endif

#if defined(RK_FLASH_BOOT_EN)
	if (media == BOOT_FROM_FLASH) {
		if (write)
			ret = gpMemFun->WriteLba(0x10,
					addr + NAND_VENDOR_PART_START,
					buffer, n_sec, 0);
		else
			ret = gpMemFun->ReadLba(0x10,
					addr + NAND_VENDOR_PART_START,
					buffer, n_sec);
	}
#endif
	return ret;
}

int vendor_storage_init(void)
{
	u32 i, max_ver, max_index, vendor_part_num;
	u8 *p_buf;

	vendor_part_num = VENDOR_PART_NUM;
	#if defined(RK_FLASH_BOOT_EN)
	if (StorageGetBootMedia() == BOOT_FROM_FLASH)
		vendor_part_num = NAND_VENDOR_PART_NUM;
	#endif

	max_ver = 0;
	max_index = 0;
	for (i = 0; i < vendor_part_num; i++) {
		/* read first 512 bytes */
		p_buf = (u8 *)&g_vendor;
		if (vendor_ops(p_buf, VENDOR_PART_SIZE * i, 1, 0))
			return -1;
		/* read last 512 bytes */
		p_buf += (VENDOR_PART_SIZE - 1) * 512;
		if (vendor_ops(p_buf, VENDOR_PART_SIZE * (i + 1) - 1, 1, 0))
			return -1;

		if (g_vendor.tag == VENDOR_TAG &&
		    g_vendor.version2 == g_vendor.version) {
			if (max_ver < g_vendor.version) {
				max_index = i;
				max_ver = g_vendor.version;
			}
		}
	}
	if (max_ver) {
		if (vendor_ops((u8 *)&g_vendor, VENDOR_PART_SIZE * max_index,
				VENDOR_PART_SIZE, 0))
			return -1;
	} else {
		memset(&g_vendor, 0, sizeof(g_vendor));
		g_vendor.version = 1;
		g_vendor.tag = VENDOR_TAG;
		g_vendor.version2 = g_vendor.version;
		g_vendor.free_offset = 0;
		g_vendor.free_size = sizeof(g_vendor.data);
	}
	return 0;
}

int vendor_storage_read(u32 id, void *pbuf, u32 size)
{
	u32 i;

	for (i = 0; i < g_vendor.item_num; i++) {
		if (g_vendor.item[i].id == id) {
			if (size > g_vendor.item[i].size)
				size = g_vendor.item[i].size;
			memcpy(pbuf,
			       &g_vendor.data[g_vendor.item[i].offset],
			       size);
			return size;
		}
	}
	return (-1);
}

int vendor_storage_write(u32 id, void *pbuf, u32 size)
{
	u32 i, next_index, algin_size, vendor_part_num;
	struct vendor_item *item;

	vendor_part_num = VENDOR_PART_NUM;
	#if defined(RK_FLASH_BOOT_EN)
	if (StorageGetBootMedia() == BOOT_FROM_FLASH)
		vendor_part_num = NAND_VENDOR_PART_NUM;
	#endif

	next_index = g_vendor.next_index;
	algin_size = (size + 0x3F) & (~0x3F); /* algin to 64 bytes*/

	for (i = 0; i < g_vendor.item_num; i++) {
		if (g_vendor.item[i].id == id) {
			if (size > algin_size)
				return -1;
			memcpy(&g_vendor.data[g_vendor.item[i].offset],
			       pbuf,
			       size);
			g_vendor.item[i].size = size;
			g_vendor.version++;
			g_vendor.version2 = g_vendor.version;
			g_vendor.next_index++;
			if (g_vendor.next_index >= vendor_part_num)
				g_vendor.next_index = 0;
			vendor_ops((u8 *)&g_vendor, VENDOR_PART_SIZE * next_index,
					VENDOR_PART_SIZE, 1);
			return 0;
		}
	}

	if (g_vendor.free_size >= algin_size) {
		item = &g_vendor.item[g_vendor.item_num];
		item->id = id;
		item->offset = g_vendor.free_offset;
		item->size = size;
		g_vendor.free_offset += algin_size;
		g_vendor.free_size -= algin_size;
		memcpy(&g_vendor.data[item->offset], pbuf, size);
		g_vendor.item_num++;
		g_vendor.version++;
		g_vendor.version2 = g_vendor.version;
		g_vendor.next_index++;
		if (g_vendor.next_index >= vendor_part_num)
			g_vendor.next_index = 0;
		vendor_ops((u8 *)&g_vendor, VENDOR_PART_SIZE * next_index,
				VENDOR_PART_SIZE, 1);
		return 0;
	}
	return(-1);
}
