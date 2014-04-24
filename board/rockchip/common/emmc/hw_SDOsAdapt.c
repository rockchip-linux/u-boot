/****************************************************************
//    CopyRight(C) 2008 by Rock-Chip Fuzhou
//      All Rights Reserved
//文件名:hw_SDOsAdapt.c
//描述:RK28 SD/MMC driver OS adaptation implement file
//作者:hcy
//创建日期:2008-11-08
//更改记录:
//当前版本:1.00
$Log: hw_SDOsAdapt.c,v $
Revision 1.3  2011/01/27 03:43:27  Administrator
*** empty log message ***

Revision 1.2  2011/01/21 10:12:56  Administrator
支持EMMC
优化buffer效率

Revision 1.1  2011/01/14 10:03:10  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 03:28:03  Administrator
*** empty log message ***

Revision 1.1.1.1  2010/05/17 03:44:52  hds
20100517 黄德胜提交初始版本

Revision 1.1.1.1  2010/03/06 05:22:59  zjd
2010.3.6由黄德胜提交初始版本

Revision 1.1.1.1  2009/12/15 01:46:31  zjd
20091215 杨永忠提交初始版本

Revision 1.2  2009/10/13 06:31:34  hcy
hcy 09-10-13 SD卡驱动更新，现有3种卡检测方式，优化了代码提高性能

Revision 1.1.1.1  2009/08/18 06:43:27  Administrator
no message

Revision 1.1.1.1  2009/08/14 08:02:01  Administrator
no message

Revision 1.3  2009/05/07 12:11:43  hcy
hcy 使用小卡座，卡热拔插，很容易出现读不到文件的问题

Revision 1.2  2009/04/23 14:02:40  hcy
hcy 改bug：创建事件和信号量函数有错

Revision 1.1.1.1  2009/03/16 01:34:07  zjd
20090316 邓训金提供初始SDK版本

Revision 1.4  2009/03/13 01:44:44  hcy
卡检测和卡电源管理改成GPIO方式

Revision 1.3  2009/03/11 03:33:38  hcy
驱动调整

Revision 1.2  2009/03/05 12:37:16  hxy
添加CVS版本自动注释

****************************************************************/
#ifndef DRIVER_ONLY
///#include    "include.h"
//#include    "support.h"
#endif
#include    "sdmmc_config.h"
#if (eMMC_PROJECT_LINUX) 
DECLARE_COMPLETION(rk29_emmc_xfer_done);
#endif
#ifdef DRIVERS_SDMMC

typedef struct TagSDOAM_EVENT
{
    uint8 *pName;
    uint32 event;
}SDOAM_EVENT_T;

typedef struct TagSDOAM_MUTEX
{
    uint8 *pName;
    uint32 mutex;
}SDOAM_MUTEX_T;

typedef struct TagSDOAM_MSG
{
    uint32 code;
    uint32 msg;
    struct TagSDOAM_MSG *pNext;
}SDOAM_MSG_T,*pSDOAM_MSG_T;

typedef struct TagSDOAM_MSG_MANAGER
{
    uint32 count;
    pSDOAM_MSG_T  pList;
}SDOAM_MSG_MANAGER_T;
#if EN_SD_PRINTF
typedef struct TagSCREEN_PRINTF_LIST
{
    uint8 *pStr;
    struct TagSCREEN_PRINTF_LIST *pNext;
}SCREEN_PRINTF_LIST_T;
#endif

static uint8 nEvent = 0;
static SDOAM_EVENT_T gEvent[2];
static uint8 nMutex = 0;
static SDOAM_MUTEX_T gMutex[2];
#if (eMMC_PROJECT_LINUX == 0) 
static SDOAM_MSG_MANAGER_T gMsg = {0, NULL};
#endif
#if EN_SD_PRINTF
static uint8 gSPrintfBuf[512] = {0};
static SCREEN_PRINTF_LIST_T *gpScreenPrintfHead = NULL;
#endif

void SDOAM_Delay(uint32 us)
{
#if (eMMC_PROJECT_LINUX) 
    udelay(us);
#else
    DRVDelayUs(us);
#endif	
}
#ifndef MALLOC_DISABLE
void *SDOAM_Malloc(uint32 nSize)
{ 
#ifdef LINUX
    return (kmalloc(nSize, GFP_KERNEL));
#else
    return malloc(nSize);
#endif
}

void SDOAM_Free(void *ptr)
{
#ifdef LINUX
     kfree(ptr);
#else
     free(ptr);
#endif 
}
#endif
#if EN_SD_PRINTF
//打印链表各个节点的顺序是，先调用Screen_printf的打印在节点前，后调用的在节点后
static void _AddToList(uint8 *pStr)
{
    SCREEN_PRINTF_LIST_T *pItem = NULL;
    uint8 *pBuf = NULL;

    if (gpScreenPrintfHead == NULL)
    {
        gpScreenPrintfHead = (SCREEN_PRINTF_LIST_T *)SDOAM_Malloc(sizeof(SCREEN_PRINTF_LIST_T));
        Assert((gpScreenPrintfHead != NULL), "ERROR:_AddToList mallco failed\n", 0);
        pBuf = (uint8 *)SDOAM_Malloc(strlen((char*)pStr) + 1);
        Assert((pBuf != NULL), "ERROR:_AddToList mallco failed\n", 0);
        strcpy((char*)pBuf,(char*)pStr);
        pBuf[strlen((char*)pStr)] = 0;
        gpScreenPrintfHead->pStr = pBuf;
        gpScreenPrintfHead->pNext = NULL;
        return;
    }

    pItem = gpScreenPrintfHead;
    while (pItem->pNext)
    {
        pItem = pItem->pNext;
    }
    pItem->pNext = (SCREEN_PRINTF_LIST_T *)SDOAM_Malloc(sizeof(SCREEN_PRINTF_LIST_T));
    Assert((pItem->pNext != NULL), "ERROR:_AddToList mallco failed\n", 0);
    pBuf = (uint8 *)SDOAM_Malloc(strlen((char*)pStr) + 1);
    Assert((pBuf != NULL), "ERROR:_AddToList mallco failed\n", 0);
    strcpy((char*)pBuf, (char*)pStr);
    pBuf[strlen((char*)pStr)] = 0;
    pItem = pItem->pNext;
    pItem->pStr = pBuf;
    pItem->pNext = NULL;
    return;
}

void  SDOAM_Printf(const char *fmt, ...)
{
#if EN_SD_PRINTF
    va_list arg;

    va_start(arg,fmt);
    vsprintf((char*)gSPrintfBuf, fmt, arg);
    va_end(arg);

    gSPrintfBuf[511] = 0x00;  //防止打印数据超长
    _AddToList(gSPrintfBuf);
#endif
}

void SDOAM_PrintfAll(void)
{
#if EN_SD_PRINTF
    SCREEN_PRINTF_LIST_T *pItem = NULL;
    uint8 *pStr = NULL;

    pItem = gpScreenPrintfHead;
    while (pItem != NULL)
    {
        pStr = pItem->pStr;
        PRINTF((char*)pStr);
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
#if 0//暂时屏蔽,测试用,在driver工程中; note by xbw

    pSDOAM_MSG_T pItem = NULL;
    pSDOAM_MSG_T pMalloc = NULL;
    #ifdef DRIVER_ONLY
    int          temp;
    int          backup;
    #else
    DECLARE_CUP_SR;
    #endif

    pMalloc = (pSDOAM_MSG_T)SDOAM_Malloc(sizeof(SDOAM_MSG_T));
    Assert((pMalloc != NULL), "SDOAM_SendMsg:malloc failed\n", 0);
    pMalloc->code = code;
    pMalloc->msg  = param;
    pMalloc->pNext = NULL;

    #ifdef DRIVER_ONLY
    __asm
    {
		MRS temp, CPSR
		MOV backup, temp
		ORR temp, temp, #0x80         
		MSR CPSR_c, temp
	}
    #else
    ENTER_CRITICAL();
    #endif
    if (gMsg.pList == NULL)
    {
        gMsg.pList = pMalloc;
        gMsg.count = 1;
        #ifdef DRIVER_ONLY
        __asm
        {      
    		MSR CPSR_c, backup
    	}
        #else
        EXIT_CRITICAL();
        #endif
        return;
    }

    pItem = gMsg.pList;
    while (pItem->pNext != NULL)
    {
        pItem = pItem->pNext;
    }

    pItem->pNext = pMalloc;
    gMsg.count++;
    #ifdef DRIVER_ONLY
    __asm
    {      
		MSR CPSR_c, backup
	}
    #else
    EXIT_CRITICAL();
    #endif

#endif

    return;
}

bool SDOAM_GetMsg(uint32 *pCode, uint32 *pMsg)
{
#if 0//暂时屏蔽,测试用,在driver工程中; note by xbw
//#ifndef LINUX
    pSDOAM_MSG_T pItem = NULL;
    uint32      *pTmp = NULL;
    #ifdef DRIVER_ONLY
    int          temp;
    int          backup;
    #else
    DECLARE_CUP_SR;
    #endif

    #ifdef DRIVER_ONLY
    __asm
    {
		MRS temp, CPSR
		MOV backup, temp
		ORR temp, temp, #0x80         
		MSR CPSR_c, temp
	}
    #else
    ENTER_CRITICAL();
    #endif
    pTmp = &gMsg.count;
    if (0 == *(volatile uint32 *)pTmp)
    {
        #ifdef DRIVER_ONLY
        __asm
        {      
    		MSR CPSR_c, backup
    	}
        #else
        EXIT_CRITICAL();
        #endif
        return FALSE;
    }

	
	
    pItem = gMsg.pList;
    Assert((pItem != NULL), "SDOAM_GetMsg:msg list NULL\n", gMsg.count);
    gMsg.count--;
    gMsg.pList = pItem->pNext;
    #ifdef DRIVER_ONLY
    __asm
    {      
		MSR CPSR_c, backup
	}
    #else
    EXIT_CRITICAL();
    #endif

    *pCode = pItem->code;
    *pMsg = pItem->msg;
    SDOAM_Free(pItem);
#endif
    return TRUE;
}

pMUTEX SDOAM_CreateMutex(uint8 *name)
{
#ifdef DRIVER_ONLY
    uint32 n;
    
    if(nMutex >= 2)
    {
        nMutex = 0;
    }
    gMutex[nMutex].pName = name;
    gMutex[nMutex].mutex = 1;
    n = nMutex;
    nMutex++;
    return (&gMutex[n].mutex);
#else
    uint32 n;

    if(nMutex >= 2)
    {
        nMutex = 0;
    }
    gMutex[nMutex].pName = name;
    gMutex[nMutex].mutex = 1;
    n = nMutex;
    nMutex++;
    return (&gMutex[n].mutex);
#endif
}

void SDOAM_RequestMutex(pMUTEX handle)
{
#if 0//暂时屏蔽,测试用,在driver工程中; note by xbw
#ifdef DRIVER_ONLY
    if ((handle != &gMutex[0].mutex) && (handle != &gMutex[1].mutex))
    {
        Assert(0, "SDOAM_RequestMutex:mutex handle error\n", (uint32)handle);
    }

    while (!(*(volatile uint32 *)handle))
    {
        SDOAM_Delay(1);
    }
    *handle = 0;
    return;
#else
    if (((ROCK_SEM_DATA *)handle != &gSD0_Sem) && ((ROCK_SEM_DATA *)handle != &gSD1_Sem))
    {
        Assert(0, "SDOAM_RequestMutex:mutex handle error\n", (uint32)handle);
    }
    RockSemObtain((ROCK_SEM_DATA *)handle);
#endif

#endif
}

void SDOAM_ReleaseMutex(pMUTEX handle)
{
#if 0//暂时屏蔽,测试用,在driver工程中; note by xbw

#ifdef DRIVER_ONLY
    if ((handle != &gMutex[0].mutex) && (handle != &gMutex[1].mutex))
    {
        Assert(0, "SDOAM_ReleaseMutex:mutex handle error\n", (uint32)handle);
    }

    if (!(*handle))
    {
        *handle = 1;
    }
    return;
#else
    if (((ROCK_SEM_DATA *)handle != &gSD0_Sem) && ((ROCK_SEM_DATA *)handle != &gSD1_Sem))
    {
        Assert(0, "SDOAM_ReleaseMutex:mutex handle error\n", (uint32)handle);
    }
    RockSemRelease((ROCK_SEM_DATA *)handle);
#endif

#endif
}

pEVENT SDOAM_CreateEvent(uint8 *name)
{
    uint32 n;
#if (eMMC_PROJECT_LINUX) 
    init_completion(&rk29_emmc_xfer_done);
#endif
    if(nEvent >= 2)
    {
        nEvent = 0;
    }
    gEvent[nEvent].pName = name;
    gEvent[nEvent].event = 0;
    n = nEvent; 
    nEvent++;
    return (&gEvent[n].event);
}

void SDOAM_SetEvent(pEVENT handle, uint32 event)
{
#if EN_SD_INT
#if (eMMC_PROJECT_LINUX) 
    complete(&rk29_emmc_xfer_done);
#else
    if ((handle != &gEvent[0].event) && (handle != &gEvent[1].event))
    {
        Assert(0, "SDOAM_SetEvent:event handle error\n", (uint32)handle);
        return;
    }
    *handle |= event;
#endif
#endif
}

void SDOAM_GetEvent(pEVENT handle, uint32 event)
{
#if EN_SD_INT
#if (eMMC_PROJECT_LINUX) 
    wait_for_completion(&rk29_emmc_xfer_done);
#else
    if ((handle != &gEvent[0].event) && (handle != &gEvent[1].event))
    {
        Assert(0, "SDOAM_GetEvent:event handle error\n", (uint32)handle);
    }
    while (!(*((volatile uint32 *)handle) & event))
    {
        SDOAM_Delay(1);
    }
    *handle &= ~(event);
    return;
#endif
#endif
}

void *SDOAM_Memcpy(void *dest, void *src, uint32 count)
{
    return memcpy(dest, src, count);
}

void *SDOAM_Memset(void *dest, uint8 ch, uint32 count)
{
    return memset(dest, ch, count);
}

int32 SDOAM_Memcmp(void *dest, void *src, uint32 count)
{
    return memcmp(dest, src, count);
}

int32 SDOAM_Strlen(uint8 *s)
{
    return strlen((char*)s);
}

char *SDOAM_Strcat(uint8 *dest, uint8 *src)
{
    return strcat((char*)dest, (char*)src);
}

#endif //end of #ifdef DRIVERS_SDMMC
