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

#include <fastboot.h>
#include <malloc.h>
#include <linux/sizes.h>
#include "../config.h"
DECLARE_GLOBAL_DATA_PTR;

extern unsigned long gIdDataBuf[512];

char bootloader_ver[24]="";
uint16 internal_boot_bloader_ver = 0;
uint16 update_boot_bloader_ver = 0;
uint32 g_id_block_size = 1024;

extern void P_RC4(unsigned char * buf, unsigned short len);

#if 0
#undef GET_BYTE
#undef PUT_BYTE
#undef GET_CHAR
#undef PUT_CHAR

#define GET_BYTE(pb, ib)	(pb)[(ib)]
#define PUT_BYTE(pb, ib, b)	(pb)[(ib)] = (b)
#define GET_CHAR(pch, ich)	(pch)[(ich)]
#define PUT_CHAR(pch, ich, ch)	(pch)[(ich)] = (ch)


//重新定义类型
#define DRM_VOID  void
#define DRM_API
#define IN
#define OUT

#define DRM_BYTE  unsigned char
#define DRM_DWORD unsigned int
#define DRM_UINT  unsigned int
#define DRM_INT   int

#define RC4_TABLESIZE 256

/* Key structure */
typedef struct __tagRC4_KEYSTRUCT
{
	unsigned char S[(RC4_TABLESIZE)];		/* State table */
	unsigned char i, j;						/* Indices */
} RC4_KEYSTRUCT;

DRM_VOID DRM_API NAND_RC4_KeySetup(
		OUT   RC4_KEYSTRUCT  *pKS,
		IN        DRM_DWORD       cbKey,
		IN  const DRM_BYTE       *pbKey)
{
	DRM_BYTE j;
	DRM_BYTE k;
	DRM_BYTE t;
	DRM_INT  i;

	for (i = 0;i < RC4_TABLESIZE;i++)
	{
		PUT_BYTE(pKS->S, i, (DRM_BYTE)i);
	}

	pKS->i = 0;
	pKS->j = 0;
	j      = 0;
	k      = 0;
	for (i = 0;i < RC4_TABLESIZE;i++)
	{
		t = GET_BYTE(pKS->S, i);
		j = (DRM_BYTE)((j + t + GET_BYTE(pbKey, k)) % RC4_TABLESIZE);
		PUT_BYTE(pKS->S, i, GET_BYTE(pKS->S, j));
		PUT_BYTE(pKS->S, j, t);
		k = (DRM_BYTE)((k + 1) % cbKey);
	}
}

/******************************************************************************/
DRM_VOID DRM_API NAND_RC4_Cipher(
		IN OUT RC4_KEYSTRUCT *pKS,
		IN     DRM_UINT       cbBuffer,
		IN OUT DRM_BYTE      *pbBuffer)
{
	DRM_BYTE  i = pKS->i;
	DRM_BYTE  j = pKS->j;
	DRM_BYTE *p = pKS->S;
	DRM_DWORD ib = 0;

	while (cbBuffer--)
	{
		DRM_BYTE bTemp1 = 0;
		DRM_BYTE bTemp2 = 0;

		i = ((i + 1) & (RC4_TABLESIZE - 1));
		bTemp1 = GET_BYTE(p, i);
		j = ((j + bTemp1) & (RC4_TABLESIZE - 1));

		PUT_BYTE(p, i, GET_BYTE(p, j));
		PUT_BYTE(p, j, bTemp1);
		bTemp2 = GET_BYTE(pbBuffer, ib);

		bTemp2 ^= GET_BYTE(p, (GET_BYTE(p, i) + bTemp1) & (RC4_TABLESIZE - 1));
		PUT_BYTE(pbBuffer, ib, bTemp2);
		ib++;
	}

	pKS->i = i;
	pKS->j = j;
}

/*****************************************************************************************/
#define RK_IDBLOCK_RC4KEY_KEY   {124,78,3,4,85,5,9,7,45,44,123,56,23,13,23,17};
#define RK_IDBLOCK_RC4KEY_LEN   16

void GetIdblockDataNoRc4(char * fwbuf, int len)
{
	RC4_KEYSTRUCT       rc4KeyStruct;
	unsigned char       rc4Key[RK_IDBLOCK_RC4KEY_LEN] = RK_IDBLOCK_RC4KEY_KEY

	NAND_RC4_KeySetup(&rc4KeyStruct, RK_IDBLOCK_RC4KEY_LEN , rc4Key);
	NAND_RC4_Cipher(&rc4KeyStruct , len, (DRM_BYTE *)fwbuf);
}

#else

static void GetIdblockDataNoRc4(char * fwbuf, int len)
{
	P_RC4((unsigned char*)fwbuf, len);
}
#endif

