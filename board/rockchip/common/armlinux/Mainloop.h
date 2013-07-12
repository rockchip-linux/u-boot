/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    mainloop.h
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#ifndef _IN_MAIN
#define _IN_MAIN

/*******************************************************************
宏常数定义
*******************************************************************/


//1全局变量
#undef	EXT
#ifdef	IN_MAIN
		#define	EXT
#else
		#define	EXT		extern
#endif

//1函数原型声明
extern uint8* g_32secbuf;
extern uint8* g_cramfs_check_buf;

extern uint8* g_pIDBlock;
extern uint8* g_pLoader;
extern uint8* g_pReadBuf;
extern uint8* g_pFlashInfoData;
extern uint32 g_bootRecovery;
extern uint32 g_FwEndLba;

extern uint32 g_Rk29xxChip;
extern uint32 SecureBootEn;
extern uint32 SecureBootCheckOK;
extern uint32 g_BootRockusb;
extern uint32  SecureBootLock;
extern uint32  SecureBootLock_backup;

//extern void serial_init (void);
extern int BootSnapshot(PBootInfo pboot_info);

#endif

