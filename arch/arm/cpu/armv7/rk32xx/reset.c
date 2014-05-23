/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;


#define RKRESET_VERSION		"1.2"


void rk_module_deinit(void)
{
#ifdef CONFIG_RK_I2C
	// soft reset i2c0 - i2c5
	writel(0x3f<<10 | 0x3f<<(10+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
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

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
        /* disable remap */
	/* rk3288 address remap control bit: SGRF soc con0 bit 11 */
        writel(1 << (11 + 16), RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON0);

	/* pll enter slow mode */
	writel(PLL_MODE_SLOW(APLL_ID) | PLL_MODE_SLOW(GPLL_ID) | PLL_MODE_SLOW(CPLL_ID) | PLL_MODE_SLOW(NPLL_ID), RKIO_GRF_PHYS + CRU_MODE_CON);

	/* soft reset */
	writel(0xeca8, RKIO_CRU_PHYS + CRU_GLB_SRST_SND);
#else
	#error "PLS config platform for reset.c!"
#endif /* CONFIG_RKPLATFORM */
}