#if 0
int GetIdBlockSysData(char * buf, int Sector)
{
	int ret = -1;
	if(Sector <= 3)
	{
		memcpy(buf,gIdDataBuf+128*Sector,512);
		if(Sector!=1)
			GetIdblockDataNoRc4(buf,512);
		ret = 0;
	}
	return ret;
}


char GetSNSectorInfo(char * pbuf)
{
	return (GetIdBlockSysData(pbuf,3));    
}

char GetChipSectorInfo(char * pbuf)
{
	return (GetIdBlockSysData(pbuf,2));    
}
#endif

int get_bootloader_ver(char *boot_ver)
{
	uint8 *buf = (uint8*)&gIdDataBuf[0];
	memset(bootloader_ver,0,24);

	if( *(uint32*)buf == 0xfcdc8c3b )
	{
		uint16 year, date;
		// GetIdblockDataNoRc4((char*)&gIdDataBuf[0],512);
		GetIdblockDataNoRc4((char*)&gIdDataBuf[128*2],512);
		GetIdblockDataNoRc4((char*)&gIdDataBuf[128*3],512);
		year = *(uint16*)((uint8*)buf+512+18);
		date = *(uint16*)((uint8*)buf+512+20);
		internal_boot_bloader_ver = *(uint16*)((uint8*)buf+512+22);
		//loader_tag_set_version( year<<16 |date , ver>>8 , ver&0xff );
		sprintf(bootloader_ver,"%04X-%02X-%02X#%X.%02X",
				year,
				(uint8)((date>>8)&0x00FF), (uint8)(date&0x00FF),
				(uint8)((internal_boot_bloader_ver>>8)&0x00FF), (uint8)(internal_boot_bloader_ver&0x00FF));
		return 0;
	}
	return -1;
}


uint8* g_32secbuf;
uint8* g_cramfs_check_buf;

uint8* g_pIDBlock;
uint8* g_pLoader;
uint8* g_pReadBuf;
uint8* g_pFlashInfoData;

#if 1
FlashInfo m_flashInfo;
uint16 g_IDBlockOffset[5];

#define CALC_UNIT(a, b)		((a>0)?((a-1)/b+1):(a))			// 计算a可以分成多少个b，剩余部分算1个
#define MB2SECTOR(x)		(x*1024*(1024/SECTOR_SIZE))		// x MB的数据所须的Sector数
#define BYTE2SECTOR(x)		(CALC_UNIT(x, SECTOR_SIZE))		// x Bytes所须的Sector数
#define INT2BCD(num) (((num)%10)|((((num)/10)%10)<<4)|((((num)/100)%10)<<8)|((((num)/1000)%10)<<12))
#define PAGEALIGN(x)		(CALC_UNIT(x, 4))//x sectors所需的page数

/***********************************************************
 *  Copyright (C),2007-2008, Fuzhou Rockchip Co.,Ltd.
 *  Function name : BuildNFBlockStateMap()
 *  Author:         Meiyou Chen
 *  Description:    生成指定Flash的块状态表
 *  Calls:          TestBadBlock()
 *  Input:          ucFlashIndex - 指定的Flash, uiNFBlockLen - 待生成的块数
 *  Output:         NFBlockState - Flash块状态表
 *  Return:         TRUE - 成功	FALSE - 失败
 *  History:        
 *           <author>      <time>     <version>       <desc>
 *          Meiyou Chen   07/12/20       1.0            ORG
 *
 **********************************************************/
static uint32 BuildNFBlockStateMap(uint8 ucFlashIndex, uint8 *NFBlockState, uint32 uiNFBlockLen)
{
	return TRUE;
}

/*************************** 生成指定Flash的所有块的状态表 ******************************/
static uint32 BuildFlashStateMap(uint8 ucFlashIndex, FlashInfo *pFlash)
{
	memset((void*)pFlash->BlockState, 0, 200);
	return BuildNFBlockStateMap(ucFlashIndex, pFlash->BlockState, 200);//pFlash->uiBlockNum);//Test 512个块就足够了 pFlash->uiBlockNum);
}

/***********************************************************
 *  Copyright (C),2007-2008, Fuzhou Rockchip Co.,Ltd.
 *  Function name :	FindSerialBlocks()
 *  Author:			Meiyou Chen
 *  Description:		查找指定位置开始的连续多个好块的位置
 *  Calls:			
 *  Input:			NFBlockState - 块状态表, iNFBlockLen - 块状态表所含的块的个数,
 *					iBegin - 起始块, iLen - 连续的块数
 *  Output:			
 *  Return:			-1 - 失败, 其它值指示找到的位置
 *  History:			
 *           <author>      <time>     <version>       <desc>
 *          Meiyou Chen   07/12/20       1.0            ORG
 *
 **********************************************************/
