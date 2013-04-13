/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    FDT.C
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#define     IN_FDT

#include    "../../armlinux/config.h"
#include    "fatInclude.h"		//FAT头文件


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:读取FDT信息
** 输　入	:Rt：存储返回信息的指针
**        	 SecIndex：扇区号
**         	 ByteIndex：偏移量
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:无
** 调用模块	:ReadSecs
********************************************************************************************************/
uint8 ReadFDTInfo(FDT *Rt, uint8 Drive, uint32 SecIndex, uint16 ByteIndex)
{
	uint8 status;
    pCACHE cache;

    status=NOT_FIND_FDT;
    if (OK == ReadCache(Drive, SecIndex, &cache, CACHE_FDT))
	{
        ftl_memcpy((uint8*)Rt, cache->Buf+ByteIndex, sizeof(FDT));
	    status=RETURN_OK;
	}
	return (status);
}
#ifndef BOOT_ONLY
/*********************************************************************************************************
** 函数名称	:
** 功能描述	:写FDT信息
** 输　入	:SecIndex：扇区号
**        	 ByteIndex：偏移量
**        	 FDT *FDTData:数据
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:无
** 调用模块	:ReadSecs
********************************************************************************************************/
uint8 WriteFDTInfo(FDT *FDTData, uint8 Drive, uint32 SecIndex, uint16 ByteIndex)
{
	uint8 status;
    pCACHE cache;

    ReadCache(Drive, SecIndex, &cache, CACHE_FDT);
    cache->Flag=1;
    cache->Drive=Drive;
    ftl_memcpy(cache->Buf+ByteIndex, (uint8*)FDTData, sizeof(FDT));
    status=NOT_FIND_FDT;
    if (OK == CacheWriteBack(cache))
    {
        status=RETURN_OK;
    }
    return (status);
}
#endif

/*********************************************************************************************************
** 函数名称	:
** 功能描述	:获取根目录指定文件(目录)信息
** 输　入	:Rt：存储返回信息的指针
**        	 Index：文件(目录)在FDT中的位置
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 GetRootFDTInfo(FDT *Rt, uint8 Drive, uint32 Index)
{
	uint16 ByteIndex;
	uint32 SecIndex;
	uint8 temp;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);
    
	temp = NOT_FAT_DISK;
	Index = Index << 5;
	if (pInfo->FATType == FAT12 || pInfo->FATType == FAT16)
	{
		temp = FDT_OVER;
		if (Index < (pInfo->RootDirSectors << 9))
		{
			ByteIndex = Index & (pInfo->BytsPerSec-1);
          	SecIndex = (Index >> 9)+(pInfo->FirstDataSector-pInfo->RootDirSectors);
          	temp = ReadFDTInfo(Rt, Drive, SecIndex, ByteIndex);
    	}
	}
	return (temp);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:获取指定目录指定文件(目录)信息
** 输　入	:Rt：存储返回信息的指针
**        	 ClusIndex：目录首簇号
**        	 Index：文件(目录)在FDT中的位置
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 GetFDTInfo(FDT *Rt, uint8 Drive, uint32 ClusIndex, uint32 Index)
{
	uint16 ByteIndex;
	uint16 ClusCnt;
	uint32 SecIndex, i;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	if (ClusIndex == EMPTY_CLUS)
	{
		if (pInfo->FATType == FAT32)
		{
			ClusIndex = pInfo->RootClus;
		}
		else
		{
			return (GetRootFDTInfo(Rt, Drive, Index));
		}
	}

	if (pInfo->FATType == FAT12 || pInfo->FATType == FAT16 || pInfo->FATType == FAT32)
	{
		if (ClusIndex != pInfo->FdtRef.DirClus)
		{
			pInfo->FdtRef.DirClus=ClusIndex;
			pInfo->FdtRef.CurClus=ClusIndex;
			pInfo->FdtRef.Cnt=0;
		}
		Index = Index << 5;
		ByteIndex = Index & (pInfo->BytsPerSec-1);
		SecIndex=Index >> 9;
		ClusCnt = SecIndex >> pInfo->LogSecPerClus;
		if (ClusCnt < pInfo->FdtRef.Cnt)
		{
			pInfo->FdtRef.Cnt=0;
			pInfo->FdtRef.CurClus=ClusIndex;
		}
		else
		{
			SecIndex-=pInfo->FdtRef.Cnt << pInfo->LogSecPerClus;
		}
		/* 计算目录项所在扇区 */
		i = pInfo->SecPerClus;
		while(SecIndex >= i)
		{
			pInfo->FdtRef.CurClus = FATGetNextClus(Drive, pInfo->FdtRef.CurClus, 1);
			pInfo->FdtRef.Cnt++;
			if (pInfo->FdtRef.CurClus <= EMPTY_CLUS_1 || pInfo->FdtRef.CurClus >= BAD_CLUS) 
			{
				return (FDT_OVER);
			}
			SecIndex -= i;
		}
		SecIndex = ((pInfo->FdtRef.CurClus - 2) << pInfo->LogSecPerClus) + SecIndex + pInfo->FirstDataSector;
		return (ReadFDTInfo(Rt, Drive, SecIndex, ByteIndex));
	}
	return (NOT_FAT_DISK);
}

