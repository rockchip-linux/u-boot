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
#ifndef _STORAGE_H
#define _STORAGE_H


#ifndef __GNUC__
#define PACKED1	__packed
#define PACKED2
#else
#define PACKED1
#define PACKED2	__attribute__((packed))
#endif

//1:flash 2:emmc 4:sdcard0 8:sdcard1
#define     BOOT_FROM_NULL    (0)
#define     BOOT_FROM_FLASH   (1<<0)
#define     BOOT_FROM_EMMC    (1<<1)
#define     BOOT_FROM_SD0     (1<<2)
#define     BOOT_FROM_SD1     (1<<3)
#define     BOOT_FROM_SPI     (1<<4)

#define     FTL_OK                  0
#define     FTL_ERROR               -1
#define     FTL_NO_FLASH            -2
#define     FTL_NO_IDB              -3

#define     DATA_LEN            (1024*8*2/4)              //数据块单位word
#define     SPARE_LEN           (32*8*2/4)               //校验数据长度
#define     PAGE_LEN            (DATA_LEN+SPARE_LEN)    //每个数据单位的长度


extern  void    FW_ReIntForUpdate(void);
extern  void	FW_SorageLowFormat(void);
extern  void    FW_SorageLowFormatEn(int en);
extern  uint32	FW_StorageGetValid(void);
extern  uint32	FW_GetCurEraseBlock(void);
extern  uint32	FW_GetTotleBlk(void);

extern  int StorageWriteLba(uint32 LBA, void *pbuf, uint16 nSec, uint16 mode);
extern  int StorageReadLba(uint32 LBA, void *pbuf, uint16 nSec);
extern  int StorageReadPba(uint32 PBA, void *pbuf, uint16 nSec);
extern  int StorageWritePba(uint32 PBA, void *pbuf, uint16 nSec);
extern  uint32 StorageGetCapacity(void);
extern  uint32 StorageSysDataLoad(uint32 Index, void *Buf);
extern  uint32 StorageSysDataStore(uint32 Index, void *Buf);
extern  uint32 StorageUbootDataStore(uint32 Index, void *Buf);
extern  uint32 StorageUbootDataLoad(uint32 Index, void *Buf);
extern  int StorageReadFlashInfo( void *pbuf);
extern  int StorageEraseBlock(uint32 blkIndex, uint32 nblk, uint8 mod);
extern  uint16 StorageGetBootMedia(void);
extern  uint32 StorageGetSDFwOffset(void);
extern  uint32 StorageGetSDSysOffset(void);
extern  int StorageReadId(void *pbuf);
extern  int32 StorageInit(void);
extern  uint32 UsbStorageSysDataLoad(uint32 offset, uint32 len, uint32 *Buf);
extern  uint32 UsbStorageSysDataStore(uint32 offset, uint32 len, uint32 *Buf);
#ifdef RK_SDCARD_BOOT_EN
extern  uint32 StorageSDCardUpdateMode(void);
#endif

