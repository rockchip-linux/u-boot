/*
 * (C) Copyright 2002-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef	__ASM_GBL_DATA_H
#define __ASM_GBL_DATA_H

#ifdef CONFIG_OMAP
#include <asm/omap_boot.h>
#endif

/* Architecture-specific global data */
struct arch_global_data {
#if defined(CONFIG_FSL_ESDHC)
	u32 sdhc_clk;
#endif
#ifdef CONFIG_AT91FAMILY
	/* "static data" needed by at91's clock.c */
	unsigned long	cpu_clk_rate_hz;
	unsigned long	main_clk_rate_hz;
	unsigned long	mck_rate_hz;
	unsigned long	plla_rate_hz;
	unsigned long	pllb_rate_hz;
	unsigned long	at91_pllb_usb_init;
#endif

#ifdef CONFIG_ROCKCHIP
	unsigned long	chiptype;
	unsigned long	cpuversion;
#ifdef CONFIG_RK_FB_DDREND
	unsigned long	ddr_end;
#endif

#ifdef CONFIG_RK_CLOCK
	/* "static data" needed by rk's clock.c */
#if defined(CONFIG_RKCHIP_RK3288)
	unsigned long	cpu_mp_rate_hz;
	unsigned long	cpu_m0_rate_hz;
	unsigned long	cpu_l2ram_rate_hz;

	unsigned long	aclk_periph_rate_hz;
	unsigned long	pclk_periph_rate_hz;
	unsigned long	hclk_periph_rate_hz;

	unsigned long	aclk_bus_rate_hz;
	unsigned long	pclk_bus_rate_hz;
	unsigned long	hclk_bus_rate_hz;
#elif defined(CONFIG_RKCHIP_RK3368) || defined(CONFIG_RKCHIP_RK322X) || defined(CONFIG_RKCHIP_RK3366)\
	|| defined(CONFIG_RKCHIP_RK322XH)
	unsigned long	aclk_periph_rate_hz;
	unsigned long	pclk_periph_rate_hz;
	unsigned long	hclk_periph_rate_hz;

	unsigned long	aclk_bus_rate_hz;
	unsigned long	pclk_bus_rate_hz;
	unsigned long	hclk_bus_rate_hz;
#elif defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	unsigned long	aclk_cpu_rate_hz;
	unsigned long	pclk_cpu_rate_hz;
	unsigned long	hclk_cpu_rate_hz;

	unsigned long	aclk_periph_rate_hz;
	unsigned long	pclk_periph_rate_hz;
	unsigned long	hclk_periph_rate_hz;
#elif defined(CONFIG_RKCHIP_RK3399)
	unsigned long	aclk_periph_h_rate_hz;
	unsigned long	pclk_periph_h_rate_hz;
	unsigned long	hclk_periph_h_rate_hz;

	unsigned long	aclk_periph_l0_rate_hz;
	unsigned long	pclk_periph_l0_rate_hz;
	unsigned long	hclk_periph_l0_rate_hz;

	unsigned long	pclk_periph_l1_rate_hz;
	unsigned long	hclk_periph_l1_rate_hz;
#else
	#error "PLS config chiptype for clock!"
#endif

#endif /* CONFIG_RK_CLOCK */
#endif /* CONFIG_ROCKCHIP */
	/* "static data" needed by most of timer.c on ARM platforms */
	unsigned long timer_rate_hz;
	unsigned long tbu;
	unsigned long tbl;
	unsigned long lastinc;
	unsigned long long timer_reset_value;
#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	unsigned long tlb_addr;
	unsigned long tlb_size;
#endif

#ifdef CONFIG_OMAP
	struct omap_boot_parameters omap_boot_params;
#endif
#ifdef CONFIG_ROCKCHIP
	unsigned long rk_global_buf_addr;
	unsigned long rk_boot_buf_addr;
#endif
#ifdef CONFIG_CMD_FASTBOOT
	unsigned long fastboot_buf_addr;
	unsigned long fastboot_log_buf_addr;
#endif
};

#include <asm-generic/global_data.h>

#ifdef __clang__

#define DECLARE_GLOBAL_DATA_PTR
#define gd	get_gd()

static inline gd_t *get_gd(void)
{
	gd_t *gd_ptr;

#ifdef CONFIG_ARM64
	/*
	 * Make will already error that reserving x18 is not supported at the
	 * time of writing, clang: error: unknown argument: '-ffixed-x18'
	 */
	__asm__ volatile("mov %0, x18\n" : "=r" (gd_ptr));
#else
	__asm__ volatile("mov %0, r9\n" : "=r" (gd_ptr));
#endif

	return gd_ptr;
}

#else

#ifdef CONFIG_ARM64
#define DECLARE_GLOBAL_DATA_PTR		register volatile gd_t *gd asm ("x18")
#else
#define DECLARE_GLOBAL_DATA_PTR		register volatile gd_t *gd asm ("r9")
#endif
#endif

#endif /* __ASM_GBL_DATA_H */
