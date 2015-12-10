/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDOAM_H_
#define _SDOAM_H_

typedef uint32	*pEVENT;
typedef uint32	*pMUTEX;

#define MSG_CARD_INSERT    (0x1)
#define MSG_CARD_REMOVE    (0x2)
#define MSG_CARD_USEABLE   (0x3)

#define SDOAM_Memcpy ftl_memcpy
#define SDOAM_Memcmp ftl_memcmp
#define SDOAM_Memset ftl_memset

/****************************************************************
			对外函数声明
****************************************************************/
//void  *SDOAM_Malloc(uint32 nSize);
//void   SDOAM_Free(void *ptr);
//void   SDOAM_Printf(const char *fmt, ...);
void   SDOAM_PrintfAll(void);
void   SDOAM_SendMsg(uint32 code, uint32 param);
uint32   SDOAM_GetMsg(uint32 *pCode, uint32 *pMsg);
pMUTEX SDOAM_CreateMutex(void);
void   SDOAM_RequestMutex(pMUTEX handle);
void   SDOAM_ReleaseMutex(pMUTEX handle);
pEVENT SDOAM_CreateEvent(void);
void   SDOAM_SetEvent(pEVENT handle, uint32 event);
void   SDOAM_GetEvent(pEVENT handle, uint32 event);
void   SDOAM_Delay(uint32 us);
//void  *SDOAM_Memcpy(void *dest, void *src, uint32 count);
//void  *SDOAM_Memset(void *dest, uint8 ch, uint32 count);
int32  SDOAM_Strlen(uint8 *s);
char  *SDOAM_Strcat(uint8 *dest, uint8 *src);
/* only used for SDT */
//int32  SDOAM_Memcmp(void *dest, void *src, uint32 count);

#endif /* end of #ifndef _SDOAM_H_ */

#endif /* end of #ifdef DRIVERS_SDMMC */
