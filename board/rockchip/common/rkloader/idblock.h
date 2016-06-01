/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef RKIDBLOCK_H
#define RKIDBLOCK_H


#ifndef __GNUC__
#define PACKED1 __packed
#define PACKED2
#else
#define PACKED1
#define PACKED2 __attribute__((packed))
#endif


/* 函数返回的错误码 */
#define ERR_SUCCESS			0
#define ERR_DEVICE_READY		ERR_SUCCESS
#define ERR_DEVICE_OPEN_FAILED		-1
#define ERR_DEVICE_WRITE_FAILED		-2 
#define ERR_DEVICE_READ_FAILED		-3
#define ERR_CMD_NOTMATCH		-4
#define ERR_DEVICE_UNREADY		-5
#define ERR_DEVICE_SET_FAILED		-6
#define ERR_FOUND_BAD_BLOCK		-7
#define ERR_FAILED			-8
#define ERR_CROSS_BORDER		-9
#define ERR_CSW_OPEN_FAILED		-10


#define FLASH_ID_LEN			5
#define BLOCK_STATE_BYTES		64
#define MAX_TEST_BLOCK			512 /* 一次可测试的最大块数 */
#define MAX_ERASE_BLOCK			128 /* 一次可擦除的最大块数 */

#define SECTOR_SIZE			512
#define MLC_SECTOR_SIZE			528
#define SLC_SECTOR_SIZE			SECTOR_SIZE
#define MAX_RW_SECTOR			20 /* 一次可读写的最大扇区数 */
#define MAX_MANUFACTURER_NAME		16
#define MAX_FLASH_NUMBER		8

#define CELL_SLC			0
#define CELL_MLC			1

#define MAX_FW_SIZE			(64 * 1024 - 1)
#define DEVICE_TRY_COUNT		5

#define IOCTL_TRANSFER_SIZE		4096
#define IDB_BLOCKS			5
#define EIB_BLOCKS			5
#define RESERVE_BLOCKS			2

#define MAX_TRY_COUNT			20 /* 重试20次就够了 */
#define MAX_BLOCK_STATE			512
#define MAX_BLOCK_SEARCH 		200

#define MAX_WRITE_SECTOR		20

#define UINT    	unsigned int
#define UCHAR   	unsigned char
#define USHORT  	unsigned short
#define BYTE    	unsigned char
#define PBYTE   	BYTE*
#define LPBYTE		BYTE*

/* id block: sector0 */
typedef PACKED1 struct _Sector0Info {	/* 总共512Bytes */
	UINT	fwSig;
	UCHAR	reserved[4];
	UINT	uiRc4Flag;		/* ddr and loader bin rc4 flaf - 1:  no rc4, 0: rc4 */
	USHORT	usBootCode1Offset;
	USHORT	usBootCode2Offset;
	UCHAR	reserved1[490];
	USHORT  usFlashDataSize;	/* 扇区为单位 */
	USHORT	ucFlashBootSize;
	UCHAR	reserved2[2];
}PACKED2 Sector0Info;

/* id block: sector1 */
typedef PACKED1 struct _Sector1Info {	/* 总共512Bytes */
	USHORT  usSysReservedBlock;	/* 2 bytes */
	USHORT  usDisk0Size;		/* 2 bytes */
	USHORT  usDisk1Size;		/* 2 bytes */
	USHORT  usDisk2Size;		/* 2 bytes */
	USHORT  usDisk3Size;		/* 2 bytes */
	UINT	uiChipTag;
	UINT	uiMachineId;
	USHORT	usLoaderYear;
	USHORT	usLoaderDate;
	USHORT	usLoaderVer;		/* Master and slave version */
	UCHAR   reserved[72];
	USHORT  usFlashDataOffset;
	USHORT  usFlashDataLen;		/* 以Sector为单位 */
	UCHAR	reserved2[384];
	UINT    uiFlashChipSize; 	/* 以Sector为单位 */
	UCHAR   reserved1;
	UCHAR   bAccessTime;
	USHORT  usPhyBlockSize;
	UCHAR   bPhyPageSize;
	UCHAR   bECCBits;

	BYTE	reserved3[8];
	USHORT  usIdBlock0;
	USHORT  usIdBlock1;
	USHORT  usIdBlock2;
	USHORT  usIdBlock3;
	USHORT  usIdBlock4;
}PACKED2 Sector1Info;

/* id block: sector2 */
typedef PACKED1 struct _Sector2Info {	/*总共512Bytes */
	USHORT  chipInfoSize;
	UCHAR   chipInfo[510];
}PACKED2 Sector2Info;


#define TAG_MANUFACTURER	"M.F.T"
#define TAG_PRODUCT_SERIES	"PROD_SERIES"
#define TAG_PRODUCT		"PRODUCT"
#define TAG_SUB_PRODUCT		"SUB_PROD"
#define TAG_MACHINE_SN		"SN"

