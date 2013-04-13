/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    File.C
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#define     IN_FILE
#include    "../../armlinux/config.h"
#include    "fatInclude.h"		//FAT头文件


/*********************************************************************************************************
** 函数名称	:FileInit
** 功能描述	:初始化文件系统
** 输　入	:无
** 输　出	:无
** 全局变量	:无
** 调用模块	:无
********************************************************************************************************/
void FileInit(void)
{
	uint16 i;

    CurDrive=0;     //默认系统盘
	for (i=0; i<MAX_OPEN_FILES; i++)
	{
		FileInfo[i].Flags = 0;
	}
	CacheInit();

    //3磁盘初始化
	for (i=0; i<MAX_DRIVE; i++)
	{
		Demount(i);
		Mount(i);
	}
}


/*********************************************************************************************************
** 函数名称	:FileOpen
** 功能描述	:以指定方式打开文件
** 输　入	:DirClus:路径的目录簇, DirFileName:用户使用的文件名, Type:打开方式
** 输　出	:Not_Open_FILE为不能打开,其它为打开文件的句柄
** 全局变量: 
** 调用模块: 无
********************************************************************************************************/
pFILE FileOpen(uint8 Drive, uint32 DirClus, char *DirFileName, char *Type)
{
	uint8 i;
	pFILE fp, Rt;
	FDT FileFDT;
    
	Rt=NULL;
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	StrUprCase(Type);
	StrUprCase(DirFileName);
	/* 查找空闲文件登记项 */
	for (i=0; i<MAX_OPEN_FILES; i++)
	{
		if (FileInfo[i].Flags == 0)
		{
			break;
		}
	}
    
	if (i < MAX_OPEN_FILES && DirClus < BAD_CLUS)
	{
		fp = &FileInfo[i];
		fp->DirClus=DirClus;
        fp->Drive=Drive;

		if (RETURN_OK==FindFDTInfo(&FileFDT, fp->Drive, fp->DirClus, DirFileName))
		{
		    
			if ((FileFDT.Attr & ATTR_DIRECTORY) == 0)	//是文件
			{
                ftl_memcpy(fp->Name, DirFileName, 11);
				fp->FileSize = FileFDT.FileSize;
				fp->FstClus = FileFDT.FstClusLO | ((uint32)FileFDT.FstClusHI << 16);
				fp->Clus = fp->FstClus;
				fp->Offset = 0;
				fp->RefClus = fp->Clus;
				fp->RefOffset = 0;
				fp->Flags = FILE_FLAGS_READ;
                Rt=fp;
			}
		}
	}	

#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FileClose
** 功能描述	:关闭指定文件
** 输　入	:文件句柄
** 输　出	:RETURN_OK:成功
** 其它参考  fat.h中关于返回值的说明 
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint8 FileClose(pFILE fp)
{
	uint8 Rt;
    
	Rt = PARAMETER_ERR;
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1])
	{
		Rt = RETURN_OK;
#ifndef BOOT_ONLY
		if ((fp->Flags & FILE_FLAGS_WRITE) == FILE_FLAGS_WRITE)
		{
            uint8 i;
        	FDT FileFDT;
            
			FLASH_PROTECT_OFF();
			Rt = FindFDTInfo(&FileFDT, fp->Drive, fp->DirClus, fp->Name);
			if (Rt == RETURN_OK)
			{
				FileFDT.FileSize = fp->FileSize;
				if (FileFDT.FstClusLO == 0 && FileFDT.FstClusHI == 0)	//是新建文件的情况
				{
					FileFDT.FstClusLO = fp->FstClus & 0xffff;
					FileFDT.FstClusHI = (fp->FstClus >> 16) & 0xffff;
				}
				ChangeFDT(fp->Drive, fp->DirClus, &FileFDT);
#if 1
            	for (i=0; i<MAX_OPEN_FILES; i++)
            	{
            		if ((FileInfo[i].Flags & FILE_FLAGS_WRITE)==FILE_FLAGS_WRITE && fp!=&FileInfo[i])
            		{
            			break;
            		}
            	}
                if (i >= MAX_OPEN_FILES)
#endif                    
                    CacheWriteBackAll();
				//CopyFat(fp->Drive, 0);
				UpdataFreeMem(fp->Drive, -fp->FileSize);
                #ifdef IN_SYSTEM				
				ClearEncInfo();
                #endif
			}
		}
#endif
		fp->Flags = 0;
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FileRead
** 功能描述	:读取文件
** 输　入	:Buf:保存读回的数据指针
**        	 Size:1=byte, 2=halfword, 4=word
             count:读取的个数
			 fp指定的文件名柄
** 输　出	:实际读到的字节数
** 全局变量	:
** 调用模块: 无
********************************************************************************************************/
uint32 FileRead(void *Buf, uint8 Size, uint32 count, pFILE fp)
{
	uint32 SecIndex;
	uint16 offsetInSec,offsetInClu;
	uint32 len;
	uint32 cnt;
    pDRIVE_INFO pInfo;
    uint8 tmp[512];
    uint8 *pBuf=Buf;
    uint16 BytePerClus;
    uint32 ReadContLba;
    uint16 ReadContSectors=0;
    uint8 *ReadContPtr;
    PRINT_I("FileRead offset=%x,Buf = %x count = %x\n" ,fp->Offset, Buf,count);
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1] && fp->Flags != 0)
	{
        pInfo=GetDriveInfo(fp->Drive);
        count *= Size;
		if ((count + fp->Offset) > fp->FileSize)
			count = fp->FileSize  - fp->Offset;
		cnt=count;
        #if 0
		while (cnt > 0)	//判文件结束
		{
			if (fp->Clus >= BAD_CLUS)		//增加出错判断
				break;
		
			offsetInSec=fp->Offset &  (pInfo->BytsPerSec-1);
			offsetInClu=fp->Offset & ((1 << (pInfo->LogSecPerClus+9)) - 1);
			SecIndex=((fp->Clus - 2) << pInfo->LogSecPerClus) + pInfo->FirstDataSector+(offsetInClu >> 9);
			len=512-offsetInSec;	//读到该PAGE结束
            if (OK != ReadSecs(fp->Drive, SecIndex, tmp, (len+511)>>9))
            {
                cnt=count;
                break;
            }
			if (len > cnt)
			{
				len=cnt;
			}
            ftl_memcpy(pBuf, tmp+offsetInSec, len);
			pBuf += len;

			cnt -= len;
			if ((offsetInClu+len) >= (1 << (pInfo->LogSecPerClus+9)))
			{
				SecIndex=FATGetNextClus(fp->Drive, fp->Clus, 1);
				if (SecIndex == BAD_CLUS)
				{
		            cnt=count;
                    break;
				}
				else
					fp->Clus = SecIndex;
			}

			fp->Offset += len;
			if (fp->Offset >= fp->FileSize)		//判文件结束
			{
				cnt=fp->Offset-fp->FileSize;
				fp->Offset=fp->FileSize;
				break;
			}
		}
        #else
        BytePerClus = (1 << (pInfo->LogSecPerClus+9));
        while (cnt > 0)	//判文件结束
		{
			if (fp->Clus >= BAD_CLUS)		//增加出错判断
				break;

            offsetInSec=fp->Offset &  (pInfo->BytsPerSec-1);
            offsetInClu=fp->Offset & (BytePerClus - 1);
			SecIndex=((fp->Clus - 2) << pInfo->LogSecPerClus) + pInfo->FirstDataSector+(offsetInClu >> 9);

            if(offsetInSec || cnt < BytePerClus || offsetInClu)
            //if(offsetInSec || cnt < BytePerClus)
            {
                len=512-offsetInSec;	//读到该PAGE结束
                if (OK != ReadSecs(fp->Drive, SecIndex, tmp, (len+511)>>9))
                {
                    cnt=count;
                    break;
                }
    			if (len > cnt)
    			{
    				len=cnt;
    			}
                ftl_memcpy(pBuf, tmp+offsetInSec, len);
                pBuf += len;
                cnt -= len;
                fp->Offset += len;
                
                if ((offsetInClu+len) >= BytePerClus)
    			{
    				SecIndex=FATGetNextClus(fp->Drive, fp->Clus, 1);
    				if (SecIndex == BAD_CLUS)
    				{
    		            cnt=count;
                        break;
    				}
    				else
    					fp->Clus = SecIndex;
    			}
                SecIndex = ((fp->Clus - 2) << pInfo->LogSecPerClus) + pInfo->FirstDataSector+(offsetInClu >> 9);
            }
            else
            {
                while(1)
                {
                    len = BytePerClus;
                    if(ReadContSectors == 0)
                    {
                        ReadContLba = SecIndex;
                        ReadContPtr = pBuf;
                    }
                    ReadContSectors += (len>>9);
                    pBuf += len;
                    cnt -= len;
                    if ((offsetInClu+len) >= BytePerClus)
        			{
        				SecIndex=FATGetNextClus(fp->Drive, fp->Clus, 1);
        				if (SecIndex == BAD_CLUS)
        				{
        		            cnt=count;
                            break;
        				}
        				else
        					fp->Clus = SecIndex;
        			}
                    
                    fp->Offset += len;
                    
                    SecIndex = ((fp->Clus - 2) << pInfo->LogSecPerClus) + pInfo->FirstDataSector+(offsetInClu >> 9);

                    if(cnt < BytePerClus || SecIndex != ReadContLba + ReadContSectors)
                    {
                        break;
                    }
                }

                if (OK != ReadSecs(fp->Drive, ReadContLba, ReadContPtr, ReadContSectors))
                {
                    cnt=count;
                    break;
                }
                ReadContSectors = 0;
            }
            
			if (fp->Offset >= fp->FileSize)		//判文件结束
			{
				cnt=fp->Offset-fp->FileSize;
				fp->Offset=fp->FileSize;
				break;
			}
		}
        #endif
	}

