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
