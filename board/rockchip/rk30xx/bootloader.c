#include "config.h"

extern int execute_cmd(PBootInfo pboot_info, char* cmdlist, bool* reboot);

uint32 g_id_block_size = 1024;

#if 1
extern void P_RC4(unsigned char * buf, unsigned short len);
extern int32 CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec);
extern void DRVDelayMs(uint32 ms);

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
uint32 BuildNFBlockStateMap(uint8 ucFlashIndex, uint8 *NFBlockState, uint32 uiNFBlockLen)
{
	return TRUE;
}

/*************************** 生成指定Flash的所有块的状态表 ******************************/
uint32 BuildFlashStateMap(uint8 ucFlashIndex, FlashInfo *pFlash)
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
int FindSerialBlocks(uint8 *NFBlockState, int iNFBlockLen, int iBegin, int iLen)
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
int FindIDBlock(FlashInfo* pFlashInfo, int iStart,int *iPos)
{
    BYTE ucSpareData[4*528];
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
		iRet = StorageReadPba(i*g_id_block_size , ucSpareData, 4);
		
		if(ERR_SUCCESS != iRet)
		{
			continue;
		}
		
        if ( (*(unsigned int*)ucSpareData) == 0xfcdc8c3b ) 
		{
			*iPos = i;
			return 0;//找到idb
        }
		else
			continue;
	}
	return -1;							// new mp3
}

int FindAllIDB()
{
	int i,iRet,iIndex,iStart=0,iCount=0;
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
bool GenericIDBData(PBYTE pIDBlockData, UINT *needIdSectorNum)
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
	#ifdef RK_FLASH_BOOT_EN
	if( pSec1->usFlashDataOffset && pSec1->usFlashDataLen )
	{
    	hasFlashInfo = 1;
    	ReadFlashInfo(g_pFlashInfoData);
    }
    #endif

//cmy: 使用新的loader代码更新IDBlock的数据
    hdr = (RK28BOOT_HEAD*)g_pLoader;

    PRINT_I("update loader data\n");

// update page0
    // cmy: 更新FlashData及FlashBoot内容
    pSec0->usFlashDataSize = PAGEALIGN(BYTE2SECTOR(hdr->uiFlashDataLen))*4;
    pSec0->ucFlashBootSize = PAGEALIGN(BYTE2SECTOR(hdr->uiFlashBootLen))*4;
    for(i=0; i<pSec0->ucFlashBootSize; i++)
    {
    	ftl_memcpy( (void*)(pIDBlockData+SECTOR_OFFSET*(4+pSec0->usFlashDataSize+i)), (void*)(g_pLoader+hdr->uiFlashBootOffset+i*512), 512 );
    }
    for(i=0; i<pSec0->usFlashDataSize; i++)
    {
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

int get_rk28boot(uint8 * pLoader, bool dataLoaded)
{
    mtd_partition *misc_part = gBootInfo.cmd_mtd.parts+gBootInfo.index_misc;
    RK28BOOT_HEAD* hdr = (RK28BOOT_HEAD*)pLoader;
    int nBootSize = 0;

    if (!dataLoaded)
    {
        if(StorageReadLba(misc_part->offset+96, (void*)pLoader,4)!=0 )
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
        if( CopyFlash2Memory( (int32)pLoader, misc_part->offset+96, BYTE2SECTOR(nBootSize)) )
            return -3;
    }

    update_boot_bloader_ver = (INT2BCD(hdr->uiMajorVersion)<<8)|INT2BCD(hdr->uiMinorVersion);
    return 0;
}

//返回值：
// true  成功
// false 失败
bool WriteXIDBlock(USHORT *pSysBlockAddr, int iIDBCount, UCHAR *idBlockData, UINT uiIdSectorNum)
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
	int i=0;
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
int update_loader()
{
	PRINT_E("NOT SUPPORT!\n");
	return 0;
}
#endif

#ifdef USE_RECOVER
// 0         成功
// 其它值    失败
int dispose_bootloader_cmd(struct bootloader_message *msg, mtd_partition *misc_part)
{
    int ret = 0;
	if( 0 == strcmp(msg->command, "boot-recovery") )
		g_bootRecovery = TRUE;// 进入Recovery System
	else if( 0 == strcmp(msg->command, "bootloader")
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
				if(StorageWriteLba(misc_part->offset+i*32,  (void*)g_32secbuf ,32,0) != 0 )
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
                SoftReset();
			}
		} 
	}
	else
	{
		PRINT_W("Unsupport cmd\n");
	}
	
    return ret;
}
#endif 