#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
    return (count-cnt);
}


/*********************************************************************************************************
** 函数名称	:FileEof
** 功能描述	:判断文件是否到读\写到文件尾
** 输　入	:无
** 输　出	:0:否,1:是
** 全局变量: 
** 调用模块: 无
********************************************************************************************************/
bool FileEof(pFILE fp)
{
	bool Rt;

#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	Rt=TRUE;
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1])
	{
		if (fp->Offset < fp->FileSize)
			Rt=FALSE;
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FileSeek
** 功能描述	:移动文件读\写位置
** 输　入	:offset:移动偏移量
**        	 Whence:移动模式
				 SEEK_SET:从文件头计算(往后偏移都为+)
				 SEEK_CUR:从当前位置计算(往前为-, 往后为+)
				 SEEK_END:从文件尾计算(往前都为+, 往后为-)
** 输　出	:无
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint8 FileSeek(pFILE fp, int32 offset, uint8 Whence)
{
	uint8 Rt;
	uint32 OldClusCnt;
	uint32 NewClusCnt;
	uint32 Clus;
    pDRIVE_INFO pInfo;
    
	Rt = PARAMETER_ERR;
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1])
	{
        pInfo=GetDriveInfo(fp->Drive);
		if (fp->Flags  != 0)                            		// 对应的文件是否已打开
		{
			Rt = RETURN_OK;
			OldClusCnt = fp->Offset >> (pInfo->LogSecPerClus+9);
			switch (Whence)
			{
				case SEEK_END:					    /* 从文件尾计算 */
					fp->Offset = fp->FileSize - offset;
					offset = -offset;
					break;
				case SEEK_SET:
					fp->Offset = offset;			/* 从文件头计算 */
					break;
				case SEEK_CUR:                      /* 从当前位置计算 */
					fp->Offset += offset;
					break;
				case SEEK_REF:                      /* 从参考位置计算 */
					fp->Offset += offset;
					if (fp->Offset >= fp->RefOffset)
					{
						OldClusCnt = fp->RefOffset >> (pInfo->LogSecPerClus+9);
						fp->Clus = fp->RefClus;
					}
					break;
				default:
					Rt = PARAMETER_ERR;
					break;
			}
			if (Rt == RETURN_OK)
			{
				if (fp->Offset > fp->FileSize)
				{
					if (offset > 0)
						fp->Offset = fp->FileSize;
					else
						fp->Offset = 0;
				}
				/* 改变当前簇号 */
				NewClusCnt = fp->Offset >> (pInfo->LogSecPerClus+9);
				if (NewClusCnt >= OldClusCnt)
				{
					NewClusCnt -= OldClusCnt;
					Clus = fp->Clus;
				}
				else
				{
					Clus = fp->FstClus;
				}
                OldClusCnt=FATGetNextClus(fp->Drive, Clus, NewClusCnt);
				if (OldClusCnt == BAD_CLUS)
					Rt=FAT_ERR;				
				else
					fp->Clus=OldClusCnt;
			}
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FileExtNameMatch
** 功能描述	:文件扩展名过滤
** 输　入	:SrcExtName源扩展名,Filter过滤器
** 输　出	:TRUE匹配,FALSE不匹配
** 全局变量	:无
** 调用模块	:无
********************************************************************************************************/
bool FileExtNameMatch(char *SrcExtName, char *Filter)
{
	if (*Filter == '*')
		return (TRUE);

	StrUprCase(Filter);
	while (*Filter != '\0')
	{
		if (SrcExtName[0]==Filter[0] || Filter[0]=='?')
		{
			if (SrcExtName[1]==Filter[1] || Filter[1]=='?')
			{
				if (SrcExtName[2]==Filter[2] || Filter[2]=='?')
				{
					return (TRUE);
				}
			}
		}
		Filter += 3;
	}
	return (FALSE);
}



/*********************************************************************************************************
** 函数名称	:StrUprCase
** 功能描述	:小写字母转大写字母 
** 输　入	:大小写字母混合的文件名
** 输　出	:大写字母文件名
** 全局变量	:无
** 调用模块	:无
********************************************************************************************************/
void StrUprCase(char *name)
{
#if 0
	while (*name != '\0')
	{
		if (*name >= 'a' && *name <= 'z')
			*name=*name-'a'+'A';
		name++;
	}
#endif	
}


/*********************************************************************************************************
** 函数名称	:FileOpenExt
** 功能描述	:以指定方式打开文件
** 输　入	:Path:路径, DirFileName:用户使用的文件名, Type:打开方式
** 输　出	:Not_Open_FILE为不能打开,其它为打开文件的句柄
** 全局变量: 
** 调用模块: 无
********************************************************************************************************/
pFILE FileOpenExt(char *Path, char *DirFileName, char *Type)
{
    uint8 Drive;
    uint32 DirClus;
    
    //if (Type[0]=='W' || Type[0]=='w')
    //    return (FileCreate(Path, DirFileName));
    Drive=GetDrive(Path);
	DirClus=GetDirClusIndex(Path);
    return (FileOpen(Drive, DirClus, DirFileName, Type));
}



#ifndef BOOT_ONLY
/*********************************************************************************************************
** 函数名称 :FindFile
** 功能描述 :查找当前目录/全局目录下指定类型的第几个文件
**           FileNum:文件号,ExtName:扩展名, Attr:属性(ATTR_DIRECTORY目录, ATTR_VOLUME_ID卷标, 0文件)
** 输　出   :Rt:找到的文件目录项信息
** 全局变量 :
** 调用模块 :无
** 说  明   :当扩展名指定为"*"时, 也会查找到目录
********************************************************************************************************/
uint8 FindFileSub(FDT *Rt, FIND_DATA* FindData, uint16 FileNum, char *ExtName, uint8 Attr)
{
    uint16 i, num;
    uint8 offset;
    LONG_FDT *LongFdt;
    uint32 index;
    uint16 Items;
    FDT tmp;

    index=FindData->Index;
    num=0;
    while (1)
    {
        if (RETURN_OK!=GetFDTInfo(Rt, FindData->Drive, FindData->Clus, index++))
            break;
        
        if (Rt->Name[0]==FILE_NOT_EXIST)                //空目录项,后面不再有文件
            break;
        
        if (Rt->Name[0]!=FILE_DELETED)
        {
            Items=0;
            while (Rt->Attr==ATTR_LFN_ENTRY)            //长文件名项要找到短文件名
            {
                GetFDTInfo(Rt, FindData->Drive, FindData->Clus, index++);
                Items++;    
            }

            //if ((Rt->Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == Attr)
            if ((Rt->Attr & Attr) == Attr)  
            {
                if (FileExtNameMatch(&Rt->Name[8], ExtName))
                {
                    if (++num == FileNum)
                    {
                        FindData->Index=index;
                        if (Items != 0) //长目录项
                        {
                            index -= Items;
                            memset(LongFileName, '\0', MAX_FILENAME_LEN);
                            while (1)
                            {
                                GetFDTInfo(&tmp, FindData->Drive, FindData->Clus, index+Items-2);
                                LongFdt=(LONG_FDT*)&tmp;
                                offset=13 * ((LongFdt->Order & LFN_SEQ_MASK) - 1);
                                if ((LongFdt->Order & LFN_SEQ_MASK) <= MAX_LFN_ENTRIES)   
                                {/* 长文件名最多目录项数*/
                                    for (i = 0; i < 5; i++)
                                        LongFileName[i+offset]=(LongFdt->Name1[i*2+1]<<8) | LongFdt->Name1[i*2];
                                    for (i = 0; i < 6; i++)
                                        LongFileName[i+5+offset]=(LongFdt->Name2[i*2+1]<<8) | LongFdt->Name2[i*2];
                                    for (i = 0; i < 2; i++)
                                        LongFileName[i+11+offset]=(LongFdt->Name3[i*2+1]<<8) | LongFdt->Name3[i*2];
                                }
                                if (--Items == 0)
                                    break;
                            }
                        }
                        else        //短目录项
                        {
                            for (i=0; i<8; i++)
                            {
                                LongFileName[i] = Rt->Name[i];
                                if (LongFileName[i] == ' ')
                                    break;
                            }
                            if (Rt->Name[8] != ' ')
                            {
                                LongFileName[i++] = '.';                //追加扩展名
                                LongFileName[i++] = Rt->Name[8];
                                LongFileName[i++] = Rt->Name[9];
                                LongFileName[i++] = Rt->Name[10];
                            }
                            LongFileName[i] = '\0';                 //结束标志符unicode码的NUL
                        }
                        return (RETURN_OK);
                    }
                }
            }
        }
    }
    return (NOT_FIND_FILE);
}



/*********************************************************************************************************
** 函数名称 :FindFirst
** 功能描述 :查找指定目录的第一个文件
**           FindData:文件查找结构,Path:路径, ExtName:扩展名
** 输　出   :Rt:找到的文件目录项信息
** 全局变量 :
** 调用模块 :无
** 说  明   :当扩展名指定为"*"时, 也会查找到目录
********************************************************************************************************/
uint8 FindFirst(FDT *Rt, FIND_DATA* FindData, char *Path, char *ExtName)
{
    FindData->Clus=GetDirClusIndex(Path);
    FindData->Drive=GetDrive(Path);
    FindData->Index=0;
    return (FindNext(Rt, FindData, ExtName));
}



/*********************************************************************************************************
** 函数名称 :FindNext
** 功能描述 :查找指定目录的下一个文件
**           FindData:文件查找结构,ExtName:扩展名
** 输　出   :Rt:找到的文件目录项信息
** 全局变量 :
** 调用模块 :无
** 说  明   :当扩展名指定为"*"时, 也会查找到目录
********************************************************************************************************/
uint8 FindNext(FDT *Rt, FIND_DATA* FindData, char *ExtName)
{
    uint8 OsRt;

#ifdef OS_FILE
    OBTAIN_FAT_SEM;
#endif

    OsRt=PATH_NOT_FIND;
    if (FindData->Clus < BAD_CLUS)
    {
        OsRt=FindFileSub(Rt, FindData, 1, ExtName, 0);
    }

#ifdef OS_FILE
    RELEASE_FAT_SEM;
#endif
    return (OsRt);
}


/*********************************************************************************************************
** 函数名称	:FileModify
** 功能描述	:修改文件
** 输　入	:buf:要写的数据指针
**  		 Bytes:要写的字节数小于等于64KB
			 Handle:指定的文件句柄
			 offset:文件偏移量
** 输　出	:检查结果
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint8 FileModify(void* buf, uint32 offset, uint32 Bytes, pFILE fp)
{
	uint32 LBA;
	uint16 SecOffset;
	uint16 ClusOffset;
	uint8 Tmp[512];
	uint16 count;
	uint8 Rt;
    uint8 *pBuf=buf;
    pDRIVE_INFO pInfo;

	Rt = PARAMETER_ERR;
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1])
	{
		Rt = RETURN_OK;
		fp->Offset = offset;
        pInfo=GetDriveInfo(fp->Drive);
		while (Bytes > 0)
		{
			SecOffset = fp->Offset & 0x1ff;
			ClusOffset = fp->Offset & ((1 << (pInfo->LogSecPerClus+9)) - 1);
			LBA = ((fp->Clus - 2) << pInfo->LogSecPerClus) + pInfo->FirstDataSector+(ClusOffset >> 9);
			count=512-SecOffset;
            if (count > Bytes)
                count=Bytes;
            ftl_memcpy(&Tmp[SecOffset], pBuf, count);
            pBuf += count;
			Bytes -= count;
			fp->Offset += count;
			if (fp->Offset > fp->FileSize)
			{
				Rt = FILE_EOF;
				break;
			}
            WriteSecs(fp->Drive, LBA, Tmp, 1);
		}
	}
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FileWrite
** 功能描述	:写文件
** 输　入	:Buf:要写的数据指针
**        	 Size:1=byte, 2=halfword, 4=word
             count:写入的个数
			 fp指定的文件名柄
** 输　出	:实际写的字节数
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint32 FileWrite(void *Buf, uint8 Size, uint32 count, pFILE fp)
{
	uint32 NewClus;
	uint16 offsetInSec,offsetInClu;
	uint32 len;
	uint32 cnt;
    uint8 *pBuf=Buf;
    pDRIVE_INFO pInfo;

    count *= Size;
	if ((count % 512) != 0)
		return (0);
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1] && (fp->Flags & FILE_FLAGS_WRITE) != 0)
	{
        cnt=count;
        pInfo=GetDriveInfo(fp->Drive);
		FLASH_PROTECT_OFF();
		while (cnt > 0)
		{
			offsetInSec=fp->Offset &  (pInfo->BytsPerSec-1);
			offsetInClu= fp->Offset & ((1 << (pInfo->LogSecPerClus+9)) - 1);

			if (fp->Offset > fp->FileSize)	//文件指针超出文件长度
				break;
			
			if (offsetInClu == 0)			//要新增一个簇
			{
				if (fp->Offset < fp->FileSize)
				{
					if (fp->Offset == 0)		//回写首簇
						fp->Clus = fp->FstClus;
					else
						fp->Clus = FATGetNextClus(fp->Drive, fp->Clus, 1);
				}
				else
				{
					NewClus= FATAddClus(fp->Drive, fp->Clus);
					if (NewClus >= BAD_CLUS)			//磁盘空间满
					{
						break;
					}
					fp->Clus = NewClus;
					if (fp->FstClus == EMPTY_CLUS)		//还未分配簇
					{
						fp->FstClus = fp->Clus;
                        #ifdef IN_SYSTEM
						{
							RCV_INFO EncRcvInfo;
							
							EncRcvInfo.Flag=0x0f;
							EncRcvInfo.Drive=fp->Drive;
							EncRcvInfo.DirClus=fp->DirClus;
							EncRcvInfo.FstClus=fp->FstClus;
                            ftl_memcpy(EncRcvInfo.FileName, fp->Name, 11);
							SaveEncInfo(&EncRcvInfo);
							FLASH_PROTECT_OFF();
						}
                        #endif
					}
				}
			}
			fp->Sec=((fp->Clus - 2) << pInfo->LogSecPerClus)+pInfo->FirstDataSector+(offsetInClu >> 9);
			len=pInfo->BytsPerSec-offsetInSec;		//读到该扇区结束
			if (len > cnt)
				len=cnt;
            if (OK != WriteSecs(fp->Drive, fp->Sec, pBuf, 1))
                break;
			cnt -= len;
			pBuf += len;
			fp->Offset += len;
			if (fp->Offset > fp->FileSize)
				fp->FileSize = fp->Offset;
		}
		FLASH_PROTECT_ON();
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
    return (count-cnt);
}


/*********************************************************************************************************
** 函数名称	:FindOpenFile
** 功能描述	:查找已打开的指定文件的文件句柄
** 输　入	:FileName:内部文件名
** 输　出	:文件句柄
** 全局变量	:
** 调用模块: 无
********************************************************************************************************/
uint8 FindOpenFile(uint32 DirClus, char *FileName)
{
	pFILE fp;
	uint8 i, j;
	
	fp=FileInfo;
	for (j=0; j<MAX_OPEN_FILES; j++)
	{
		if (fp->Flags != 0)
		{
    		if (fp->DirClus == DirClus)
    		{
        		for (i=0; i<11; i++)
        		{
        			if (fp->Name[i] != FileName[i])
        				break;
        		}
        		if (i == 11)
        			break;
    		}
		}
		fp++;
	}
	return (j);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:删除文件
** 输　入	:Path:路径, DirFileName:用户使用的文件名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
uint8 FileDelete(char *Path, char *DirFileName)
{
	uint32 ClusIndex1;
	uint32 DirClus;
	uint8 Rt;
	FDT temp;
    uint8 Drive;

#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	StrUprCase(Path);
	StrUprCase(DirFileName);
	DirClus=GetDirClusIndex(Path);	//获取路径所在的簇号
	Drive=GetDrive(Path);
	Rt = PATH_NOT_FIND;
	if (DirClus != BAD_CLUS)			//确定路径存在
	{
		Rt = NOT_FIND_FILE;
		if (RETURN_OK == FindFDTInfo(&temp, Drive, DirClus, DirFileName))		//找到目录项
		{
			if ((temp.Attr & ATTR_DIRECTORY) == 0)  		// 是文件才删除
			{
				Rt = FILE_LOCK;
				if (FindOpenFile(DirClus, DirFileName) >= MAX_OPEN_FILES)	//文件没有打开才删除
				{
					ClusIndex1 = temp.FstClusLO + ((uint32)(temp.FstClusHI) << 16);
					FATDelClusChain(Drive, ClusIndex1);
					Rt = DelFDT(Drive, DirClus, DirFileName);
					UpdataFreeMem(Drive, temp.FileSize);
				}
			}
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FileCreate
** 功能描述	:建立文件
** 输　入	:Path:路径, DirFileName:用户使用的文件名
** 输　出	:RETURN_OK：成功
** 其它参考  fat.h中关于返回值的说明
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
pFILE FileCreate(char *Path, char *DirFileName)
{
	pFILE fp, Rt;
	FDT temp;
	uint8 i;
    uint8 Drive;

	Rt=NULL;
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	StrUprCase(Path);
	StrUprCase(DirFileName);
    Drive=GetDrive(Path);
	/* 查找空闲文件登记项 */
	for (i=0; i<MAX_OPEN_FILES; i++)
	{
		if (FileInfo[i].Flags == 0)
		{
			break;
		}
	}
    
	if (i < MAX_OPEN_FILES)
	{
		fp = &FileInfo[i];		//指向文件句柄
		memset((uint8*)&temp, 0x00, sizeof(FDT));
		ftl_memcpy((uint8*)temp.Name, DirFileName, 11);
		temp.Attr = ATTR_ARCHIVE;		//存档
		fp->DirClus=GetDirClusIndex(Path);
		if (fp->DirClus < BAD_CLUS)
		{
            fp->Drive=GetDrive(Path);
			if (AddFDT(fp->Drive, fp->DirClus, &temp) == RETURN_OK)      //增加文件
			{
				/* 设置文件信息 */
		        ftl_memcpy(fp->Name, (uint8*)temp.Name, 11);
				fp->Flags = FILE_FLAGS_READ | FILE_FLAGS_WRITE;
				fp->FileSize = 0;
				fp->FstClus = 0;
				fp->Clus = 0;
				fp->Offset = 0;
                Rt=fp;
			}
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FindFirst
** 功能描述	:查找指定目录的第一个文件
**				 FindData:文件查找结构,Path:路径
** 输　出	:Rt:找到的文件目录项信息
** 全局变量	:
** 调用模块	:无
** 说  明	:当扩展名指定为"*"时, 也会查找到目录
********************************************************************************************************/
uint8 FindDirFirst(FDT *Rt, FIND_DATA* FindData, char *Path)
{
	FindData->Clus=GetDirClusIndex(Path);
    FindData->Drive=GetDrive(Path);
	FindData->Index=0;
	return (FindDirNext(Rt, FindData));
}



/*********************************************************************************************************
** 函数名称	:FindNext
** 功能描述	:查找指定目录的下一个文件
**			 FindData:文件查找结构
** 输　出	:Rt:找到的文件目录项信息
** 全局变量	:
** 调用模块	:无
** 说  明	:当扩展名指定为"*"时, 也会查找到目录
********************************************************************************************************/
uint8 FindDirNext(FDT *Rt, FIND_DATA* FindData)
{
	uint8 OsRt;

#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif

	OsRt=PATH_NOT_FIND;
	if (FindData->Clus < BAD_CLUS)
	{
    	OsRt=FindFileSub(Rt, FindData, 1, "*", ATTR_DIRECTORY);
	}

#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (OsRt);
}


/*********************************************************************************************************
** 函数名称	:GetLongFileName
** 功能描述	:获取长文件名
** 输　入	:无
** 输　出	:lfn:以UNICODE编码的长文件名16bit
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
void GetLongFileName(char *lfn)
{
	uint16 i=0;

	do
	{
		*lfn++ = LongFileName[i];
	}while (LongFileName[i++] != '\0');
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:获取当前目录下的文件总数
** 输　入	:ExtName:文件扩展名
** 输　出	:文件总数
** 全局变量	:
** 调用模块	:无
********************************************************************************************************/
uint16 GetTotalFiles(char *Path, char *ExtName)
{
	uint32 offset;
	uint16 TotalFiles;
	FDT temp;
	uint32 DirClus;
    uint8 Drive;

	offset=0; TotalFiles=0;
	DirClus=GetDirClusIndex(Path);
    Drive=GetDrive(Path);
	if (DirClus != BAD_CLUS)
	{
		while (1)
		{
			if (RETURN_OK!=GetFDTInfo(&temp, Drive, DirClus, offset++))
				break;

			if (temp.Name[0]==FILE_NOT_EXIST)
				break;

			if (temp.Name[0]!=FILE_DELETED)
			{
				if ((temp.Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0)	//是文件
				{
					while (temp.Attr==ATTR_LFN_ENTRY)		//长文件名
					{
						GetFDTInfo(&temp, Drive, DirClus, offset++);
					}

					if (FileExtNameMatch(&temp.Name[8], ExtName))
						TotalFiles++;
				}
			}
		}
	}
	return (TotalFiles);
}


/*********************************************************************************************************
** 函数名称	:RecoveryEncInfo
** 功能描述	:录音异常中断时上电恢复
** 输　入	:文件恢复信息(文件名, 文件首簇, 目录簇)
** 输　出	:恢复成功指示(OK--成功, ERROR--失败)
** 全局变量	:
** 调用模块	:
** 说   明  :RecoveryInfo结构体由录音分配首簇后存FLASH
********************************************************************************************************/
uint8 RecoveryEncInfo(RCV_INFO* RecoveryInfo)
{
	FDT tmp;
	uint32 FileSize;
	uint32 ThisClus, NextClus;
    pDRIVE_INFO pInfo;

	if (RecoveryInfo->Flag == 0x0f)
	{
		if (FindFDTInfo(&tmp, RecoveryInfo->Drive, RecoveryInfo->DirClus, RecoveryInfo->FileName) == RETURN_OK)
		{
            pInfo=GetDriveInfo(RecoveryInfo->Drive);
            //从FAT表中搜索链表以确定文件大小
            for (FileSize=0, ThisClus=RecoveryInfo->FstClus; ; FileSize++)
			{
                NextClus=FATGetNextClus(RecoveryInfo->Drive, ThisClus, 1);
                if (NextClus>=BAD_CLUS || NextClus<=EMPTY_CLUS_1 || NextClus<=ThisClus)
                    break;
                ThisClus=NextClus;
			}
			FileSize <<= pInfo->LogSecPerClus+9;
			tmp.FstClusLO=(uint16)(RecoveryInfo->FstClus & 0xffff);
			tmp.FstClusHI=(uint16)(RecoveryInfo->FstClus >> 16);
			tmp.FileSize=FileSize;
			ChangeFDT(RecoveryInfo->Drive, RecoveryInfo->DirClus, &tmp);
			UpdataFreeMem(RecoveryInfo->Drive, -FileSize);
			#if 0
			if (RecoveryInfo->FileName[8]=='W' && RecoveryInfo->FileName[9]=='A' && RecoveryInfo->FileName[10]=='V')
			{
	            uint32 LBA;
	            uint8 buf[512];
                
				LBA = ((RecoveryInfo->FstClus - 2) << pInfo->LogSecPerClus) + pInfo->FirstDataSector;
                ReadSecs(RecoveryInfo->Drive, LBA, buf, 1);
				buf[7]=(uint8)(FileSize >> 24) & 0xff;
				buf[6]=(uint8)(FileSize >> 16) & 0xff;
				buf[5]=(uint8)(FileSize >> 8) & 0xff;
				buf[4]=(uint8)FileSize & 0xff;

				FileSize -= 512;
				buf[511]=(uint8)(FileSize >> 24) & 0xff;
				buf[510]=(uint8)(FileSize >> 16) & 0xff;
				buf[509]=(uint8)(FileSize >> 8) & 0xff;
				buf[508]=(uint8)FileSize & 0xff;
                WriteSecs(RecoveryInfo->Drive, LBA, buf, 1);
			}
			#endif
            #ifdef IN_SYSTEM				
			ClearEncInfo();
            #endif
		}
	}
	return (OK);
}



/*********************************************************************************************************
** 函数名称	:FindFile
** 功能描述	:查找当前目录/全局目录下指定类型的第几个文件
**			 FileNum:文件号,Path:路径,ExtName:扩展名
** 输　出	:Rt:找到的文件目录项信息
** 全局变量	:
** 调用模块	:无
** 说  明	:当扩展名指定为"*"时, 也会查找到目录
********************************************************************************************************/
uint8 FindFile(FDT *Rt, uint16 FileNum, char *Path, char *ExtName)
{
	uint8 OsRt;
    uint16 FileTotal;
    FIND_DATA FindTmp;
    pDRIVE_INFO pInfo;

#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif

	OsRt=PATH_NOT_FIND;
	FindTmp.Clus=GetDirClusIndex(Path);
	if (FindTmp.Clus != BAD_CLUS)
	{
        FindTmp.Drive=GetDrive(Path);
        pInfo=GetDriveInfo(FindTmp.Drive);
    	//modify by lxs @2007.03.20, 路径中的字符有可能含有'\'(如繁体的诊)
    	if ((Path[0] == '\\'  && Path[1] == '\0') || (Path[1] == ':' && Path[2] == '\\' && Path[3] == '\0'))
    	{
    		GotoRootDir(FindTmp.Drive, ExtName);
            while (1)
    		{
                if(ExtName[0] == '*')
                    FileTotal = pInfo->DirInfo[pInfo->DirDeep].TotalFile + pInfo->DirInfo[pInfo->DirDeep].TotalSubDir;
                else
                    FileTotal = pInfo->DirInfo[pInfo->DirDeep].TotalFile;
                if (FileNum <= FileTotal)
                    break;
    			FileNum -= FileTotal;
    			GotoNextDir(FindTmp.Drive, ExtName);
    			if (pInfo->DirDeep == 0)
    			{
    				goto FindErr;
    			}
    		}
    		FindTmp.Clus=pInfo->PathClus;
    	}
        FindTmp.Index=0;
    	OsRt=FindFileSub(Rt, &FindTmp, FileNum, ExtName, 0);
	}

FindErr:
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (OsRt);
}


/*********************************************************************************************************
** 函数名称	:FindFile
** 功能描述	:查找当前目录/全局目录下指定类型的第几个文件
**			 FileNum:文件号,Path:路径,ExtName:扩展名
** 输　出	:Rt:找到的文件目录项信息
** 全局变量	:
** 调用模块	:无
** 说  明	:当扩展名指定为"*"时, 也会查找到目录
********************************************************************************************************/
uint8 FindFileBrowser(FDT *Rt, uint16 FileNum, char *Path, char *ExtName)
{
	uint8 OsRt;
    FIND_DATA FindTmp;

#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif

	OsRt=PATH_NOT_FIND;
	FindTmp.Clus=GetDirClusIndex(Path);
	if (FindTmp.Clus != BAD_CLUS)
	{
    	FindTmp.Index=0;
        FindTmp.Drive=GetDrive(Path);
    	OsRt=FindFileSub(Rt, &FindTmp, FileNum, ExtName, 0);
	}

#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (OsRt);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:从全局文件号指针得到当前目录文件号指针
** 输　入	:全局文件指针FileNum, 文件类型ExtName
** 输　出	:当前目录下文件号指针, 返回0出错
** 全局变量	:
** 调用模块	:
** 说    明 :调用该函数后自动进入该目录
********************************************************************************************************/
uint16 GetCurFileNum(uint8 Drive, uint16 FileNum, char *ExtName)
{
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	GotoRootDir(Drive, ExtName);
	while (FileNum > pInfo->DirInfo[pInfo->DirDeep].TotalFile)
	{
		FileNum -= pInfo->DirInfo[pInfo->DirDeep].TotalFile;
		GotoNextDir(Drive, ExtName);
		if (pInfo->DirDeep == 0)
		{
			FileNum=0;
			break;
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (FileNum);
}


/*********************************************************************************************************
** 函数名称	:
** 功能描述	:从当前目录文件号指针得到全局文件号指针
** 输　入	:当前目录下文件号指针FileNum, 当前目录路径Path, 文件类型ExtName
** 输　出	:全局文件指针, 返回0出错
** 全局变量	:
** 调用模块	:
** 说   明  :调用该函数后自动进入该目录
********************************************************************************************************/
uint16 GetGlobeFileNum(uint16 FileNum, char *Path, char *ExtName)
{
	uint32 DirClus;
    uint8 Drive;
    pDRIVE_INFO pInfo;

#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	DirClus=GetDirClusIndex(Path);
	if (DirClus == BAD_CLUS)
	{
		FileNum=0;
	}
	else
	{
        Drive=GetDrive(Path);
        pInfo=GetDriveInfo(Drive);
		GotoRootDir(Drive, ExtName);
		while (DirClus != pInfo->PathClus)		//直到找到当前目录
		{
			FileNum += pInfo->DirInfo[pInfo->DirDeep].TotalFile;
			GotoNextDir(Drive, ExtName);
			if (pInfo->DirDeep == 0)
			{
				FileNum=0;
				break;
			}
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (FileNum);
}


/*********************************************************************************************************
** 函数名称	:FileRefSet
** 功能描述	:设置文件参数位置点
** 输　入	:Handle:文件句柄
** 输　出	:无
** 全局变量	:无
** 调用模块	:
** 说  明	:
********************************************************************************************************/
uint8 FileRefSet(pFILE fp)
{
	uint8 Rt;
    pDRIVE_INFO pInfo;
    
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	Rt = PARAMETER_ERR;
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1])
	{
        pInfo=GetDriveInfo(fp->Drive);
		if ((fp->Offset <= fp->FileSize) &&  (fp->Clus <= pInfo->TotClus+2))
		{
			Rt = RETURN_OK;
			fp->RefClus=fp->Clus;
			fp->RefOffset=fp->Offset;
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}


/*********************************************************************************************************
** 函数名称	:FileRefReset
** 功能描述	:恢复文件参数位置点
** 输　入	:Handle:文件句柄
** 输　出	:无
** 全局变量	:无
** 调用模块	:
** 说  明	:
********************************************************************************************************/
uint8 FileRefReset(pFILE fp)
{
	uint8 Rt;
    pDRIVE_INFO pInfo;
    
#ifdef OS_FILE
	OBTAIN_FAT_SEM;
#endif
	Rt = PARAMETER_ERR;
	if (fp >= &FileInfo[0] && fp <= &FileInfo[MAX_OPEN_FILES-1])
	{
        pInfo=GetDriveInfo(fp->Drive);
		if ((fp->Offset <= fp->FileSize) &&  (fp->Clus <= pInfo->TotClus+2))
		{
			Rt = RETURN_OK;
			fp->Clus=fp->RefClus;
			fp->Offset=fp->RefOffset;
		}
	}
#ifdef OS_FILE
	RELEASE_FAT_SEM;
#endif
	return (Rt);
}



/*********************************************************************************************************
** 函数名称	:VolumeCreate
** 功能描述	:建立磁盘卷标
** 输　入	:DirFileName:卷标名必须是8.3格式
** 输　出	:
** 其它参考  
** 全局变量	:
** 调用模块	:
********************************************************************************************************/
void VolumeCreate(uint8 Drive, char *DirFileName)
{
	FDT temp, temp1;
	uint8 i;
	uint8 Result;
	uint32 index;
    pDRIVE_INFO pInfo=GetDriveInfo(Drive);

    memset((uint8*)&temp, 0x00, 32);
    memset((uint8*)temp.Name, ' ', 11);
	for (i = 0; i < 11; i++)
	{	
		if (DirFileName[i] == '\0')
			break;
		temp.Name[i] = DirFileName[i];
	}
	temp.Attr = ATTR_VOLUME_ID | ATTR_ARCHIVE;
	index=0;
	while (1)
	{
		Result = GetFDTInfo(&temp1, Drive, pInfo->RootClus, index);
		if (Result == FDT_OVER || Result != RETURN_OK)
		{
			break;
		}

		if (temp1.Name[0] == FILE_NOT_EXIST)
		{
			SetFDTInfo(Drive, pInfo->RootClus, index, &temp);
			break;
		}
		
		if ((temp.Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_VOLUME_ID)
		{
			for (i=0; i<11; i++)
			{
				if (temp1.Name[i] != temp.Name[i])
				{
					SetFDTInfo(Drive, pInfo->RootClus, index, &temp);
					break;
				}
			}
			break;
		}
		index++;
	}
}
#endif

/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/

