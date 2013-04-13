

#include "armlinux/config.h"

char RSA_KEY_DATA[];
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

void ResetCpu(unsigned long remap_addr)
{
}

uint8 ReadSecs(uint8 LUN, uint32 Index, void *buf, uint32 nSec)
{
}


