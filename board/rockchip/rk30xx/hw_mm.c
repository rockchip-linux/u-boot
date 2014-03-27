
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
#include <asm/arch/drivers.h>
//#include "hwapi_MMU.h"

#define DRIVERS_CACHE
//#include "hwapi_Cache.h"


void MMUDeinit(uint32 adr)
{

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

}

uint32 CacheFlushDRegion(uint32 adr, uint32 size)
{
#ifndef CONFIG_SYS_DCACHE_OFF
	flush_cache(adr, size);
#endif
}

/*uint32 CacheCleanDRegion(uint32 adr, uint32 size)
{
    __CacheCleanDRegion(adr,size);
#ifdef L2CACHE_ENABLE    
    l2x0_flush_range(adr, adr+size);
#endif
}*/