//local memory operation function
typedef uint32 (*Memory_Init)(uint32 BaseAddr);
typedef uint32 (*Memory_ReadPba)(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec);
typedef uint32 (*Memory_WritePba)(uint8 ChipSel, uint32 PBA, void *pbuf, uint16 nSec);
typedef uint32 (*Memory_ReadLba)(uint8 ChipSel, uint32 LBA, void *pbuf, uint16 nSec);
typedef uint32 (*Memory_WriteLba)(uint8 ChipSel, uint32 LBA, void *pbuf, uint16 nSec, uint16 mode);
typedef uint32 (*Memory_Erase)(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
typedef void (*Memory_ReadID)(uint8 ChipSel, void *buf);
typedef void (*Memory_ReadInfo)(void *buf);
typedef void (*Memory_IntForUpdate)(void);
typedef uint32 (*Memory_LowFormat)(void);
typedef uint32 (*Memory_GetCurEraseBlock)(void);
typedef uint32 (*Memory_GetTotleBlk)(void);
typedef uint32 (*Memory_GetCapacity)(uint8 ChipSel);
typedef uint32 (*Memory_SysDataLoad)(uint8 ChipSel, uint32 Index, void *Buf);
typedef uint32 (*Memory_SysDataStore)(uint8 ChipSel, uint32 Index, void *Buf);

typedef struct MEM_FUN_Tag
{
	uint16 id;
	uint16 flag; // 传递给kernel的，确定从哪里引导到flash的
	uint32 Valid;
	Memory_Init Init;
	Memory_ReadID ReadId;
	Memory_ReadPba ReadPba;
	Memory_WritePba WritePba;
	Memory_ReadLba ReadLba;
	Memory_WriteLba WriteLba;
	Memory_Erase Erase;
	Memory_ReadInfo ReadInfo;
	Memory_IntForUpdate IntForUpdate;
	Memory_LowFormat LowFormat;
	Memory_GetCurEraseBlock GetCurEraseBlock;
	Memory_GetTotleBlk GetTotleBlk;
	Memory_GetCapacity GetCapacity;
	Memory_SysDataLoad SysDataLoad;
	Memory_SysDataStore SysDataStore;
} MEM_FUN_T, pMEM_FUN_T;


typedef PACKED1  struct  _FLASH_INFO//需要加__packed或着声明时4对齐不然程序可能在有判断的时候出现异常
{
	uint32  FlashSize;          //（Sector为单位）   4Byte
	uint16  BlockSize;          //（Sector为单位）   2Byte
	uint8   PageSize;           // (Sector为单位）    1Byte
	uint8   ECCBits;            //（bits为单位）    1Byte
	uint8   AccessTime;
	uint8   ManufacturerName;   // 1Byte
	uint8   FlashMask;          // 每一bit代表那个片选是否有FLASH
}PACKED2 FLASH_INFO, *pFLASH_INFO;


//for nand
typedef uint32 (*Memory_GetBlkSize)(void);
typedef uint32 (*Memory_FlashReadLba)(uint8 ChipSel, uint32 LBA, uint16 nSec, void *pbuf);
typedef uint32 (*Memory_FlashWriteLba)(uint8 ChipSel, uint32 LBA, uint16 nSec, void *pbuf);
typedef uint32 (*Memory_ftl_deinit)(void);
typedef uint32 (*Memory_flash_deinit)(void);
typedef void   (*uart_Trace)(const char* Format, ...);

typedef struct LOADER_MEM_API_Tag
{
	uint32 tag;                       //0x4e460001 
	uint32 id;                        //0 nand,1 emmc ,2 spi
	uint32 reversd0;                  //do not used
	uint32 reversd1;                  //do not used
	uart_Trace Trace;
	Memory_Init Init;
	Memory_ReadID ReadId;
	Memory_ReadPba ReadPba;
	Memory_WritePba WritePba;
	Memory_FlashReadLba ReadLba;
	Memory_FlashWriteLba WriteLba;         
	Memory_Erase Erase;
	Memory_ReadInfo ReadInfo;
	Memory_GetBlkSize getBlkSize;
	Memory_LowFormat LowFormat;
	Memory_ftl_deinit ftl_deinit;
	Memory_flash_deinit flash_deinit;
	Memory_GetCapacity GetCapacity;    //get capacity
	Memory_SysDataLoad SysDataLoad;    //vendor part,1MB
	Memory_SysDataStore SysDataStore;  //vendor part,1MB
} LOADER_MEM_API_T, *pLOADER_MEM_API_T;

//1全局变量
#undef	EXT
#ifdef	IN_STORAGE
#define	EXT
#else
#define	EXT		extern
#endif

EXT MEM_FUN_T * gpMemFun;
EXT uint32 gIdDataBuf[512] __attribute__((aligned(ARCH_DMA_MINALIGN)));
EXT FLASH_INFO g_FlashInfo __attribute__((aligned(ARCH_DMA_MINALIGN)));

#endif

