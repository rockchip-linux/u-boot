/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    file.h
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#ifndef _FILE_H
#define _FILE_H

//1可配置参数
#define 	MAX_OPEN_FILES		    4		/*可以同时打开的文件数目*/
#define 	MAX_LFN_ENTRIES   		3		/*最长就3*13个字符*/


//1常量定义

#define 	NOT_OPEN_FILE			-1		//不能打开文件,文件句柄满


/* 文件系统数据宽度,参加运算, 不能更改定义值*/
#define	X8		1
#define	X16		2
#define	X32		4



/* 长文件名*/
#define 	MAX_FILENAME_LEN  	    ((MAX_LFN_ENTRIES * 13 * 2 + 2)/2)		//结尾的NUL占用2B
#define 	LFN_SEQ_MASK			0x3f

/* 文件打开方式 */
#define 	FILE_FLAGS_READ          	1 << 0		//可读
#define 	FILE_FLAGS_WRITE        	1 << 1		//可写

/* 文件指针调整方式 */
#define 	SEEK_SET    				0           		//从文件开始处移动文件指针
#define 	SEEK_CUR    				1           		//从文件当前位置移动文件指针
#define 	SEEK_END    				2           		//从文件尾移动文件指针
#define 	SEEK_REF    				3           		//从文件参考点开始


//1结构定义
//查找结构体
typedef struct _FIND_DATA
{
    uint8   Drive;
	uint32	Index;
	uint32	Clus;     			//当前簇号
} FIND_DATA;

//文件信息结构体
typedef struct _FILE
{
	uint8	Flags;          	//一些标志
	char	Name[11];    		//文件名
	uint8   Drive;
	uint32	Sec;	            //当前扇区
	uint32	DirClus;	        //所在目录开始簇号
	uint32	FileSize;		    //文件大小
	uint32	FstClus;			//起始簇号
	uint32	Clus;     			//当前簇号
	uint32	Offset;			    //文件指针偏移量
	uint32	RefClus;    		//当前簇号
	uint32	RefOffset;		    //文件指针偏移量
} MY_FILE, *pFILE;


typedef struct _recovery_info
{
	uint8 Drive;
	uint8 Flag;
	char FileName[11];
    uint32 FstClus;
	uint32 DirClus;
} RCV_INFO, *pRCV_INFO;



//1全局变量
#undef	EXT
#ifdef	IN_FILE
		#define	EXT
#else
		#define	EXT		extern
#endif		
		
EXT		MY_FILE 	FileInfo[MAX_OPEN_FILES];		//同时打开文件信息表
EXT		uint16 		LongFileName[MAX_FILENAME_LEN];	//长文件名


/*******************************************************************************************************/
//1函数原型声明
//#ifndef IN_FILE
extern 	void 	FileInit(void);
extern 	pFILE 	FileCreate(char *Path, char *DirFileName);
extern 	uint8 	FileDelete(char *Path, char *DirFileName);
extern 	pFILE 	FileOpen(uint8 Drive, uint32 DirClus, char *DirFileName, char *Type);
extern 	pFILE 	FileOpenExt(char *Path, char *DirFileName, char *Type);
extern 	uint8 	FileClose(pFILE fp);
extern 	uint32  FileRead(void *Buf, uint8 Size, uint32 count, pFILE fp);
extern 	uint32 	FileWrite(void *Buf, uint8 Size, uint32 count, pFILE fp);
extern 	uint8 	FileModify(void *buf, uint32 offset, uint32 Bytes, pFILE fp);
extern 	uint8 	FileCopy(char *SrcPath, char *DestPath, FDT *SrcFile);
extern 	bool 	FileEof(pFILE fp);
extern 	uint8 	FileSeek(pFILE fp, int32 offset, uint8 Whence);
extern 	uint8 	FindOpenFile(uint32 DirClus, char *FileName);
extern 	bool 	FileExtNameMatch(char *SrcExtName, char *Filter);
extern 	uint8 	FindFile(FDT *Rt, uint16 FileNum, char *Path, char *ExtName);
extern 	uint8 	FindFileBrowser(FDT *Rt, uint16 FileNum, char *Path, char *ExtName);
extern 	uint8 	FindFirst(FDT *Rt, FIND_DATA* FindData, char *Path, char *ExtName);
extern 	uint8 	FindNext(FDT *Rt, FIND_DATA* FindData, char *ExtName);
extern 	uint8 	FindDirFirst(FDT *Rt, FIND_DATA* FindData, char *Path);
extern 	uint8 	FindDirNext(FDT *Rt, FIND_DATA* FindData);
extern 	uint16 	GetTotalFiles(char *Path, char *ExtName);
extern 	void 	GetLongFileName(char *lfn);
extern 	uint16 	GetCurFileNum(uint8 Drive, uint16 FileNum, char *ExtName);
extern 	uint16 	GetGlobeFileNum(uint16 FileNum, char *Path, char *ExtName);
extern 	void 	StrUprCase(char *name);

extern 	uint8 	FileRefSet(pFILE fp);
extern 	uint8 	FileRefReset(pFILE fp);
extern 	void    VolumeCreate(uint8 Drive, char *DirFileName);
extern 	uint8	RecoveryEncInfo(RCV_INFO* RecoveryInfo);
extern 	void 	SaveEncInfo(RCV_INFO* RecoveryInfo);
#endif

/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/

