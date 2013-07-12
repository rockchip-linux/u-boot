

#include "armlinux/config.h"

#ifdef DRIVERS_MMU
uint32 __CacheFlushDRegion(uint32 *adr, uint32 size)
{
}

void CacheFlushBoth(void)
{
}

void MMUDisable(void)
{
}


void CacheDisableBoth(void)
{
}

void MMUSetTTB(uint32 *ttb)
{
}

uint32 MMUSetProcessID(uint32 processID)
{
}

uint32 MMUSetDomain(uint32 id, uint32 domain)
{
}

void   MMUEnable(void)
{
}

void MMUFlushTLB(void)
{
}

void CacheEnableBoth(void)
{
}

void CacheInvBoth(void)
{
}
#endif


//uint8 ReadSecs(uint8 LUN, uint32 Index, void *buf, uint32 nSec)
//{
//}


