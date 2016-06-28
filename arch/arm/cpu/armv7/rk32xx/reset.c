/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKRESET_VERSION		"1.4"

extern void FW_NandDeInit(void);

#if defined(CONFIG_RKCHIP_RK322X)
void rk322x_cru_divide_adjust_for_kernel(void)
{
	uint32 con;

	/* bus */
	con =  cru_readl(CRU_CLKSELS_CON(0));
	cru_writel(((con & (~(0x1f << 8))) | (0x3 << 8)) | (0x1f << 24), CRU_CLKSELS_CON(0));

	/* nandc */
	con =  cru_readl(CRU_CLKSELS_CON(2));
	cru_writel(((con & (~(0x1f << 8))) | (0x5 << 8)) | (0x1f << 24), CRU_CLKSELS_CON(2));

	/* peri */
	con =  cru_readl(CRU_CLKSELS_CON(10));
	cru_writel(((con & (~(0x1f << 0))) | (0x3 << 0)) | (0x1f << 16), CRU_CLKSELS_CON(10));

	/* rga */
	con =  cru_readl(CRU_CLKSELS_CON(22));
	cru_writel(((con & (~(0x1f << 0))) | (0x3 << 0)) | (0x1f << 16), CRU_CLKSELS_CON(22));
	con =  cru_readl(CRU_CLKSELS_CON(33));
	cru_writel(((con & (~(0x1f << 8))) | (0x3 << 8)) | (0x1f << 24), CRU_CLKSELS_CON(33));

	/* cabac */
	con =  cru_readl(CRU_CLKSELS_CON(28));
	cru_writel(((con & (~(0x1f << 8))) | (0x3 << 8)) | (0x1f << 24), CRU_CLKSELS_CON(28));

	/*rkved*/
	con =  cru_readl(CRU_CLKSELS_CON(28));
	cru_writel(((con & (~(0x1f << 0))) | (0x3 << 0)) | (0x1f << 16), CRU_CLKSELS_CON(28));
	con =  cru_readl(CRU_CLKSELS_CON(34));
	cru_writel(((con & (~(0x1f << 8))) | (0x3 << 8)) | (0x1f << 24), CRU_CLKSELS_CON(34));

	/* iep */
	con =  cru_readl(CRU_CLKSELS_CON(29));
	cru_writel(((con & (~(0x1f << 0))) | (0x3 << 0)) | (0x1f << 16), CRU_CLKSELS_CON(29));

	/*vpu*/
	con =  cru_readl(CRU_CLKSELS_CON(32));
	cru_writel(((con & (~(0x1f << 0))) | (0x3 << 0)) | (0x1f << 16), CRU_CLKSELS_CON(32));

	/* vop */
#if 0
	con =  cru_readl(CRU_CLKSELS_CON(33));
	cru_writel(((con & (~(0x1f << 0))) | (0x3 << 0)) | (0x1f << 16), CRU_CLKSELS_CON(33));
#endif

	/* gpu */
	con =  cru_readl(CRU_CLKSELS_CON(34));
	cru_writel(((con & (~(0x1f << 0))) | (0x3 << 0)) | (0x1f << 16), CRU_CLKSELS_CON(34));
}
#endif /* CONFIG_RKCHIP_RK322X */


void rk_module_deinit(void)
{
	/* i2c deinit */
	rkcru_i2c_soft_reset();

	/* rk pl330 dmac deinit */
#ifdef CONFIG_RK_PL330_DMAC
	rk_pl330_dmac_deinit_all();
#endif /* CONFIG_RK_PL330_DMAC */

#ifdef CONFIG_RK_SDCARD_BOOT_EN
	rkcru_mmc_soft_reset(0);
	irq_handler_disable(IRQ_SDMMC);
#endif
	/* emmc disable tunning */
	rkclk_disable_mmc_tuning(2);

#if defined(CONFIG_RKCHIP_RK322X)
	rk322x_cru_divide_adjust_for_kernel();
#endif
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

	/* cpu soft reset */
	rkcru_cpu_soft_reset();
}