static int FindSerialBlocks(uint8 *NFBlockState, int iNFBlockLen, int iBegin, int iLen)
{
	int iCount = 0;
	int iIndex = iBegin;
	while(iBegin < iNFBlockLen)
	{
		if(0 == NFBlockState[iBegin++])
			++iCount;
		else
		{
			iCount = 0;
			iIndex = iBegin;
		}
		if(iCount >= iLen)
			break;
	}
	if(iBegin >= iNFBlockLen)
		iIndex = -1;

	return iIndex;
}

//寻找ID Block，该块在位于最前面的20个好块中若能找到，则是我们的片子，否则便是其它公司或者新片
static int FindIDBlock(FlashInfo* pFlashInfo, int iStart, int *iPos)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, ucSpareData, 4*528);
	int iRet = ERR_SUCCESS;
	int i = FindSerialBlocks(pFlashInfo->BlockState, MAX_BLOCK_SEARCH/*MAX_BLOCK_STATE*/, iStart, 1);

	*iPos = 0;
	//	PRINT_I("i = %d \n", i);
	if ( i<0 )
	{
		return -1;
	}
	for(; i<MAX_BLOCK_SEARCH/*MAX_BLOCK_STATE*/; i=FindSerialBlocks(pFlashInfo->BlockState, MAX_BLOCK_SEARCH/*MAX_BLOCK_STATE*/, i+1, 1))
	{
		//	    PRINT_I("i = %d \n", i);
		if ( i<0 ) break;
		memset(ucSpareData, 0, 4*528);
		iRet = StorageReadPba(i*g_id_block_size, ucSpareData, 4);

		if(ERR_SUCCESS != iRet)
		{
			continue;
		}
		unsigned int tag = 0;
		memcpy(&(tag), ucSpareData, sizeof(tag));
		if (tag == (unsigned int)0xfcdc8c3b ) 
		{
			*iPos = i;
			return 0;//找到idb
		}
		else
			continue;
	}
	return -1;							// new mp3
}

static int FindAllIDB(void)
{
	int i,iRet,iIndex,iStart=0,iCount=0;

	if(g_FlashInfo.BlockSize)
		g_id_block_size = g_FlashInfo.BlockSize;

	memset(g_IDBlockOffset, 0xFFFF, 5);
	for( i=0; i<5; i++)
	{
		//	    PRINT_I("find %d \n", i);
		iRet = FindIDBlock( &m_flashInfo, iStart, &iIndex );
		if ( iRet<0 )
		{
			return iCount;
		}
		g_IDBlockOffset[i] = iIndex;
		iCount++;
		iStart = iIndex+1;
	}
	return iCount;
}

#define SECTOR_OFFSET 528

