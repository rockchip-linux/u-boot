/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/

#include <common.h>
#include <version.h>
#include <asm/arch/typedef.h>
#include <asm/arch/iomap.h>
#include <asm/arch/grf.h>
#include <asm/arch/cru.h>
#include <asm/arch/cpu.h>
#include <asm/arch/reg.h>



DECLARE_GLOBAL_DATA_PTR;



/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	/* Set Initial global variables */

	gd->bd->bi_arch_number = MACH_TYPE_RK30XX;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x88000;

	return 0;
}


/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
	gd->ram_size = get_ram_size(
			(void *)CONFIG_SYS_SDRAM_BASE,
			CONFIG_SYS_SDRAM_SIZE);

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
}

#ifdef CONFIG_OF_LIBFDT
extern uint32 ddr_get_cap(void);
int rk_fixup_memory_banks(void *blob, u64 start[], u64 size[], int banks) {
    //TODO:auto detect size.
    if (banks > 0){
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
        size[0] = ddr_get_cap();
#else
	size[0] = 0x40000000;//1G for now
#endif
    }
    return fdt_fixup_memory_banks(blob, start, size, banks);
}
void board_lmb_reserve(struct lmb *lmb) {
    //reserve 48M for kernel & 8M for nand api.
    lmb_reserve(lmb, gd->bd->bi_dram[0].start, 56 * 1024 * 1024);
}

#endif



#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif



