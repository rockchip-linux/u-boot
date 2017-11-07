/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
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
#define     BOOT_FROM_UMS     (1<<5)
#define     BOOT_FROM_NVME    (1<<6)

#define     FTL_OK                  0
#define     FTL_ERROR               -1
#define     FTL_NO_FLASH            -2
#define     FTL_NO_IDB              -3

#define     DATA_LEN            (1024*8*2/4)              //数据块单位word
#define     SPARE_LEN           (32*8*2/4)               //校验数据长度
#define     PAGE_LEN            (DATA_LEN+SPARE_LEN)    //每个数据单位的长度

#define EMMC_VENDOR_PART_START		(1024 * 7)
#define NAND_VENDOR_PART_START		0
#define VENDOR_PART_NUM			4
#define NAND_VENDOR_PART_NUM		2
#define VENDOR_TAG			0x524B5644
#define VENDOR_PART_SIZE		128

#define VENDOR_SN_ID			1
#define VENDOR_WIFI_MAC_ID		2
#define VENDOR_LAN_MAC_ID		3
#define VENDOR_BLUETOOTH_ID		4
#define VENDOR_IMEI_ID			5
#define VENDOR_OEM_UNLOCKED_ID		6

struct vendor_item {
	u16  id;
	u16  offset;
	u16  size;
	u16  flag;
};

struct vendor_info {
	u32	tag;
	u32	version;
	u16	next_index;
	u16	item_num;
	u16	free_offset;
	u16	free_size;
	struct	vendor_item item[126]; /* 126 * 8*/
	u8	data[VENDOR_PART_SIZE * 512 - 1024 - 8];
	u32	hash;
	u32	version2;
};

extern  void    FW_ReIntForUpdate(void);
extern  void	FW_SorageLowFormat(void);
extern  void    FW_SorageLowFormatEn(int en);
extern  uint32	FW_StorageGetValid(void);
extern  uint32	FW_GetCurEraseBlock(void);
extern  uint32	FW_GetTotleBlk(void);

extern  int StorageWriteLba(uint32 LBA, void *pbuf, uint32 nSec, uint16 mode);
extern  int StorageReadLba(uint32 LBA, void *pbuf, uint32 nSec);
extern  int StorageReadPba(uint32 PBA, void *pbuf, uint32 nSec);
extern  int StorageWritePba(uint32 PBA, void *pbuf, uint32 nSec);
extern  uint32 StorageGetCapacity(void);
extern  uint32 StorageSysDataLoad(uint32 Index, void *Buf);
extern  uint32 StorageSysDataStore(uint32 Index, void *Buf);
extern  uint32 StorageUbootSysDataStore(uint32 Index, void *Buf);
extern  uint32 StorageUbootSysDataLoad(uint32 Index, void *Buf);
extern  int StorageReadFlashInfo( void *pbuf);
extern  int StorageEraseBlock(uint32 blkIndex, uint32 nblk, uint8 mod);
extern  uint16 StorageGetBootMedia(void);
extern  uint32 StorageGetSDFwOffset(void);
extern  uint32 StorageGetSDSysOffset(void);
extern  int StorageReadId(void *pbuf);
extern  int32 StorageInit(void);
extern  uint32 StorageVendorSysDataLoad(uint32 offset, uint32 len, uint32 *Buf);
extern  uint32 StorageVendorSysDataStore(uint32 offset, uint32 len, uint32 *Buf);
#ifdef RK_SDCARD_BOOT_EN
extern  bool StorageSDCardUpdateMode(void);
extern	bool StorageSDCardBootMode(void);
#endif
#ifdef RK_UMS_BOOT_EN
extern	bool StorageUMSUpdateMode(void);
extern	bool StorageUMSBootMode(void);
#endif
int StorageEraseData(uint32 lba, uint32 n_sec);

//local memory operation function
typedef uint32 (*Memory_Init)(uint32 BaseAddr);
typedef uint32 (*Memory_ReadPba)(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
typedef uint32 (*Memory_WritePba)(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec);
typedef uint32 (*Memory_ReadLba)(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec);
typedef uint32 (*Memory_WriteLba)(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec, uint32 mode);
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
typedef uint32 (*Memory_erase_data)(uint8 ChipSel, uint32 LBA, uint32 nSec);
typedef uint32 (*Memory_read_fat_bbt)(uint8 *pbbt, uint32 die, uint32 len);

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
	Memory_erase_data erase_data;
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
typedef uint32 (*Memory_FlashReadLba)(uint8 ChipSel, uint32 LBA, uint32 nSec, void *pbuf);
typedef uint32 (*Memory_FlashWriteLba)(uint8 ChipSel, uint32 LBA, uint32 nSec, void *pbuf);
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
	Memory_read_fat_bbt read_bbt;
	Memory_erase_data erase_data;
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
int vendor_storage_init(void);
int vendor_storage_read(u32 id, void *pbuf, u32 size);
int vendor_storage_write(u32 id, void *pbuf, u32 size);

#endif

