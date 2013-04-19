/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    FDT.h
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#ifndef _FDT_H
#define _FDT_H

//1可配置参数
#define 	FDT_CACHE_NUM               1

#define 	CACHE_FAT                   1
#define 	CACHE_FDT                   2
#define 	CACHE_DATA                  3

//1常量定义
/* FDT文件属性 */
#define 	ATTR_READ_ONLY              0x01
#define 	ATTR_HIDDEN                 0x02
#define 	ATTR_SYSTEM                 0x04
#define 	ATTR_VOLUME_ID             	0x08
#define 	ATTR_LFN_ENTRY    		    0x0F      /* LFN entry attribute */
#define 	ATTR_DIRECTORY             	0x10
#define 	ATTR_ARCHIVE                0x20

/* FDT类型 */
#define 	FILE_NOT_EXIST			    0
#define 	FILE_DELETED			    0xe5
#define 	ESC_FDT                    	0x05

/* 函数返回值 */
#define	    RETURN_OK                   0x00    	/* 操作成功*/
#define	    NOT_FIND_DISK               0x01    	/* 逻辑盘不存在*/
#define	    DISK_FULL                   0x02    	/* 逻辑盘满*/
#define	    SECTOR_NOT_IN_CACHE         0x03    	/* 扇区没有被cache  */
#define	    NOT_EMPTY_CACHE          	0x04    	/* 没有空闲cache*/
#define	    SECTOR_READ_ERR           	0x05    	/* 读扇区错误*/
#define	    CLUSTER_NOT_IN_DISK         0x06    	/* 逻辑盘中没有此簇  */
#define 	NOT_FIND_FDT                0x07    	/* 没有发现文件(目录)*/
#define 	NOT_FAT_DISK                0x08    	/* 非FAT文件系统*/
#define 	FDT_OVER                    0x09    	/* FDT索引超出范围*/
#define 	FDT_EXISTS                  0x0a    	/* 文件(目录)已经存在*/
#define 	ROOT_FDT_FULL               0x0b    	/* 根目录满*/
#define 	DIR_EMPTY                   0x0C    	/* 目录空*/
#define 	DIR_NOT_EMPTY               0x0d    	/* 目录不空*/
#define 	PATH_NOT_FIND               0x0e    	/* 路径未找到*/
#define 	FAT_ERR                     0x0f    	/* FAT表错误*/
#define 	FILE_NAME_ERR               0x10    	/* 文件(目录)名错误*/
#define 	FILE_EOF                    0x11    	/* 文件结束*/
#define 	FILE_LOCK                   0x12    	/* 文件被锁定*/
#define 	NOT_FIND_FILE               0x13    	/* 没有发现指定文件*/
#define 	NOT_FIND_DIR                0x14    	/* 没有发现指定目录*/
#define 	NOT_RUN                     0xfd    	/* 命令未执行*/
#define 	BAD_COMMAND                 0xfe    	/* 错误命令*/
#define 	PARAMETER_ERR               0xff    	/* 非法参数*/


//1结构定义

//目录项连接结构体
typedef struct _FDT_REF
{
	uint32	DirClus;			//目录首簇
	uint32	CurClus;     		//当前簇号
	uint16	Cnt;				//计数簇链
} FDT_REF;

typedef __packed struct _FDT
{
	char	Name[11];		    //短文件名主文件名
	uint8	Attr;           	//文件属性
	uint8	NTRes;              //保留给NT
	uint8	CrtTimeTenth;  	    //建立时间（fat16保留）

	uint16	CrtTime;           	//建立时间（fat16保留）
	uint16	CrtDate;            //建立日期（fat16保留）
	uint16	LstAccDate;      	//最后访问日期（fat16保留）
	uint16	FstClusHI;         	//起始簇号高两个字节（fat16保留）
	uint16	WrtTime;           	//最后写时间
	uint16	WrtDate;            //最后写日期
	uint16	FstClusLO;          //起始簇(cluster)号低两个字节
	uint32	FileSize;         	//文件大小
} FDT;


/***************************************************************************
长目录项(32字节)
***************************************************************************/
typedef __packed struct _LONG_FDT
{
    uint8 	Order;
    char 	Name1[10];
    uint8 	Attr;
    uint8 	Type;
    uint8 	Chksum;
    char 	Name2[12];
    uint16 	FstClusLO;
    char 	Name3[4];
}LONG_FDT;

//1全局变量
#undef	EXT
#ifdef	IN_FDT
		#define	EXT
#else
		#define	EXT		extern
#endif		
EXT		CACHE       FdtCache[FDT_CACHE_NUM];

//1函数原型声明
extern 	uint8 	ReadFDTInfo(FDT *Rt, uint8 Drive, uint32 SecIndex, uint16 ByteIndex);
extern 	uint8 	WriteFDTInfo(FDT *FDTData, uint8 Drive, uint32 SecIndex, uint16 ByteIndex);
extern 	uint8 	GetFDTInfo(FDT *Rt, uint8 Drive, uint32 ClusIndex, uint32 Index);
extern 	uint8   SetRootFDTInfo(uint8 Drive, uint32 Index, FDT *FDTData);
extern 	uint8 	SetFDTInfo(uint8 Drive, uint32 ClusIndex, uint32 Index, FDT *FDTData);
extern 	uint8 	FindFDTInfo(FDT *Rt, uint8 Drive, uint32 ClusIndex, char *FileName);
extern 	uint8 	AddFDT(uint8 Drive, uint32 ClusIndex, FDT *FDTData);
extern 	uint8 	ChangeFDT(uint8 Drive, uint32 ClusIndex, FDT *FDTData);
extern 	uint8 	DelFDT(uint8 Drive, uint32 ClusIndex, char *FileName);
extern 	uint8 	FDTIsLie(uint8 Drive, uint32 ClusIndex, char *FileName);
extern 	uint8   ClearClus(uint8 Drive, uint32 Index);
#endif