#ifndef BOOT_ONLY
/*********************************************************************************************************
** 函数名称	:SetRootFDTInfo
** 功能描述	:设置根目录指定文件(目录)信息
** 输　入	:FDTData：要写入的信息
**        	 Index：文件(目录)在FDT中的位置
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 SetRootFDTInfo(uint8 Drive, uint32 Index, FDT *FDTData)
{
	uint16 ByteIndex;
	uint32 SecIndex;
	uint8 Rt;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);
    
	Rt = NOT_FIND_DISK;
	Index = Index << 5;
	if (pInfo->FATType == FAT12 || pInfo->FATType == FAT16)
	{
		Rt = FDT_OVER;
		if (Index < (pInfo->RootDirSectors << 9))
		{
			ByteIndex = Index & (pInfo->BytsPerSec-1);
			SecIndex = (Index >> 9) + (pInfo->FirstDataSector-pInfo->RootDirSectors);
			Rt = WriteFDTInfo(FDTData, Drive, SecIndex, ByteIndex);
		}
	}
	return (Rt);
}
#endif
/*********************************************************************************************************
** 函数名称	:
** 功能描述	:在指定目录查找指定文件(目录)
** 输　入	:Rt：存储返回信息的指针
**        			 ClusIndex：目录首簇号
**        			 FileName：文件(目录)名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:无
** 调用模块	:
********************************************************************************************************/
uint8 FindFDTInfo(FDT *Rt, uint8 Drive, uint32 ClusIndex, char *FileName)
{
	uint32 i;
	uint8 temp, j;
    
	i = 0;
	if (FileName[0] == FILE_DELETED)
	{
		FileName[0] = ESC_FDT;
	}
	while (1)
	{
		temp = GetFDTInfo(Rt, Drive, ClusIndex, i);		//返回RETURN_OK\NOT_FAT_DISK\FDT_OVER
		if (temp != RETURN_OK)
		{
			break;
		}
		if (Rt->Name[0] == FILE_NOT_EXIST)
		{
			temp = NOT_FIND_FDT;
			break;
		}
		if ((Rt->Attr & ATTR_VOLUME_ID) == 0)
		{
			for (j=0; j<11; j++)
				if (FileName[j] != Rt->Name[j])
					break;
			if (j==11)
			{
				temp = RETURN_OK;
				break;
			}
		}
		i++;
	}
	if (FileName[0] == ESC_FDT)
	{
		FileName[0] = FILE_DELETED;
	}
	return (temp);
}


