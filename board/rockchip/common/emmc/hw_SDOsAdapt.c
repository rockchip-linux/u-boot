/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include    "sdmmc_config.h"

#ifdef DRIVERS_SDMMC

typedef struct TagSDOAM_EVENT {
	//uint8 *pName;
	uint32 event;
} SDOAM_EVENT_T;

typedef struct TagSDOAM_MUTEX {
	//uint8 *pName;
	uint32 mutex;
} SDOAM_MUTEX_T;

#if EN_SD_PRINTF
typedef struct TagSCREEN_PRINTF_LIST {
	uint8 *pStr;
	struct TagSCREEN_PRINTF_LIST *pNext;
} SCREEN_PRINTF_LIST_T;
#endif

static uint8 nEvent;
static SDOAM_EVENT_T gEvent[2];
static uint8 nMutex;
static SDOAM_MUTEX_T gMutex[2];

#if EN_SD_PRINTF
static uint8 gSPrintfBuf[512] = {0};
static SCREEN_PRINTF_LIST_T *gpScreenPrintfHead;
#endif

void SDOAM_Delay(uint32 us)
{
	DRVDelayUs(us);
}

#if EN_SD_PRINTF
#ifndef MALLOC_DISABLE
void *SDOAM_Malloc(uint32 nSize)
{
	return malloc(nSize);
}

void SDOAM_Free(void *ptr)
{
	free(ptr);
}
#endif
#endif

#if EN_SD_PRINTF
/* 打印链表各个节点的顺序是，先调用Screen_printf的打印在节点前，后调用的在节点后 */
static void _AddToList(uint8 *pStr)
{
	SCREEN_PRINTF_LIST_T *pItem = NULL;
	uint8 *pBuf = NULL;

	if (gpScreenPrintfHead == NULL) {
		gpScreenPrintfHead = (SCREEN_PRINTF_LIST_T *)SDOAM_Malloc(sizeof(SCREEN_PRINTF_LIST_T));
		Assert((gpScreenPrintfHead != NULL), "ERROR:_AddToList mallco failed\n", 0);
		pBuf = (uint8 *)SDOAM_Malloc(strlen((char *)pStr) + 1);
		Assert((pBuf != NULL), "ERROR:_AddToList mallco failed\n", 0);
		strcpy((char *)pBuf, (char *)pStr);
		pBuf[strlen((char *)pStr)] = 0;
		gpScreenPrintfHead->pStr = pBuf;
		gpScreenPrintfHead->pNext = NULL;
		return;
	}

	pItem = gpScreenPrintfHead;
	while (pItem->pNext)
		pItem = pItem->pNext;
	pItem->pNext = (SCREEN_PRINTF_LIST_T *)SDOAM_Malloc(sizeof(SCREEN_PRINTF_LIST_T));
	Assert((pItem->pNext != NULL), "ERROR:_AddToList mallco failed\n", 0);
	pBuf = (uint8 *)SDOAM_Malloc(strlen((char *)pStr) + 1);
	Assert((pBuf != NULL), "ERROR:_AddToList mallco failed\n", 0);
	strcpy((char *)pBuf, (char *)pStr);
	pBuf[strlen((char *)pStr)] = 0;
	pItem = pItem->pNext;
	pItem->pStr = pBuf;
	pItem->pNext = NULL;
}

void  SDOAM_Printf(const char *fmt, ...)
{
#if EN_SD_PRINTF
	va_list arg;

	va_start(arg, fmt);
	vsprintf((char *)gSPrintfBuf, fmt, arg);
	va_end(arg);

	gSPrintfBuf[511] = 0x00; /* 防止打印数据超长 */
	_AddToList(gSPrintfBuf);
#endif
}

void SDOAM_PrintfAll(void)
{
#if EN_SD_PRINTF
	SCREEN_PRINTF_LIST_T *pItem = NULL;
	uint8 *pStr = NULL;

	pItem = gpScreenPrintfHead;
	while (pItem != NULL) {
		pStr = pItem->pStr;
		PRINTF((char *)pStr, "");
		SDOAM_Free(pStr);
		SDOAM_Free(pItem);
		gpScreenPrintfHead = pItem->pNext;
		pItem = gpScreenPrintfHead;
	}
#endif
}
#endif

void SDOAM_SendMsg(uint32 code, uint32 param)
{
	return;
}

uint32 SDOAM_GetMsg(uint32 *pCode, uint32 *pMsg)
{
	return TRUE;
}

pMUTEX SDOAM_CreateMutex(void)
{
	uint32 n;

	if (nMutex >= 2)
		nMutex = 0;
	//gMutex[nMutex].pName = name;
	gMutex[nMutex].mutex = 1;
	n = nMutex;
	nMutex++;
	return &gMutex[n].mutex;
}

void SDOAM_RequestMutex(pMUTEX handle)
{

}

void SDOAM_ReleaseMutex(pMUTEX handle)
{

}

pEVENT SDOAM_CreateEvent(void)
{
	uint32 n;

	if (nEvent >= 2)
		nEvent = 0;
	//gEvent[nEvent].pName = name;
	gEvent[nEvent].event = 0;
	n = nEvent;
	nEvent++;
	return &gEvent[n].event;
}

void SDOAM_SetEvent(pEVENT handle, uint32 event)
{
#if EN_SD_INT
	if ((handle != &gEvent[0].event) && (handle != &gEvent[1].event)) {
		Assert(0, "SDOAM_SetEvent:event handle error\n", (uint32)handle);
		return;
	}
	*handle |= event;
#endif
}

void SDOAM_GetEvent(pEVENT handle, uint32 event)
{
#if EN_SD_INT
	if ((handle != &gEvent[0].event) && (handle != &gEvent[1].event)) {
		Assert(0, "SDOAM_GetEvent:event handle error\n", (uint32)handle);
	}
	while (!(*((volatile uint32 *)handle) & event))
		SDOAM_Delay(1);
	*handle &= ~(event);
#endif
}

#endif /* end of #ifdef DRIVERS_SDMMC */