extern void ReadFlashInfo(void *buf);
extern uint16 update_boot_bloader_ver;
extern uint16 internal_boot_bloader_ver;
static bool GenericIDBData(PBYTE pIDBlockData, UINT *needIdSectorNum)
{
	//	PBYTE pIDB,pFlashData,pFlashCode;
	int iRet,i;
	UINT uiSectorPerBlock;
	Sector1Info *pSec1;
	Sector0Info *pSec0;
	//	Sector3Info *pSec3;
	RK28BOOT_HEAD *hdr = NULL;
	int hasFlashInfo = 0;

	//cmy: 获取已有的IDBlock加密数据，并进行解密

	//1.读出前4个secotor
	uiSectorPerBlock = g_id_block_size;
	memset( (void*)pIDBlockData, 0, 4*SECTOR_OFFSET );

	iRet = StorageReadPba( g_IDBlockOffset[0]*uiSectorPerBlock, (void*)pIDBlockData, 4 );
	if ( iRet!=ERR_SUCCESS )
	{
		PRINT_I("ERROR: %d\n", iRet);
		return FALSE;
	}

	//2.IDB解密
	PRINT_I("DDDDD...\n");
	for(i=0; i<4; i++)
	{
		if(i != 1)
			P_RC4(pIDBlockData+SECTOR_OFFSET*i, 512);
	}
	memset(pIDBlockData+SECTOR_OFFSET*2, 0, SECTOR_OFFSET ); //靠 chip info,靠靠靠靠sn
	PRINT_I("OK\n");

	pSec0 = (Sector0Info *)(pIDBlockData);
	pSec1 = (Sector1Info *)(pIDBlockData+SECTOR_OFFSET);
#ifdef CONFIG_NAND
	if( pSec1->usFlashDataOffset && pSec1->usFlashDataLen )
	{
		hasFlashInfo = 1;
		ReadFlashInfo(g_pFlashInfoData);
	}
#endif

	//cmy: 使用新的loader代码更新IDBlock的数据
	hdr = (RK28BOOT_HEAD*)g_pLoader;
	pSec0->reserved[4] = hdr->ucRc4Flag;

	PRINT_I("update loader data\n");

	// update page0
	// cmy: 更新FlashData及FlashBoot内容
	pSec0->usFlashDataSize = PAGEALIGN(BYTE2SECTOR(hdr->uiFlashDataLen))*4;
	pSec0->ucFlashBootSize = PAGEALIGN(BYTE2SECTOR(hdr->uiFlashBootLen))*4;

#define SMALL_PACKET                   512
	for(i=0; i<pSec0->ucFlashBootSize; i++)
	{
		if (hdr->ucRc4Flag) {
			P_RC4((void*)(g_pLoader+hdr->uiFlashBootOffset+i*512), 512);
		}
		ftl_memcpy( (void*)(pIDBlockData+SECTOR_OFFSET*(4+pSec0->usFlashDataSize+i)), (void*)(g_pLoader+hdr->uiFlashBootOffset+i*512), 512 );
	}
	for(i=0; i<pSec0->usFlashDataSize; i++)
	{
		if (hdr->ucRc4Flag) {
			P_RC4((void*)(g_pLoader+hdr->uiFlashDataOffset+i*512), 512);
		}
		ftl_memcpy( (void*)(pIDBlockData+SECTOR_OFFSET*(4+i)), (void*)(g_pLoader+hdr->uiFlashDataOffset+i*512), 512 );
	}
	pSec0->ucFlashBootSize += pSec0->usFlashDataSize;

	*needIdSectorNum = 4+pSec0->ucFlashBootSize;

	// cmy: 更新Loader的版本信息
	PRINT_I("update date and version\n");
	pSec1->usLoaderYear = INT2BCD(hdr->tmCreateTime.usYear);
	pSec1->usLoaderDate = (INT2BCD(hdr->tmCreateTime.usMonth)<<8)|INT2BCD(hdr->tmCreateTime.usDate);
	pSec1->usLoaderVer = (INT2BCD(hdr->uiMajorVersion)<<8)|INT2BCD(hdr->uiMinorVersion);

	if( hasFlashInfo )
	{
		pSec1->usFlashDataOffset = *needIdSectorNum;
		*needIdSectorNum += (PAGEALIGN(pSec1->usFlashDataLen)*4);
	}

	// cmy: 对新的IDBlock数据进行打包
	PRINT_I("EEEEE...\n");
	for(i=0; i<4; i++)
	{
		if(i != 1)
			P_RC4(pIDBlockData+SECTOR_OFFSET*i, 512);
	}

	if( hasFlashInfo )
	{
		for(i=0; i<pSec1->usFlashDataLen; i++)
		{
			ftl_memcpy((void*)(pIDBlockData+SECTOR_OFFSET*(pSec1->usFlashDataOffset+i)), (void*)g_pFlashInfoData, 512);
		}
	}

	PRINT_I("OK\n");

	return TRUE;
}

static int get_rk28boot(uint8 * pLoader, bool dataLoaded)
{
	const disk_partition_t *misc_part = get_disk_partition(MISC_NAME);
	if (!misc_part) {
		printf("misc partition not found!\n");
		return -1;
	}
	RK28BOOT_HEAD* hdr = (RK28BOOT_HEAD*)pLoader;
	int nBootSize = 0;

	if (!dataLoaded)
	{
		if(StorageReadLba(misc_part->start+96, (void*)pLoader,4)!=0 )
		{
			//PRINT_E("ERROR: StorageRead(%d, %d, %d, 0x%p) Failed!\n", 0, misc_part->offset+96, 4, pLoader);
			return -1;
		}
	}

	if(strcmp(BOOTSIGN, hdr->szSign) )
	{
		return -2;
	}

	nBootSize = HEADINFO_SIZE
		+ hdr->uiUsbDataLen
		+ hdr->uiUsbBootLen
		+ hdr->uiFlashDataLen
		+ hdr->uiFlashBootLen;

	if (!dataLoaded)
	{
		if( CopyFlash2Memory( (int32)pLoader, misc_part->start+96, BYTE2SECTOR(nBootSize)) )
			return -3;
	}

	update_boot_bloader_ver = (INT2BCD(hdr->uiMajorVersion)<<8)|INT2BCD(hdr->uiMinorVersion);
	return 0;
}

