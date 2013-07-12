/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    drive.h
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/

#ifndef _DRIVE_H
#define _DRIVE_H
//1可配置参数
#define 	MAX_DRIVE                   1   //最大支持的驱动器数

//1常量定义
#define 	DISK_SYS                    0
#define 	DISK_FLASH                  1
#define 	DISK_SD                     2

//1结构定义

/***************************************************************************
磁盘信息结构体
***************************************************************************/
typedef struct _DRIVE_INFO
{
    uint8   Valid;
    uint8   FATType;
    uint8 	LogSecPerClus;
	uint8	NumFATs;
	uint8	SecPerClus;
	uint16	BytsPerSec;
	uint16	ResvdSecCnt;
	uint16	RootEntCnt;
	uint32	RootClus;
	uint32	FSInfo;
	uint32  FATSz;
	uint32  RootDirSectors;
	uint32  FirstDataSector;
	uint32  PBRSector;
    uint32  PathClus;
	uint32  TotClus;
	uint32  TotSec;
    uint32  FreeClus;
    FDT_REF FdtRef;                 //FDT目录参考点, 用于快速定位
    SubDir  DirInfo[MAX_DIR_DEPTH]; //目录结构体信息
    uint8   DirDeep;
}DRIVE_INFO, *pDRIVE_INFO;


//1全局变量
#undef	EXT
#ifdef	IN_DRIVE
		#define	EXT
#else
		#define	EXT		extern
#endif
EXT		uint8           CurDrive;
//EXT		uint8           DriveLowFormat;
EXT		DRIVE_INFO      DriveInfo[MAX_DRIVE];

//1函数原型声明
extern	pDRIVE_INFO Mount(uint8 Drive);
extern	void        Demount(uint8 Drive);
extern	pDRIVE_INFO GetDriveInfo(uint8 Drive);
extern	uint8       GetDrive(char *Path);
extern	uint32      GetCapacity(uint8 Drive);
extern    uint8 WriteSecs(uint8 LUN, uint32 Index, void *buf, uint32 nSec);
extern    uint8 ReadSecs(uint8 LUN, uint32 Index, void *buf, uint32 nSec);
extern    void DRVDelayUs(uint32 us);


//1表格定义
#ifdef IN_DRIVE
#else
#endif
#endif

/*********************************************************************************************************
**                            End Of File
********************************************************************************************************/

