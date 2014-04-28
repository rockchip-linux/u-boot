/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:  flash.C
Author:     XUESHAN LIN
Created:    1st Dec 2008
Modified:
Revision:   1.00
********************************************************************************
********************************************************************************/
#define     IN_STORAGE
#include    "config.h"
#include    "storage.h"
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
//TODO 
extern void FtlReIntForUpdate(void);
extern uint32 FTLLowFormat(void);
struct mmc *storage_mmc;
static int storage_device = -1;
#define EMMC_BOOT_PART_SIZE         1024
#define SD_CARD_BOOT_PART_OFFSET    64
#define SD_CARD_FW_PART_OFFSET      8192
#define SD_CARD_SYS_PART_OFFSET     8064
#undef ALIGN
#define ALIGN(x) __attribute__ ((aligned(x)))
ALIGN(64)uint32 		 emmc_SpareBuf[(32*8*4/4)];
ALIGN(64)uint32		  emmc_Data[(1024*8*4/4)];


int32 StorageInit(void)
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
	StorageReadFlashInfo((uint8*)&g_FlashInfo);
}
void FW_ReIntForUpdate(void)
{
	gpMemFun->Valid = 1;

}

int	FWLowFormatEn = 0;
void FW_SorageLowFormatEn(int en)
{
	FWLowFormatEn = en;
} 

void FW_SorageLowFormat(void)
{
} 

uint32 FW_StorageGetValid(void)
{
	return 1;
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
	return 0;
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
	return totle;
}