//返回值：
// true  成功
// false 失败
static bool WriteXIDBlock(USHORT *pSysBlockAddr, int iIDBCount, UCHAR *idBlockData, UINT uiIdSectorNum)
{
	//	UCHAR *pIDBlockData = NULL;
	int i=0, ii=0;
	int write_failed = 0;

	////////////////// ID BLOCKS /////////////////////////////////////////////
	PBYTE writeBuf;
	PBYTE readBuf = (PBYTE)g_pReadBuf;
	UINT sysSectorAddr = 0;
	int iCMDRet;
	// 在写IDB失败后，重试一次
	int retry = 0;

	PRINT_I("Enter write idb\n");
	for(i=0; i<iIDBCount && (iIDBCount-write_failed)>1; i++)
	{
		UINT uiNeedWriteSector = uiIdSectorNum;
		UINT uiWriteSector = 0;
		int iCount = 0;

		PRINT_D("Erase block %d\n", pSysBlockAddr[i]);
		iCMDRet = StorageEraseBlock(pSysBlockAddr[i],1,1);
		if ( iCMDRet!=ERR_SUCCESS )
		{
			//PRINT_E("erase B%d failed: %d\n", pSysBlockAddr[i], iCMDRet);
			retry = !retry;
			retry?--i:++write_failed;
			continue;
		}

		sysSectorAddr = pSysBlockAddr[i]*g_id_block_size;
		PRINT_I("write IDB%d to SEC%d\n", i, sysSectorAddr);
		while(uiNeedWriteSector > 0)
		{
			uiWriteSector = (uiNeedWriteSector>MAX_WRITE_SECTOR)?MAX_WRITE_SECTOR:uiNeedWriteSector;
			writeBuf = idBlockData+iCount*MAX_WRITE_SECTOR*528;
			PRINT_D("write sector %08d ~ %08d\n", sysSectorAddr+iCount*MAX_WRITE_SECTOR, sysSectorAddr+iCount*MAX_WRITE_SECTOR+uiWriteSector-1);
			iCMDRet = StorageWritePba(sysSectorAddr+iCount*MAX_WRITE_SECTOR, writeBuf , uiWriteSector);
			if (ERR_SUCCESS != iCMDRet)
			{
				PRINT_E("write failed %d\n", iCMDRet);
				break;
			}

			// 读取数据
			memset(readBuf, 0xff, MAX_WRITE_SECTOR*(512+16));
			PRINT_D("read sector %08d ~ %08d\n", sysSectorAddr+iCount*MAX_WRITE_SECTOR, sysSectorAddr+iCount*MAX_WRITE_SECTOR+uiWriteSector-1);
			iCMDRet = StorageReadPba(sysSectorAddr+iCount*MAX_WRITE_SECTOR, readBuf, uiWriteSector);
			if (ERR_SUCCESS != iCMDRet)
			{
				PRINT_E("read failed %d\n", iCMDRet);
				break;
			}
			// 校验所写的数据
			PRINT_D("check data...\n");
			for (ii = 0; ii<uiWriteSector; ii++)
			{
				if(0 != memcmp(writeBuf+528*ii, readBuf+528*ii, 512))
				{
					PRINT_E("check failed %d\n", sysSectorAddr+iCount*MAX_WRITE_SECTOR+ii);
					break;
				}
			}
			if(ii!=uiWriteSector)
				break;

			PRINT_D("Okay!\n");
			++iCount;
			uiNeedWriteSector -= uiWriteSector;
		}
		if(uiNeedWriteSector == 0)
			PRINT_D("IDB[%d] write complete\n", pSysBlockAddr[i]);
		else
		{
			PRINT_D("IDB[%d] write abort\n", pSysBlockAddr[i]);
			retry = !retry;
			retry?--i:++write_failed;
			//		    RKU_EraseBlock(0, pSysBlockAddr[i], 1);
		}
	}

	// cmy: 如果写至少成功了一个，返回true
	//      如果写到剩下最后一个了都没有写成功，返回false
	return (i==iIDBCount);
}

