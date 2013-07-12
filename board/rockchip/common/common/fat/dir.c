/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    dir.C
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#define     IN_DIR
#include    "../../armlinux/config.h"
#include    "fatInclude.h"		//FAT头文件
#ifndef BOOT_ONLY
/*********************************************************************************************************
** 函数名称	:
** 功能描述	:建立目录信息
** 输　入	:扩展名关联文件
** 输　出	:文件数
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint16 BuildDirInfo(uint8 Drive, char* ExtName)
{
	uint16 TotalFiles=0;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);
	
	GotoRootDir(Drive, ExtName);
	while (1)
	{
		TotalFiles += pInfo->DirInfo[pInfo->DirDeep].TotalFile;
		GotoNextDir(Drive, ExtName);			//遍历所有子目录
		if (pInfo->DirDeep == 0)
			break;
	}
	return (TotalFiles);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:转到下一个目录
** 输　入	:扩展名关联文件
** 输　出	:无
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
void GotoNextDir(uint8 Drive, char* ExtName)
{
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	if (pInfo->DirInfo[pInfo->DirDeep].TotalSubDir == 0)	//该目录下没有目录即为叶结点,要找父目录或同级目录
	{
		while (1)
		{
			if (pInfo->DirDeep == 0)					//找到根目录了不能再往上找
				return;

			pInfo->PathClus=ChangeDir(Drive, 2);	//获取上一级目录开始簇号(..目录)
			pInfo->DirDeep--;						    //即指向上一级目录
			if(pInfo->DirInfo[pInfo->DirDeep].CurDirNum < pInfo->DirInfo[pInfo->DirDeep].TotalSubDir)
			{
				pInfo->DirInfo[pInfo->DirDeep].CurDirNum++;
				pInfo->DirDeep++;
				pInfo->PathClus = ChangeDir(Drive, pInfo->DirInfo[pInfo->DirDeep-1].CurDirNum);
				break;
			}
		}
	}
	else			//该目录下还有子目录,要找它的第一个子目录
	{
		if (pInfo->DirDeep == 0)
			pInfo->DirInfo[pInfo->DirDeep].CurDirNum=1;						//根目录的第一个目录号是1
		else
			pInfo->DirInfo[pInfo->DirDeep].CurDirNum=3;						//子目录的第一个目录号是3

		if (++pInfo->DirDeep < MAX_DIR_DEPTH-1)
			pInfo->PathClus = ChangeDir(Drive, pInfo->DirInfo[pInfo->DirDeep-1].CurDirNum);
		else
		{
			pInfo->DirDeep=MAX_DIR_DEPTH-1;	//边界限制
			pInfo->PathClus = ChangeDir(Drive, pInfo->DirInfo[pInfo->DirDeep-1].CurDirNum);
		}
	}

	pInfo->DirInfo[pInfo->DirDeep].TotalFile=GetTotalFiles(".", ExtName);	//add by lxs @2005.02.24
	if (pInfo->DirDeep < MAX_DIR_DEPTH-1)
		pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=GetTotalSubDir(".");		//获取该目录下的子目录数
	else
		pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=0;						//标记为叶结点
	if (pInfo->DirInfo[pInfo->DirDeep].TotalSubDir == 2 && pInfo->DirDeep>0)	//子目录下有2个子目录"."和".."
		pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=0;
	
	if (pInfo->DirInfo[pInfo->DirDeep].TotalFile == 0)						//若没有文件再查找下一个目录
		GotoNextDir(Drive, ExtName);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:转到前一个目录
** 输　入	:扩展名关联文件
** 输　出	:无
** 全局变量  :
** 调用模块	:无
********************************************************************************************************/
void GotoPrevDir(uint8 Drive, char* ExtName)
{
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	if (pInfo->DirDeep == 0)				//当前是根目录要转到最后一个目录
		GotoLastDir(Drive, ExtName);
	else
	{
		pInfo->PathClus = ChangeDir(Drive, 2);	//获取上一级目录开始簇号(..目录)
		pInfo->DirDeep--;
		if (pInfo->DirDeep == 0)
		{
			if (pInfo->DirInfo[pInfo->DirDeep].CurDirNum != 1)
			{
				pInfo->DirInfo[pInfo->DirDeep].CurDirNum--;
				pInfo->PathClus = ChangeDir(Drive, pInfo->DirInfo[pInfo->DirDeep].CurDirNum);
				pInfo->DirDeep++;
				GotoLastDir(Drive, ExtName);
			}
			else
				return;				//已经搜到根目录了
		}
		else
		{
			if (pInfo->DirInfo[pInfo->DirDeep].CurDirNum != 3)
			{
				pInfo->DirInfo[pInfo->DirDeep].CurDirNum--;
				pInfo->PathClus = ChangeDir(Drive, pInfo->DirInfo[pInfo->DirDeep].CurDirNum);
				pInfo->DirDeep++;
				GotoLastDir(Drive, ExtName);
			}
		}
	}

	pInfo->DirInfo[pInfo->DirDeep].TotalFile=GetTotalFiles(".", ExtName);	//add by lxs @2005.02.24
	if (pInfo->DirDeep < MAX_DIR_DEPTH-1)
		pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=GetTotalSubDir(".");		//获取该目录下的子目录数
	else
		pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=0;						//标记为叶结点

	if (pInfo->DirInfo[pInfo->DirDeep].TotalSubDir == 2 && pInfo->DirDeep>0)	//子目录下有2个子目录"."和".."
		pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=0;
	if (pInfo->DirInfo[pInfo->DirDeep].TotalFile == 0)						//若没有文件再查找下一个目录
		GotoPrevDir(Drive, ExtName);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:转到最后一个目录
** 输　入	:扩展名关联文件
** 输　出	:无
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
void GotoLastDir(uint8 Drive, char* ExtName)
{
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	if(pInfo->DirDeep == (MAX_DIR_DEPTH-1))
	{
		return;
	}
	//go last dir
	while(1)
	{
		pInfo->DirInfo[pInfo->DirDeep].TotalSubDir = GetTotalSubDir(".");
		if(pInfo->DirInfo[pInfo->DirDeep].TotalSubDir==0 || (pInfo->DirInfo[pInfo->DirDeep].TotalSubDir<3 && pInfo->DirDeep>0))
		{
			break;
		}
		else
		{
			pInfo->DirInfo[pInfo->DirDeep].CurDirNum = pInfo->DirInfo[pInfo->DirDeep].TotalSubDir;
			pInfo->PathClus = ChangeDir(Drive, pInfo->DirInfo[pInfo->DirDeep].CurDirNum);
			pInfo->DirDeep++;	
	
			if(pInfo->DirDeep == (MAX_DIR_DEPTH-1))
			{
				break;
			}
		}
	}
	pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=0;
	pInfo->DirInfo[pInfo->DirDeep].TotalFile=GetTotalFiles(".", ExtName);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:转到根目录
** 输　入	:扩展名关联文件
** 输　出	:无
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
void GotoRootDir(uint8 Drive, char* ExtName)
{
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	pInfo->DirDeep=0;						//目录深度0
	pInfo->PathClus=pInfo->RootClus;	//目录簇根目录
	pInfo->DirInfo[pInfo->DirDeep].TotalSubDir=GetTotalSubDir(".");
	pInfo->DirInfo[pInfo->DirDeep].TotalFile=GetTotalFiles(".", ExtName);
}


/*********************************************************************************************************
** 函数名称	:MakeDir
** 功能描述	:创建目录
** 输　入	:Path:路径, DirFileName是8.3格式目录名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 MakeDir(char *Path, char *DirFileName)
{
	uint8 Rt;
	uint8 i;
	uint32 ClusIndex, Temp1;
	FDT temp;
    uint8 Drive;
    pDRIVE_INFO pInfo;
    
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif

	StrUprCase(Path);
    Drive=GetDrive(Path);
    pInfo=GetDriveInfo(Drive);
	ClusIndex = GetDirClusIndex(Path);
	Rt=PATH_NOT_FIND;
	if (ClusIndex != BAD_CLUS)
	{
		for (i=0; i<11; i++)		//目录项填空格
			temp.Name[i]=' ';
		for (i=0; i<11; i++)
		{
			if (*DirFileName == '\0')	//到路径结束
			{
				break;
			}
			temp.Name[i]=*DirFileName++;
		}

		/* FDT是否存在 */
		Rt = FDTIsLie(Drive, ClusIndex, temp.Name);
		if (Rt == NOT_FIND_FDT)
		{
			/* 不存在 */
			Temp1 = FATAddClus(Drive, 0);               	/* 获取目录所需磁盘空间 */
            CacheWriteBackAll();
			Rt=DISK_FULL;					/* 没有空闲空间 */
			if ((Temp1 > EMPTY_CLUS_1) && (Temp1 < BAD_CLUS))
			{
				ClearClus(Drive, Temp1);            		/* 清空簇 */
			        /* 设置FDT属性 */
				temp.Attr = ATTR_DIRECTORY;
				temp.FileSize = 0;
				temp.NTRes = 0;
				temp.CrtTimeTenth = 0;
				temp.CrtTime = 0;
				temp.CrtDate = 0;
				temp.LstAccDate = 0;
				temp.WrtTime = 0;
				temp.WrtDate = 0;
				temp.FstClusLO = Temp1 & 0xffff;
				temp.FstClusHI = Temp1 / 0x10000;
				Rt = AddFDT(Drive, ClusIndex, &temp);       /* 增加目录项 */
				if (Rt == RETURN_OK)
				{
					/* 建立'.'目录 */
					temp.Name[0] = '.';
					for (i=1; i < 11; i++)
					{
						temp.Name[i] = ' ';
					}
					AddFDT(Drive, Temp1, &temp);

					/* 建立'..'目录 */
					temp.Name[1] = '.';
					if (ClusIndex == pInfo->RootClus)
						ClusIndex=0;
					temp.FstClusLO = ClusIndex & 0xffff;
					temp.FstClusHI = ClusIndex / 0x10000;
					Rt = AddFDT(Drive, Temp1, &temp);
				}
				else
				{
					FATDelClusChain(Drive, Temp1);
				}
				Rt=RETURN_OK;
			}
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:改变目录，转到当前目录下的子目录索引
** 输　入	:SubDirIndex
** 输　出	:指定子目录索引的簇号
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint32 ChangeDir(uint8 Drive, uint16 SubDirIndex)
{
	FDT Rt;
	uint32 cluster;
	uint32 index;
	uint8 *buf;
	uint8 offset;
	uint8 i;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

	index=0;
	cluster=0;
	while (1)
	{
		if (RETURN_OK != GetFDTInfo(&Rt, Drive, pInfo->PathClus, index++))
			break;

		if (Rt.Name[0] == 0x00)			//找到空的意为结束
			break;
		
		if (Rt.Name[0] == FILE_DELETED)	//已删除
			continue;
		
		for (i=0; i<MAX_FILENAME_LEN; i++)
			pInfo->DirInfo[pInfo->DirDeep].LongDirName[i]='\0';

		while (Rt.Attr==ATTR_LFN_ENTRY)			//长文件名项要找到短文件名
		{
			buf=(uint8 *)&Rt;
			offset=13 * ((buf[0] & LFN_SEQ_MASK) - 1);
			if ((buf[0] & LFN_SEQ_MASK) <= MAX_LFN_ENTRIES)   
			{/* 长文件名最多目录项数*/
				for (i = 0; i < 10; i++)
					pInfo->DirInfo[pInfo->DirDeep].LongDirName[i/2+offset] |= buf[i+1] << (i % 2)*8;
				for (i = 0; i < 6; i++)
					pInfo->DirInfo[pInfo->DirDeep].LongDirName[i+5+offset]=buf[i+14];
				for (i = 0; i < 2; i++)
					pInfo->DirInfo[pInfo->DirDeep].LongDirName[i+11+offset]=buf[i+21];
			}
			
			GetFDTInfo(&Rt, Drive, pInfo->PathClus, index++);
		}
		if ((Rt.Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY)
		{
			if (--SubDirIndex == 0)
			{
				cluster=Rt.FstClusHI;
				cluster <<= 16;
				cluster |= Rt.FstClusLO;
				for (i=0; i<11; i++)
				{
					pInfo->DirInfo[pInfo->DirDeep].DirName[i]=Rt.Name[i];
				}
				if (pInfo->DirInfo[pInfo->DirDeep].LongDirName[0] == '\0')
				{
					for (i=0; i<8; i++)
					{
						pInfo->DirInfo[pInfo->DirDeep].LongDirName[i]=Rt.Name[i];
						if (pInfo->DirInfo[pInfo->DirDeep].LongDirName[i] == ' ')
							break;
					}
					if (Rt.Name[8] != ' ')
					{
						pInfo->DirInfo[pInfo->DirDeep].LongDirName[i++] = '.';				//追加扩展名
						pInfo->DirInfo[pInfo->DirDeep].LongDirName[i++] = Rt.Name[8];
						pInfo->DirInfo[pInfo->DirDeep].LongDirName[i++] = Rt.Name[9];
						pInfo->DirInfo[pInfo->DirDeep].LongDirName[i++] = Rt.Name[10];
					}
					pInfo->DirInfo[pInfo->DirDeep].LongDirName[i] = '\0';
				}
				break;
			}
		}
	}
	return (cluster);
}


/*********************************************************************************************************
** 函数名称	:GetTotalSubDir
** 功能描述	:获取总子目录数
** 输　入	:路径Path
** 输　出	:子目录数
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint16 GetTotalSubDir(char *Path)
{
	FDT Rt;
	uint32 index;
	uint32 DirClus;
	uint16 TotSubDir=0;

	DirClus=GetDirClusIndex(Path);
	if (DirClus == BAD_CLUS)
		return (0);
	
	for (index=0; ; index++)
	{
		if (RETURN_OK != GetFDTInfo(&Rt, GetDrive(Path), DirClus, index))
			break;
		
		if (Rt.Name[0] == FILE_NOT_EXIST)	//找到空的意为结束
			break;
		
		if (Rt.Name[0] == FILE_DELETED)	//已删除
			continue;
		
		if ((Rt.Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY)
			TotSubDir++;
	}
	return (TotSubDir);
}
#endif

/*********************************************************************************************************
** 函数名称	: GetDirClusIndex
** 功能描述	: 获取指定目录开始簇号
** 输　入	: Path:路径名(不包括文件名)
** 输　出	: 开始簇号
** 全局变量	: 
** 调用模块	: 
** 说	 明		: 最后的路径不能以'\'结束, 支持1~11个路径名
********************************************************************************************************/
uint32 GetDirClusIndex(char *Path)
{
	uint8 i;
	uint32 DirClusIndex;
	FDT temp;
	char PathName[12];
    uint8 Drive;
    pDRIVE_INFO pInfo;

	DirClusIndex = BAD_CLUS;
	if (Path != NULL)		//null pointer
	{
//***********************************************************************
//支持盘符如A:
//***********************************************************************
		StrUprCase(Path);
        Drive=GetDrive(Path);
        pInfo=GetDriveInfo(Drive);
		if (Path[1] == ':')
		{
			Path += 2;
		}
		
		DirClusIndex = pInfo->RootClus;	//根目录
//***********************************************************************
//A:TEMP、TEMP和.\TEMP都是指当前目录下的TEMP子目录
//***********************************************************************
		if (Path[0] != '\\')			//* 不是目录分隔符号,表明起点是当前路径
		{
			DirClusIndex = pInfo->PathClus;
		}
		else
		{
			Path++;
		}
		
		if (Path[0] == '.')             // '\.'表明起点是当前路径
		{
			DirClusIndex = pInfo->PathClus;
			if (Path[1] == '\0' || Path[1] == '\\')		//case "." or ".\"
			{
				Path++;
			}
		}
#if 0	//remove by lxs for filename include '\'
		if (Path[0] == '\\')
		{
			Path++;
		}
#endif		
//***********************************************************************
//***********************************************************************
		while (Path[0] != '\0')
		{
			if (Path[0] == ' ')			//首个字符不允许为空格
			{
				DirClusIndex = BAD_CLUS;
				break;
			}

			for (i=0; i<11; i++)		//目录项填空格
				PathName[i]=' ';
			for (i=0; i<12; i++)
			{
				#if 0	//remove by lxs for filename include '\'
				if (*Path == '\\')		//到目录分隔符
				{
					Path++;
					break;
				}
				#endif
				if (*Path == '\0')		//到路径结束
				{
					break;
				}
				PathName[i]=*Path++;
			}
			
			if (FindFDTInfo(&temp, Drive, DirClusIndex, PathName) != RETURN_OK)	//获取FDT信息
			{
				DirClusIndex = BAD_CLUS;
				break;
			}

			if ((temp.Attr & ATTR_DIRECTORY) == 0)	//FDT是否是目录
			{
				DirClusIndex = BAD_CLUS;
				break;
			}
			
			DirClusIndex = ((uint32)(temp.FstClusHI) << 16) + temp.FstClusLO;
		}
	}
	return (DirClusIndex);
}


/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/