int StorageReadPba(uint32 PBA , void *pbuf, uint16 nSec )
{
   	 int ret = FTL_ERROR;
	uint32 i;
	uint16 len;
	uint8 pageSizeRaw;
	uint8 pageSizeLimit;
	uint32 BlockOffset;
	uint16 PageOffset;
	uint16 PageAlignOffset;
	uint16 idblk_flag = 0;
	uint16 read_len;
	int n;
	uint32 *pDataBuf = pbuf;
	pageSizeLimit = DATA_LEN/128;
	memset(emmc_SpareBuf,0xFF,SPARE_LEN);
	if(PBA + nSec >= EMMC_BOOT_PART_SIZE*5) 
		PBA &= (EMMC_BOOT_PART_SIZE-1);
	PBA = PBA + SD_CARD_BOOT_PART_OFFSET;

	for (len=0; len<nSec; len+=pageSizeLimit)
	{
		n = storage_mmc->block_dev.block_read(storage_device, PBA,
								  (MIN(nSec,pageSizeLimit)), emmc_Data);
		if(n != (MIN(nSec,pageSizeLimit)))
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
int StorageWritePba(uint32 PBA , void *pbuf, uint16 nSec )
{
	int ret = FTL_ERROR;
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
		for (i=0; i<(MIN(nSec,pageSizeLimit)); i++)
		{
			ftl_memcpy(emmc_Data+i*128, pDataBuf+(len+i)*132, 512);
			ftl_memcpy(emmc_SpareBuf+i*4, pDataBuf+(len+i)*132+128, 16);
		}
		n = storage_mmc->block_dev.block_write(storage_device, PBA,
							  (MIN(nSec,pageSizeLimit)), emmc_Data);
		if(n != (MIN(nSec,pageSizeLimit)))
			return -1;
	}
	return 0;
}

int StorageReadLba( uint32 LBA ,void *pbuf  , uint16 nSec)
{
	int ret = FTL_ERROR;
	int n;
	n = storage_mmc->block_dev.block_read(storage_device, LBA + SD_CARD_FW_PART_OFFSET,
						  nSec, pbuf);
	flush_cache((ulong)pbuf, nSec*512);
	return (n == nSec) ? 0 : -1;
}

int StorageWriteLba( uint32 LBA, void *pbuf  , uint16 nSec  ,uint16 mode)
{
	int ret = FTL_ERROR;
	int n;
	n = storage_mmc->block_dev.block_write(storage_device, LBA + SD_CARD_FW_PART_OFFSET,
							  nSec, pbuf);
	return (n == nSec) ? 0 : -1;
}

uint32 StorageGetCapacity(void)
{
	uint32 ret = FTL_ERROR;
	ret = storage_mmc->capacity/512 - SD_CARD_FW_PART_OFFSET;
	return ret;
}

uint32 StorageSysDataLoad(uint32 Index,void *Buf)
{
	uint32 ret = FTL_ERROR;
	int n;
	memset(Buf,0,512);
	n = storage_mmc->block_dev.block_read(storage_device, SD_CARD_SYS_PART_OFFSET+Index,
						  1, Buf);
	/* flush cache after read */
	flush_cache((ulong)Buf, 512); /* FIXME */
	return (n == 1) ? 0 : -1;
}

uint32 StorageSysDataStore(uint32 Index,void *Buf)
{
	uint32 ret = FTL_ERROR;
	int n;
	n = storage_mmc->block_dev.block_write(storage_device, SD_CARD_SYS_PART_OFFSET+Index,1, Buf);
	return (n == 1) ? 0 : -1;
}

#define UBOOT_SYS_DATA_OFFSET 64
uint32 StorageUbootDataLoad(uint32 Index,void *Buf) {
    StorageSysDataLoad(Index + UBOOT_SYS_DATA_OFFSET, Buf);
}

uint32 StorageUbootDataStore(uint32 Index,void *Buf) {
    StorageSysDataStore(Index + UBOOT_SYS_DATA_OFFSET, Buf);
}

uint32 UsbStorageSysDataLoad(uint32 offset,uint32 len,uint32 *Buf)
{
    uint32 ret = FTL_ERROR;
    uint32 i;
    for(i=0;i<len;i++)
    {
        StorageSysDataLoad(offset + 2 + i,Buf);
        if(offset + i < 2) 
        {
            Buf[0] = 0x444E4556;
            Buf[1] = 504;
        }
        Buf += 128;
    }
    return ret;
}

uint32 UsbStorageSysDataStore(uint32 offset,uint32 len,uint32 *Buf)
{
    uint32 ret = FTL_ERROR;
    uint32 i;
    for(i=0;i<len;i++)
    {
        Buf[0] = 0x444E4556;
        Buf[1] = 504;
        StorageSysDataStore(offset + 2 + i,Buf);
        Buf += 128;
    }
    return ret;
}

int StorageReadFlashInfo( void *pbuf)
{
    pFLASH_INFO pInfo=(pFLASH_INFO)pbuf;
    ftl_memset((uint8*)pbuf,0,512);
    pInfo->BlockSize = EMMC_BOOT_PART_SIZE;
    pInfo->ECCBits = 0;
    pInfo->FlashSize =  storage_mmc->capacity/512; 
    pInfo->PageSize = 4;
    pInfo->AccessTime = 40;
    pInfo->ManufacturerName=0;
    pInfo->FlashMask = 0;
    pInfo->FlashMask=1;
    return 0;
}

int StorageEraseBlock(uint32 blkIndex, uint32 nblk, uint8 mod)
{
    int Status = FTL_OK;
    return Status;
}

uint16 StorageGetBootMedia(void)
{
   return BOOT_FROM_EMMC;
}

uint32 StorageGetSDFwOffset(void)
{
	uint32 offset = 0;
	offset = SD_CARD_FW_PART_OFFSET;
	return offset;
}

uint32 StorageGetSDSysOffset(void)
{
	uint32 offset = 0;
	offset = SD_CARD_FW_PART_OFFSET;
	return offset;
}
uint32 StorageReadId(void *buf)
{
	uint8 * pbuf = buf;
	pbuf[0] = 'E';
	pbuf[1] = 'M';
	pbuf[2] = 'M';
	pbuf[3] = 'C';
	pbuf[4] = ' ';
}