#define SN_MAX_SIZE		30
#define SEC3_RESERVED_SIZE 	435
#define UID_MAX_SIZE 		30
#define BT_MAX_SIZE 		6
#define MAC_MAX_SIZE 		6

/* id block: sector3 */
typedef PACKED1 struct _Sector3Info {
	USHORT  snSize;
	UCHAR   sn[SN_MAX_SIZE];
	UCHAR   macTag[3];
	UCHAR	macSize;
	UCHAR	macAddr[6];
}PACKED2 Sector3Info;


typedef UCHAR Sector[512];
typedef UCHAR Spare[16];

/* idb */
typedef PACKED1 struct _IDBlock {
	Sector0Info	sector0;
	Sector1Info	sector1;
	Sector		sector2;
	Sector		sector3;
	UCHAR		SDRAM_Initial[512];
	UCHAR		*flashBoot;
} PACKED2 IDBlock;

typedef PACKED1 struct _DataInfo {
	BOOL 	bIsExist;		/* 0,该数据不存在；1,存在 */
	int 	iOffset;
	int 	iLength;
	UINT 	uiRAMAddress;
} PACKED2 DataInfo;

typedef PACKED1 struct _ManufacturerInfo {
	UCHAR	manufacturerID;
	char	manufacturerName[MAX_MANUFACTURER_NAME];
} PACKED2 ManufacturerInfo;


typedef	PACKED1 struct _FlashID {
	UCHAR makerCode;
	UCHAR deviceCode;
	UCHAR cellType;		/* 0: 512 bytes;1: 2024 bytes; 2: 2048 bytes */
	UCHAR flashType;
	UCHAR reserved2;
} PACKED2 FlashID;

/* Flash 信息 */
typedef	PACKED1 struct _FlashInfo {
	char szManufacturerName[MAX_MANUFACTURER_NAME];
	UINT uiFlashSize;		/* MB */
	USHORT usBlockSize;		/* KB */
	UINT uiPageSize;		/* KB */
	UINT uiSectorPerBlock;
	//	UCHAR *BlockState;
	UCHAR   BlockState[200];
	UINT	uiBlockNum;
	BYTE	bECCBits;
	BYTE	bAccessTime;		/* 两次访问Flash的间隔时间 */
	BYTE	bFlashCS;		/* Flash片选(若Flash片选存在，则将相应的Bit置1，否则置0) */
} PACKED2 FlashInfo, *PFlashInfo;

typedef PACKED1 struct _FlashInfoCmd {
	UINT	uiFlashSize;	/* Flash大小（以Sector为单位） */
	USHORT	usBlockSize;	/* 物理的Block大小（以Sector为单位） */
	BYTE	bPageSize;	/* 物理的Page大小（以Sector为单位） */
	BYTE	bECCBits;	/* 8/14 */
	BYTE	AccessTime;	/* 两次访问Flash的间隔时间 */
	BYTE	bManufCode;	/* 厂商识别码 */
	BYTE	bFlashCS;	/* Flash片选(若Flash片选存在，则将相应的Bit置1，否则置0) */
} PACKED2 FlashInfoCmd, *PFlashInfoCmd;


#define BOOTSIGN		"RK28@Copyright2008Rockchip"
#define BOOTSIGN_SIZE		32
#define CHECK_SIZE		16
#define HEADINFO_SIZE		512

typedef PACKED1 struct _rk_time {
	USHORT		usYear;
	USHORT		usMonth;
	USHORT		usDate;
	USHORT		usHour;
	USHORT		usMinute;
	USHORT		usSecond;
} PACKED2 RK_TIME;

typedef PACKED1 struct _RK28BOOT_HEAD {
	char		szSign[BOOTSIGN_SIZE];
	UCHAR		bMD5Check[CHECK_SIZE];
	RK_TIME		tmCreateTime;

	UINT		uiMajorVersion;
	UINT		uiMinorVersion;

	UINT		uiUsbDataOffset;
	UINT		uiUsbDataLen;

	UINT		uiUsbBootOffset;
	UINT		uiUsbBootLen;

	UINT		uiFlashDataOffset;
	UINT		uiFlashDataLen;

	UINT       	uiFlashBootOffset;
	UINT		uiFlashBootLen;
	UCHAR		ucRc4Flag;

	UINT		MergerVersion;	/* 生成Boot文件所用Merger工具的版本号(高16字节为主版本号、低16字节为副版本号) */
} PACKED2 RK28BOOT_HEAD, *PRK28BOOT_HEAD;


#define SECURE_BOOT_SIGN_TAG    0x4E474953

extern char bootloader_ver[24];

int rkidb_update_loader(bool dataLoaded);
void rkidb_setup_space(uint32 begin_addr);
int rkidb_get_bootloader_ver(void);
int rkidb_get_idblk_data(void);
int rkidb_get_rc4_flag(void);
int rkidb_get_sn(char *buf);
bool rkidb_get_mac_address(char *macaddr);
int rkidb_erase_drm_key(void);
int rkidb_get_hdcp_key(char *buf, int offset, int size);

#endif /* RKIDBLOCK_H */
