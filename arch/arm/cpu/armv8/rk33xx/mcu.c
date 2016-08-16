/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/rkplat.h>


/* Max MCU's SRAM value is 8K, begin at (RKIO_IMEM_PHYS + 4K) */
#define MCU_SRAM_ADDR_BASE	(RKIO_IMEM_PHYS + (1024*4))
/* exsram may using by mcu to accessing dram(0x0-0x20000000) */
#define MCU_EXSRAM_ADDR_BASE	(0)
/* experi no used, reserved value = 0 */
#define MCU_EXPERI_ADDR_BASE	(0)

void rk_mcu_init(void)
{
	debug("rk mcu init\n");

#if defined(CONFIG_RKCHIP_RK3368) || defined(CONFIG_RKCHIP_RK3366)
	uint32_t pll_src, div;

	/* mcu sam memory map to internel ram */
#ifdef CONFIG_RKCHIP_RK3368
	grf_writel((0xF << (4 + 16)) | ((MCU_SRAM_ADDR_BASE >> 28) << 4), GRF_SOC_CON14);
	grf_writel((0xFFFF << (0 + 16)) | ((MCU_SRAM_ADDR_BASE >> 12) << 0), GRF_SOC_CON11);

	grf_writel((0xF << (8 + 16)) | ((MCU_EXSRAM_ADDR_BASE >> 28) << 8), GRF_SOC_CON14);
	grf_writel((0xFFFF << (0 + 16)) | ((MCU_EXSRAM_ADDR_BASE >> 12) << 0), GRF_SOC_CON12);

	grf_writel((0xF << (0xc + 16)) | ((MCU_EXPERI_ADDR_BASE >> 28) << 0xc), GRF_SOC_CON14);
	grf_writel((0xFFFF << (0 + 16)) | ((MCU_EXPERI_ADDR_BASE >> 12) << 0), GRF_SOC_CON13);
#elif CONFIG_RKCHIP_RK3366
	grf_writel((0xF << (4 + 16)) | ((MCU_SRAM_ADDR_BASE >> 28) << 4), PMU_GRF_SOC_CON7);
	grf_writel((0xFFFF << (0 + 16)) | ((MCU_SRAM_ADDR_BASE >> 12) << 0), PMU_GRF_SOC_CON4);
#endif

	/* mcu clk: select gpll as source clock and div = 6 */
	pll_src = 1;
	div = (6 - 1);
	cru_writel((((1 << 7) | 0x1f) << 16) | (pll_src << 7) | div, CRU_CLKSELS_CON(12));

	/* mcu dereset, for start running */
	cru_writel(0x30000000, CRU_SOFTRSTS_CON(1));

#elif CONFIG_RKCHIP_RK3399
	/*
	 * 1. mem remap related sgrf operation, so moved it to miniloader;
	 * 2. moved clock configuration to clock-rk3399.c
	 */
#ifdef CONFIG_PERILP_MCU
	/* perilp m0 dereset */
	cru_writel(0x00160000, CRU_SOFTRSTS_CON(11));
#endif

#ifdef CONFIG_PMU_MCU
	/* pmu m0 dereset */
	writel(0x002c0000, RKIO_PMU_CRU_PHYS + PMUCRU_SOFTRSTS_CON(0));
#endif

#else
	#error "PLS config chiptype for mcu init!"
#endif
}