// cmy: 升级loader
int update_loader(bool dataLoaded)
{
	int iRet=0,iResult;
	int iIDBCount;
	UINT uiNeedIdSectorNum;
	PRINT_E("update loader\n");

	FW_ReIntForUpdate();//升级之前检查idblk数据是正确的

	// 从MISC分区中取出rk28loader(L).bin的内容，存放在g_pLoader
	PRINT_I("get loader\n");
	iResult = get_rk28boot(g_pLoader, dataLoaded);
	if( iResult )
	{
		PRINT_E("rk28boot Err:%d\n", iResult);
		iRet = -6;
		goto Exit_update;
	}

	PRINT_E("ver %x %x\n",internal_boot_bloader_ver,update_boot_bloader_ver);
	PRINT_I("SecPerBlock=%d\n", g_id_block_size);
	//**************1.创建状态表******************
	PRINT_I("create flash block map\n");
	if( !BuildFlashStateMap(0, &m_flashInfo) )
	{
		PRINT_E("failed1\n");
		iRet = -1;
		goto Exit_update;
	}

	//**************2.查找所有IDB******************
	PRINT_I("Search all id block...\n");
	iIDBCount = FindAllIDB();
	if ( iIDBCount<=0 )
	{
		PRINT_E("failed2\n");
		iRet = -2;
		goto Exit_update;
	}
	else if(iIDBCount == 1)
	{// 至少保证有一块idb是正常的
		//PRINT_E("Remain last one IDBlock!\n");
		iRet = -3;
		goto Exit_update;
	}

	//PRINT_I("ID BLOCK:\n");
	//for(i=0; i<iIDBCount; i++)
	//    PRINT_I("%d\n", g_IDBlockOffset[i]);

	//**************3.更新IDB******************
	PRINT_I("generic id block\n");
	if ( !GenericIDBData(g_pIDBlock, &uiNeedIdSectorNum) )
	{
		PRINT_E("failed3\n");
		iRet = -4;
		goto Exit_update;
	}

	//**************4.写入IDB******************
	PRINT_I("write id block\n");
	if( !WriteXIDBlock(g_IDBlockOffset, iIDBCount, g_pIDBlock, uiNeedIdSectorNum) )
		iRet = -5;
Exit_update:
	PRINT_I(">>> LEVEL update(%d)\n", iRet);
	return iRet;    
}
#else
int update_loader(void)
{
	PRINT_E("NOT SUPPORT!\n");
	return 0;
}
#endif

static int dispose_bootloader_cmd(struct bootloader_message *msg,
		const disk_partition_t *misc_part)
{
	int ret = 0;
	if(0 == strcmp(msg->command, "boot-recovery")) {
		// Recovery System
	} else if( 0 == strcmp(msg->command, "bootloader")
			|| 0 == strcmp(msg->command, "loader") ) // 新Loader才能支持"loader"命令
	{
		bool reboot;
		FW_ReIntForUpdate();
		if( execute_cmd(&gBootInfo, msg->recovery, &reboot) )
		{
			ret = -1;
		}

		{// 不管成功与否，将misc清0
			int i=0;
			memset(g_32secbuf, 0, 32*528);
			for(i=0; i<3; i++)
			{
				if(StorageWriteLba(misc_part->start+i*32,  (void*)g_32secbuf ,32,0) != 0 )
				{
					//PRINT_E("Clear misc failed!\n");
					//break;
				}
			}
			//if(i >=3)
			//	PRINT_I("Clear misc okay!\n");

			if(reboot)
			{
				PRINT_I("reboot\n");
				//DRVDelayMs(10);
				ISetLoaderFlag(SYS_LOADER_REBOOT_FLAG | BOOT_NORMAL);
				reset_cpu(0);
			}
		} 
	}
	else
	{
		PRINT_W("Unsupport cmd\n");
	}

	return ret;
}

void setup_space(uint32 begin_addr)
{
	uint32 next = 0;

	g_32secbuf = (uint8*)begin_addr;
	next += 32*528;
	g_cramfs_check_buf = (uint8*)begin_addr;
	g_pIDBlock = (uint8*)begin_addr;
	next = begin_addr + 2048*528;
	g_pLoader = (uint8*)next;
	next += 1024*1024;
	g_pReadBuf = (uint8*)next;
	next += MAX_WRITE_SECTOR*528;
	g_pFlashInfoData = (uint8*)next;
	next += 2048;
	if ((next - begin_addr) > CONFIG_RK_GLOBAL_BUFFER_SIZE) {
		printf("CONFIG_RK_GLOBAL_BUFFER_SIZE too small:%d < %lu\n",
				CONFIG_RK_GLOBAL_BUFFER_SIZE, (next - begin_addr));
		while(1);
	}
}

void SysLowFormatCheck(void)
{
	FW_SorageLowFormat();
}

#if 0
#define MaxFlashReadSize  16384  //8MB
int32 CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec)
{
	uint8 * pSdram = (uint8*)dest_addr;
	uint16 sec = 0;
	uint32 LBA = src_addr;
	uint32 remain_sec = total_sec;

	//  RkPrintf("Enter >> src_addr=0x%08X, dest_addr=0x%08X, total_sec=%d\n", src_addr, dest_addr, total_sec);

	//  RkPrintf("(0x%X->0x%X)  size: %d\n", src_addr, dest_addr, total_sec);

	while(remain_sec > 0)
	{
		sec = (remain_sec > MaxFlashReadSize) ? MaxFlashReadSize : remain_sec;
		if(StorageReadLba(LBA,(uint8*)pSdram, sec) != 0)
		{
			return -1;
		}
		remain_sec -= sec;
		LBA += sec;
		pSdram += sec*512;
	}

	//  RkPrintf("Leave\n");
	return 0;
}
#else
#define MaxFlashReadSize  32  //16KB
int32 CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, RK_BLK_SIZE * MaxFlashReadSize);
	uint8 * pSdram = (uint8*)dest_addr;
	uint16 sec = 0;
	uint32 LBA = src_addr;
	uint32 remain_sec = total_sec;

	while(remain_sec > 0)
	{
		sec = (remain_sec > MaxFlashReadSize) ? MaxFlashReadSize : remain_sec;
		if(StorageReadLba(LBA, (uint8*)buf, sec) != 0) {
			return -1;
		}
		memcpy(pSdram, buf, RK_BLK_SIZE * sec);
		remain_sec -= sec;
		LBA += sec;
		pSdram += sec * RK_BLK_SIZE;
	}

	return 0;
}
#endif