#ifndef BOOT_ONLY
/*********************************************************************************************************
** 函数名称	:
** 功能描述	:设置指定目录指定文件(目录)信息
** 输　入	:FDTData：要写入的信息
**        	 ClusIndex：目录首簇号
**        	 Index：文件(目录)在FDT中的位置
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 SetFDTInfo(uint8 Drive, uint32 ClusIndex, uint32 Index, FDT *FDTData)
{
	uint16 ByteIndex;
	uint32 SecIndex;
	uint8 i;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);
    
	if (ClusIndex == EMPTY_CLUS)
	{
		if (pInfo->FATType == FAT32)
		{
			ClusIndex = pInfo->RootClus;
		}
		else
		{
			return (SetRootFDTInfo(Drive, Index, FDTData));
		}
	}

	if (pInfo->FATType == FAT12 || pInfo->FATType == FAT16 || pInfo->FATType == FAT32)
	{
		Index = Index << 5;
		ByteIndex = Index & (pInfo->BytsPerSec-1);
		SecIndex = Index >> 9;	/* 计算目录项所在偏移扇区 */
		i = pInfo->SecPerClus;
		while(SecIndex >= i)
		{
			ClusIndex = FATGetNextClus(Drive, ClusIndex, 1);
			if (ClusIndex <= EMPTY_CLUS_1 ||ClusIndex >= BAD_CLUS) 
			{
				return (FDT_OVER);
			}
			SecIndex -= i;
		}
		SecIndex = ((ClusIndex - 2) << pInfo->LogSecPerClus) + SecIndex + pInfo->FirstDataSector;
		return (WriteFDTInfo(FDTData, Drive, SecIndex, ByteIndex));
	}
	return (NOT_FAT_DISK);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:在指定目录查增加指定文件(目录)
** 输　入	:ClusIndex：目录首簇号
**        	 FDTData：文件(目录)名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 AddFDT(uint8 Drive, uint32 ClusIndex, FDT *FDTData)
{
	uint32 i;
	FDT TempFDT;
	uint8 temp;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	if (ClusIndex == EMPTY_CLUS)
	{
		if (pInfo->FATType == FAT32)
		{
			ClusIndex = pInfo->RootClus;
		}
	}

	temp = FindFDTInfo(&TempFDT, Drive, ClusIndex, FDTData->Name);		//NOT_FIND_FDT\RETURN_OK
	if (temp == RETURN_OK)
	{
		return (FDT_EXISTS);
	}

	if (temp != NOT_FIND_FDT && temp != FDT_OVER)		//NOT_FAT_DISK
	{
		return (temp);
	}

	if (FDTData->Name[0] == FILE_DELETED)
	{
		FDTData->Name[0] = ESC_FDT;
	}

	i = 0;
	temp = RETURN_OK;
	while (temp == RETURN_OK)
	{
		temp = GetFDTInfo(&TempFDT, Drive, ClusIndex, i);
		if (temp == RETURN_OK)
		{
			if (TempFDT.Name[0] == FILE_DELETED || TempFDT.Name[0] == FILE_NOT_EXIST)
			{
				temp = SetFDTInfo(Drive, ClusIndex, i, FDTData);
				break;
			}
		}
		i++;
	}
	
	if (temp == FDT_OVER && ClusIndex != EMPTY_CLUS)	//当前目录项簇已满,需增加一个簇
	{
		i = FATAddClus(Drive, ClusIndex);
        CacheWriteBackAll();
		temp = DISK_FULL;
		if (i != BAD_CLUS)
		{
			ClearClus(Drive, i);
			temp = SetFDTInfo(Drive, i, 0, FDTData);
		}
	}
	
	if (FDTData->Name[0] == ESC_FDT)
	{
		FDTData->Name[0] = FILE_DELETED;
	}
	return (temp);
}



/*********************************************************************************************************
** 函数名称	:
** 功能描述	:在指定目录删除指定文件(目录)
** 输　入	:ClusIndex：目录首簇号
**        	 FileName：文件(目录)名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:无
** 调用模块	:
********************************************************************************************************/
uint8 DelFDT(uint8 Drive, uint32 ClusIndex, char *FileName)
{
	uint32 i;
	FDT TempFDT;
	uint8 temp, j;
    
	i = 0;
	if (FileName[0] == FILE_DELETED)
	{
		FileName[0] = ESC_FDT;
	}
	while (1)
	{
		temp = GetFDTInfo(&TempFDT, Drive, ClusIndex, i);
		if (temp != RETURN_OK)
		{
			break;
		}
            
		if (TempFDT.Name[0] == FILE_NOT_EXIST)
		{
			temp = NOT_FIND_FDT;
			break;
		}
//		if ((TempFDT.Attr & ATTR_VOLUME_ID) == 0)		//卷标不能删除
		{
			for (j=0; j<11; j++)
				if (FileName[j] != TempFDT.Name[j])
					break;
			if (j==11)
			{
                //删除长短目录项
				do
				{
					TempFDT.Name[0] = FILE_DELETED;
					temp = SetFDTInfo(Drive, ClusIndex, i, &TempFDT);
					if (RETURN_OK != GetFDTInfo(&TempFDT, Drive, ClusIndex, --i))
						break;
				}while (TempFDT.Attr==ATTR_LFN_ENTRY);			//长文件名项要找到短文件名
				break;
			}
		}
		i++;
	}
	if (FileName[0] == ESC_FDT)
	{
		FileName[0] = FILE_DELETED;
	}
	return (temp);
}

