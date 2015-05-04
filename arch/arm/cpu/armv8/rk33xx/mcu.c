/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/rkplat.h>


/* Max MCU's SRAM value is 8K, begin at (RKIO_IMEM_PHYS + 4K) */
#define MCU_SRAM_ADDR_BASE	(RKIO_IMEM_PHYS + (1024*4))


void rk_mcu_init(void)
{
	uint32_t pll_src, div;

	debug("rk mcu init\n");

#if defined(CONFIG_RKCHIP_RK3368)
	//mcu sam memory map to internel ram
	grf_writel((0xF << (4 + 16)) | ((MCU_SRAM_ADDR_BASE >> 28) << 4), GRF_SOC_CON14);
	grf_writel((0xFFFF << (0 + 16)) | ((MCU_SRAM_ADDR_BASE >> 12) << 0), GRF_SOC_CON11);

	//mcu clk: select gpll as source clock and div = 6
	pll_src = 1;
	div = (6 - 1);
	cru_writel((((1<<7)|0x1f)<<16)|(pll_src<<7)|div, CRU_CLKSELS_CON(12));

	//mcu dereset, for start running
	cru_writel(0x30000000, CRU_SOFTRSTS_CON(1));
#else
	#error "PLS config chiptype for mcu init!"
#endif
}

