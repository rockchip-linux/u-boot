
/******************************************************************/
/*  Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.       */
/*******************************************************************
File     :  MM.c
Desc   :    内存管理文件
Author :  nizy
Date     :  2008-11-20
Notes  :
$Log     :  Revision 1.00  2009/02/14   nizy
********************************************************************/

//#include "drivers.h"
#include "../../armlinux/config.h"
#include "hwapi_MMU.h"

#define DRIVERS_CACHE
#include "hwapi_Cache.h"

/*
    自动生成TLB表。0x60000000~0x68000000为cache able
*/
void MMUCreateTlb(INT32U * TlbAddr)
{
#ifdef DRIVERS_MMU
    int i;
//#if(PALTFORM==RK29XX)
    *TlbAddr++ = 0x00000c0E; // i == 0
    for(i=1;i<0x1000;i++)
    {
        if( i >= 0x600 && i < 0x604)  // loader 部分不使用 write back,usb就不用处理clean
            *TlbAddr++ = 0x00000c0A+0x00100000*i;
        else if( i >= 0x600 && i <= 0x9F0 ) //其他空间使用write back，crc32运算速度可以快4倍
            *TlbAddr++ = 0x00000c0E + 0x00100000*i;
        else
            *TlbAddr++ = 0x00000c12+0x00100000*i;
    }
/*#elif(PALTFORM==RK30XX)
    *TlbAddr++ = 0x00000c0E; // i == 0 使用 write back
    for(i=1;i<0x1000;i++)
    {
        if( i >= 0x600 && i <= 0x63F )
            *TlbAddr++ = 0x00000c0E+0x00100000*i;// kernel 的部分空间 不使用 write back
        else if( i >= 0x640 && i <= 0x9F0 )
            *TlbAddr++ = 0x00000c0A+0x00100000*i;// loader 的bss部分不使用 write back,usb就不用处理clean
        else
            *TlbAddr++ = 0x00000c12+0x00100000*i; //c12
    }
#endif*/
#endif
}
/*----------------------------------------------------------------------
Name   : MMUInit
Desc   : MMU相关初始化
Params :
Return :
Notes  :
----------------------------------------------------------------------*/
void MMUInit(uint32 adr)
{
#ifdef DRIVERS_MMU
    MMUDisable();              /*关闭MMU*/
    CacheDisableBoth();        /*关闭所有cache*/
    MMUSetTTB((uint32*)adr);            /*设置页表在内存的首地址*/
    MMUSetProcessID(0);        /*设置FCSE的转换参数(没有转换)*/
    MMUSetDomain(0, CLIENT);   /*设置区域0的管理权限(用户模式)*/
    CacheInvBoth();           /*清除所有cache内所有内容*/
    CacheEnableBoth();         /*打开所有cache*/
    MMUEnable();               /*打开MMU*/
    MMUFlushTLB();             /*清除TLB中的PTE*/
#ifdef L2CACHE_ENABLE  
    //L2x0Deinit();
    L2x0Init();
#endif
#endif
}

void MMUDeinit(uint32 adr)
{
#if 0

#ifdef DRIVERS_MMU
#ifdef L2CACHE_ENABLE    
    L2x0Deinit();
#endif
    CacheFlushBoth();
    MMUDisable();              /*关闭MMU*/
    CacheDisableBoth();        /*关闭所有cache*/
#endif

#else

#ifndef CONFIG_SYS_L2CACHE_OFF
	v7_outer_cache_disable();
#endif
#ifndef CONFIG_SYS_DCACHE_OFF
	flush_dcache_all();
#endif
#ifndef CONFIG_SYS_ICACHE_OFF
	invalidate_icache_all();
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
	dcache_disable();
#endif

#ifndef CONFIG_SYS_ICACHE_OFF
	icache_disable();
#endif

#endif
}

uint32 CacheFlushDRegion(uint32 adr, uint32 size)
{
#if 0

#ifdef DRIVERS_MMU
    __CacheFlushDRegion(adr,size);
#ifdef L2CACHE_ENABLE    
    l2x0_flush_range(adr, adr+size);
#endif
#endif

#else
#ifndef CONFIG_SYS_DCACHE_OFF
	flush_cache(adr, size);
#endif

#endif
}

/*uint32 CacheCleanDRegion(uint32 adr, uint32 size)
{
    __CacheCleanDRegion(adr,size);
#ifdef L2CACHE_ENABLE    
    l2x0_flush_range(adr, adr+size);
#endif
}*/