int CopyMemory2Flash(uint32 src_addr, uint32 dest_offset, int sectors)
{
	uint16 sec = 0;
	uint32 remain_sec = sectors;

	while(remain_sec > 0)
	{
		sec = (remain_sec>32)?32:remain_sec;

		if(StorageWriteLba(dest_offset, (void *)src_addr, sec, 0) != 0)
		{
			return -2;
		}

		remain_sec -= sec;
		src_addr += sec*512;
		dest_offset += sec;
	}

	return 0;
}

void fixInitrd(PBootInfo pboot_info, int ramdisk_addr, int ramdisk_sz)
{
#define MAX_BUF_SIZE 100
	char str[MAX_BUF_SIZE];
	char *cmd_line = strdup(pboot_info->cmd_line);
	char *s_initrd_start = NULL;
	char *s_initrd_end = NULL;
	int len = 0;

	if (!cmd_line)
		return;

	s_initrd_start = strstr(cmd_line, "initrd=");
	if (s_initrd_start) {
		len = strlen(cmd_line);
		s_initrd_end = strstr(s_initrd_start, " ");
		if (!s_initrd_end)
			*s_initrd_start = '\0';
		else {
			len = cmd_line + len - s_initrd_end;
			memcpy(s_initrd_start, s_initrd_end, len);
			*(s_initrd_start + len) = '\0';
		}
	}
	snprintf(str, sizeof(str), "initrd=0x%08X,0x%08X", ramdisk_addr, ramdisk_sz);

#ifndef CONFIG_OF_LIBFDT
	snprintf(pboot_info->cmd_line, sizeof(pboot_info->cmd_line),
			"%s %s", str, cmd_line);
#else
	snprintf(pboot_info->cmd_line, sizeof(pboot_info->cmd_line),
			"%s", cmd_line);
#endif
	free(cmd_line);
}

int execute_cmd(PBootInfo pboot_info, char* cmdlist, bool* reboot)
{
	char* cmd = cmdlist;

	*reboot = FALSE;
	while(*cmdlist)
	{
		if(*cmdlist=='\n') *cmdlist='\0';
		++cmdlist;
	}

	while(*cmd)
	{
		PRINT_I("bootloader cmd: %s\n", cmd);

		if( !strcmp(cmd, "update-bootloader") )// 脡媒录露 bootloader
		{
			PRINT_I("--- update bootloader ---\n");
			if( update_loader(false)==0 )
			{// cmy: 脡媒录露脥锚鲁脡潞贸脰脴脝么
				*reboot = TRUE;
			}
			else
			{// cmy: 脡媒录露脢搂掳脺
				return -1;
			}
		}
		else
			PRINT_I("Unsupport cmd: %s\n", cmd);

		cmd += strlen(cmd)+1;
	}
	return 0;
}

#define MISC_PAGES          3
#define MISC_COMMAND_PAGE   1
#define PAGE_SIZE           (16 * 1024)//16K
#define MISC_SIZE           (MISC_PAGES * PAGE_SIZE)//48K
#define MISC_COMMAND_OFFSET (MISC_COMMAND_PAGE * PAGE_SIZE / RK_BLK_SIZE)//32

int checkMisc(void)
{
	struct bootloader_message *bmsg = NULL;
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, DIV_ROUND_UP(sizeof(struct bootloader_message),
			RK_BLK_SIZE) * RK_BLK_SIZE);
	const disk_partition_t *ptn = get_disk_partition(MISC_NAME);
	if (!ptn) {
		printf("misc partition not found!\n");
		return false;
	}
	bmsg = (struct bootloader_message *)buf;
	if (StorageReadLba(ptn->start + MISC_COMMAND_OFFSET, buf, DIV_ROUND_UP(
					sizeof(struct bootloader_message), RK_BLK_SIZE)) != 0) {
		printf("failed to read misc\n");
		return false;
	}
	if(!strcmp(bmsg->command, "boot-recovery")) {
		printf("got recovery cmd from misc.\n");
		return true;
	} else if((!strcmp(bmsg->command, "bootloader")) ||
			(!strcmp(bmsg->command, "loader"))) {
		printf("got bootloader cmd from misc.\n");
		const disk_partition_t* misc_part = get_disk_partition(MISC_NAME);
		if (!misc_part) {
			printf("misc partition not found!\n");
			return -1;
		}
		dispose_bootloader_cmd(bmsg, misc_part);
	}
	return false;
}


