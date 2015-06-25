/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKRESET_VERSION		"1.3"

extern void FW_NandDeInit(void);


void rk_module_deinit(void)
{
#ifdef CONFIG_RK_I2C

#if defined(CONFIG_RKCHIP_RK3368)
	// soft reset i2c0 - i2c5
	writel(0x3f<<10 | 0x3f<<(10+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
	mdelay(1);
	writel(0x00<<10 | 0x3f<<(10+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
#else
	#error "PLS config platform for i2c reset!"
#endif

#endif /* CONFIG_RK_I2C */

	/* rk pl330 dmac deinit */
#ifdef CONFIG_RK_DMAC
#ifdef CONFIG_RK_DMAC_0
	rk_pl330_dmac_deinit(0);
#endif
#ifdef CONFIG_RK_DMAC_1
	rk_pl330_dmac_deinit(1);
#endif
#endif /* CONFIG_RK_DMAC*/

	/* emmc disable tunning */
	rkclk_disable_mmc_tuning(2);
}


/*
 * Reset the cpu by setting up the watchdog timer and let him time out.
 */

void reset_cpu(ulong ignored)
{
	disable_interrupts();
	FW_NandDeInit();

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

#if defined(CONFIG_RKCHIP_RK3368)
	/* pll enter slow mode */
	cru_writel(((0x00 << 8) && (0x03 << 24)), PLL_CONS(APLLB_ID, 3));
	cru_writel(((0x00 << 8) && (0x03 << 24)), PLL_CONS(APLLL_ID, 3));
	cru_writel(((0x00 << 8) && (0x03 << 24)), PLL_CONS(GPLL_ID, 3));
	cru_writel(((0x00 << 8) && (0x03 << 24)), PLL_CONS(CPLL_ID, 3));
	cru_writel(((0x00 << 8) && (0x03 << 24)), PLL_CONS(NPLL_ID, 3));

	/* soft reset */
	writel(0xeca8, RKIO_CRU_PHYS + CRU_GLB_SRST_SND);
#else
	#error "PLS config platform for reset.c!"
#endif /* CONFIG_RKPLATFORM */
}