/*********************************************************************************************************
** 函数名称	:
** 功能描述	:改变指定目录指定文件（目录）的属性
** 输　入	:ClusIndex：目录首簇号
**        	 FileName：文件（目录）名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:无
** 调用模块	:
********************************************************************************************************/
uint8 ChangeFDT(uint8 Drive, uint32 ClusIndex, FDT *FDTData)
{
	uint32 i;
	uint8 temp, j;
	FDT TempFDT;

	i = 0;
	if (FDTData->Name[0] == FILE_DELETED)
	{
		FDTData->Name[0] = ESC_FDT;
	}
	while (1)
	{
		temp = GetFDTInfo(&TempFDT, Drive, ClusIndex, i);
		if (temp != RETURN_OK)
		{
			break;
		}
            
		if (TempFDT.Name[0] == FILE_NOT_EXIST)
		{
			temp = NOT_FIND_FDT;
			break;
		}
		if ((TempFDT.Attr & ATTR_VOLUME_ID) == 0)
		{
			for (j=0; j<11; j++)
				if (FDTData->Name[j] != TempFDT.Name[j])
					break;
			if (j==11)
			{
				temp = SetFDTInfo(Drive, ClusIndex, i, FDTData);
				break;
			}
		}
		i++;
	}
	if (FDTData->Name[0] == ESC_FDT)
	{
		FDTData->Name[0] = FILE_DELETED;
	}
	return (temp);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:在指定目录查看指定文件(目录)是否存在
** 输　入	:ClusIndex：目录首簇号
**        	 FileName：文件(目录)名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:无
** 调用模块	:
********************************************************************************************************/
uint8 FDTIsLie(uint8 Drive, uint32 ClusIndex, char *FileName)
{
	uint32 i;
	uint8 temp, j;
	FDT temp1;
    
	i = 0;
	if (FileName[0] == FILE_DELETED)
	{
		FileName[0] = ESC_FDT;
	}
	while (1)
	{
		temp = GetFDTInfo(&temp1, Drive, ClusIndex, i);
		if (temp == FDT_OVER)
		{
			temp = NOT_FIND_FDT;
			break;
		}

		if (temp != RETURN_OK)
		{
			break;
		}

		if (temp1.Name[0] == FILE_NOT_EXIST)
		{
			temp = NOT_FIND_FDT;
			break;
		}
		
		if ((temp1.Attr & ATTR_VOLUME_ID) == 0)
		{
			for (j=0; j<11; j++)
				if (FileName[j] != temp1.Name[j])
					break;
			if (j==11)
			{
				temp = FDT_EXISTS;
				break;
			}
		}
		i++;
	}
	if (FileName[0] == ESC_FDT)
	{
		FileName[0] = FILE_DELETED;
	}
	return (temp);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:将指定簇所有数据清零
** 输　入	:Path:路径
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 ClearClus(uint8 Drive, uint32 Index)
{
    uint8 buf[512];
    uint32 i;
	uint32 SecIndex;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	if (Index < (pInfo->TotClus+2))
	{
        memset(buf, 0x00, 512);
		SecIndex = ((Index - 2) << pInfo->LogSecPerClus) + pInfo->FirstDataSector;
        for (i=0; i<pInfo->SecPerClus; i++)
            WriteSecs(Drive, SecIndex+i, buf, 1);
		return (RETURN_OK);
	}
	else
	{
		return (CLUSTER_NOT_IN_DISK);
	}
}
#endif
/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/

