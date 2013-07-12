/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    chkdsk.h
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/

#ifndef _CHKDSK_H
#define _CHKDSK_H

//1可配置参数
#define 	MAX_NESTING_LEVEL		16 
#define  	MAX_CACHES   			16

//1常量定义
#define  	GET_BIT     			0
#define  	SET_BIT     			1
#define  	FREE_BIT    			2
#define 	READCOUNTER         	105
#define 	WRITECOUNTER        	100

//1结构定义
typedef struct 
{
	uint16 Valid;
	uint16 Write;
	uint16 Cnt;
	uint32 Sector;
	uint32 DirClusCnt;
	uint8 *buf;
} _CacheChkdsk;

//1全局变量
#undef	EXT
#ifdef	IN_CHKDSK
		#define	EXT
#else
		#define	EXT		extern
#endif		

EXT     uint8 			FATClusMap[8192];
EXT     uint16 			FileCorrupted;
EXT     uint8 			Nestinglevel;
EXT     _CacheChkdsk 	CacheChkdsk[MAX_CACHES];


//1函数原型声明
extern uint16 Chkdsk(uint8 Drive);

#ifdef	IN_CHKDSK
        uint8 BitMask[8]=
        {
        	0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080
        };
#else
		extern	uint8 	BitMask[];

#endif

#endif
