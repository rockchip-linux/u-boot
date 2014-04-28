
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

void MMUDeinit()
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

#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	//ChipTypeCheck();
	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	printf("CPU:\t\tRK32xx\n");
	printf("arm pll:\t%d\n", rk_get_arm_pll());
	printf("general pll:\t%d\n", rk_get_general_pll());
	printf("codec pll:\t%d\n", rk_get_codec_pll());
	printf("ddr pll:\t%d\n", rk_get_ddr_pll());
	printf("new pll:\t%d\n", rk_get_new_pll());
	return 0;
}
#endif


void reset_cpu(ulong ignored)
{
    pFunc fp;

    disable_interrupts();
    //UsbSoftDisconnect();
    FW_NandDeInit();

    MMUDeinit();              /*关闭MMU*/
    //cruReg->CRU_MODE_CON = 0x33030000;    //cpu enter slow mode
    //Delay100cyc(10);
    g_giccReg->icceoir=INT_USB_OTG;
    //DisableRemap();
	ResetCpu((0xff740000));
    //cruReg->CRU_GLB_SRST_FST_VALUE = 0xfdb9; //kernel 使用 fst reset时，loader会死机，问题还没有查，所有loader还是用snd reset
    g_cruReg->cru_glb_srst_snd_value = 0xeca8; //soft reset
    Delay100cyc(10);
    while(1);
}