int setBootloaderMsg(struct bootloader_message* bmsg)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, DIV_ROUND_UP(sizeof(struct bootloader_message),
			RK_BLK_SIZE) * RK_BLK_SIZE);
	memcpy(buf, bmsg, sizeof(struct bootloader_message));
	const disk_partition_t *ptn = get_disk_partition(MISC_NAME);
	if (!ptn) {
		printf("misc partition not found!\n");
		return -1;
	}

	return CopyMemory2Flash((uint32)&buf, ptn->start + MISC_COMMAND_OFFSET,
			DIV_ROUND_UP(sizeof(struct bootloader_message), RK_BLK_SIZE));
}

#define IDBLOCK_SN          3//the sector 3
#define IDBLOCK_SECTORS     1024
#define IDBLOCK_NUM         4
#define IDBLOCK_SIZE        512
#define SECTOR_OFFSET       528

extern uint16 g_IDBlockOffset[];


int get_idblk_data(void)
{
	uint32 index;
	int idbCount = FindAllIDB();
	uint8 *psrc, *pdst;

	if (idbCount <= 0) {
		printf("id block not found.\n");
		return false;
	}

	memset((void*)g_pIDBlock, 0, SECTOR_OFFSET * IDBLOCK_NUM);

	if (StorageReadPba(g_IDBlockOffset[0] * g_FlashInfo.BlockSize, 
				(void*)g_pIDBlock, IDBLOCK_NUM) != ERR_SUCCESS) {
		printf("read id block error.\n");
		return false;
	}

	pdst = (uint8 *)gIdDataBuf;
	psrc = (uint8 *)g_pIDBlock;
	for (index = 0; index < IDBLOCK_NUM; index++) {
		memcpy(pdst + IDBLOCK_SIZE * index, psrc + SECTOR_OFFSET * index, IDBLOCK_SIZE);
	}

#if 0
	int i = 0;
	for (i = 0; i < 512 * IDBLOCK_NUM; i ++) {
		printf("%02x ", pdst[i]);
		if ((i+1) % 16 == 0) {
			printf("\n");
		}
	}
	printf("\n");
#endif
	return true;
}


int getSn(char* buf)
{
	int size;
	Sector3Info *pSec3;

	pSec3 = (Sector3Info *)(gIdDataBuf + IDBLOCK_SIZE * IDBLOCK_SN);
	P_RC4((void *)pSec3, IDBLOCK_SIZE);

	size = pSec3->snSize;
	if (size <= 0 || size > SN_MAX_SIZE) {
		printf("empty serial no.\n");
		return false;
	}
	strncpy(buf, (char*)pSec3->sn, size);
	buf[size] = '\0';
	printf("sn:%s\n", buf);
	return true;
}

int secureCheck(struct fastboot_boot_img_hdr *hdr, int unlocked)
{
	rk_boot_img_hdr *boothdr = (rk_boot_img_hdr *)hdr;

	SecureBootCheckOK = 0;

	if (memcmp(hdr->magic, FASTBOOT_BOOT_MAGIC,
				FASTBOOT_BOOT_MAGIC_SIZE)) {
		goto end;
	}

	if(!unlocked && SecureBootEn &&
			boothdr->signTag == SECURE_BOOT_SIGN_TAG)
	{
		if(SecureBootSignCheck(boothdr->rsaHash, (uint8 *)boothdr->hdr.id,
					boothdr->signlen) == FTL_OK)
		{
			SecureBootCheckOK = 1;
		} else {
			printf("SecureBootSignCheck failed\n");
		}
	}

end:
	printf("SecureBootCheckOK:%d\n", (int)SecureBootCheckOK);
	if(SecureBootCheckOK == 0)
	{
		SecureBootDisable();
	}

#ifdef SECURE_BOOT_TEST
	SetSysData2Kernel(1);
#else
	SetSysData2Kernel(SecureBootCheckOK);
#endif
	return 0;
}


int eraseDrmKey(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, RK_BLK_SIZE);
	memset(buf, 0, RK_BLK_SIZE);
	StorageSysDataStore(1, buf);
	gDrmKeyInfo.publicKeyLen = 0;
	return 0;
}

#define FDT_PATH        "rk-kernel.dtb"
const char* get_fdt_name(void)
{
	if (!gBootInfo.fdt_name[0]) {
		return FDT_PATH;
	}
	return gBootInfo.fdt_name;
}
