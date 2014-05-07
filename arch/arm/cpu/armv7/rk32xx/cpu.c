
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


#include <asm/arch/drivers.h>
#include <common.h>


#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif


uint32 CacheFlushDRegion(uint32 adr, uint32 size)
{
#ifndef CONFIG_SYS_DCACHE_OFF
	flush_cache(adr, size);
#endif
}

#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	rkclk_get_pll();

	printf("CPU:\t\tRK32xx\n");
	printf("arm pll:\t%d\n", rkclk_get_arm_pll());
	printf("general pll:\t%d\n", rkclk_get_general_pll());
	printf("codec pll:\t%d\n", rkclk_get_codec_pll());
	printf("ddr pll:\t%d\n", rkclk_get_ddr_pll());
	printf("new pll:\t%d\n", rkclk_get_new_pll());
	return 0;
}
#endif



