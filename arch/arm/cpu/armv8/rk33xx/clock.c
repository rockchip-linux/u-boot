/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKCLOCK_VERSION		"1.3"


#ifdef CONFIG_RK_CLOCK

/* define clock sourc div */
#define CLK_DIV_1		1
#define CLK_DIV_2		2
#define CLK_DIV_3		3
#define CLK_DIV_4		4
#define CLK_DIV_5		5
#define CLK_DIV_6		6
#define CLK_DIV_7		7
#define CLK_DIV_8		8
#define CLK_DIV_9		9
#define CLK_DIV_10		10
#define CLK_DIV_11		11
#define CLK_DIV_12		12
#define CLK_DIV_13		13
#define CLK_DIV_14		14
#define CLK_DIV_15		15
#define CLK_DIV_16		16
#define CLK_DIV_17		17
#define CLK_DIV_18		18
#define CLK_DIV_19		19
#define CLK_DIV_20		20
#define CLK_DIV_21		21
#define CLK_DIV_22		22
#define CLK_DIV_23		23
#define CLK_DIV_24		24
#define CLK_DIV_25		25
#define CLK_DIV_26		26
#define CLK_DIV_27		27
#define CLK_DIV_28		28
#define CLK_DIV_29		29
#define CLK_DIV_30		30
#define CLK_DIV_31		31
#define CLK_DIV_32		32


/* pll set callback function */
struct pll_clk_set;
typedef void (*pll_callback_f)(struct pll_clk_set *clkset);


/****************************************************************************
Internal sram us delay function
Cpu highest frequency is 1 GHz
1 cycle = 1/1 ns
1 us = 1000 ns = 1000 * 1 cycles = 1000 cycles
*****************************************************************************/
static inline uint64_t arch_counter_get_cntpct(void)
{
	uint64_t cval;

	isb();
	__asm__ volatile ("mrs %0, cntpct_el0" : "=r" (cval));

	return cval;
}

static void clk_loop_delayus(uint32_t us)
{
	uint64_t orig;
	uint64_t to_wait = 24 * us;

	/* Note: u32 math is way more than enough for our small delays */
	orig = arch_counter_get_cntpct();
	while (arch_counter_get_cntpct() - orig <= to_wait)
		;
}

/*
 * rkplat calculate child clock div from parent
 * clk_parent: parent clock rate (HZ)
 * clk_child: child clock request rate (HZ)
 * even: if div needs even
 * return value: div
 */
static uint32 rkclk_calc_clkdiv(uint32 clk_parent, uint32 clk_child, uint32 even)
{
	uint32 div = 0;

	div = (clk_parent + (clk_child - 1)) / clk_child;

	if (even)
		div += (div % 2);

	return div;
}


#if defined(CONFIG_RKCHIP_RK3368)
	#include "clock-rk3368.c"
#elif defined(CONFIG_RKCHIP_RK3366)
	#include "clock-rk3366.c"
#elif defined(CONFIG_RKCHIP_RK3399)
	#include "clock-rk3399.c"
#elif defined(CONFIG_RKCHIP_RK322XH)
	#include "clock-rk322xh.c"
#else
	#error "PLS config chiptype for clock-rkxx.c!"
#endif


#else

void rkclk_pll_mode(int pll_mode) {}
void rkclk_set_pll(void) {}
void rkclk_get_pll(void) {}
void rkclk_dump_pll(void) {}
void rkclk_set_pll_rate_by_id(enum rk_plls_id pll_id, uint32 mHz) {}
uint32 rkclk_get_pll_rate_by_id(enum rk_plls_id pll_id) { return 24 * MHZ; }
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz) { return 0; }
int rkclk_lcdc_dclk_pll_sel(uint32 lcdc_id, uint32 pll_sel) {};
uint32 rkclk_get_sdhci_emmc_clk(void) { return 24 * MHZ; }
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src) {}
uint32 rkclk_get_mmc_clk(uint32 sdid) { return 24 * MHZ; }
uint32 rkclk_get_mmc_freq_from_gpll(uint32 sdid) { return 24 * MHZ; }
int rkclk_set_nandc_freq_from_gpll(uint32 nandc_id, uint32 freq) { return 0; }
int rkclk_set_mmc_clk_div(uint32 sdid, uint32 div) { return 0; }
int rkclk_set_mmc_tuning(uint32 sdid, uint32 degree, uint32 delay_num) { return 0; }
int rkclk_disable_mmc_tuning(uint32 sdid) { return 0; }
int32 rkclk_set_mmc_clk_freq(uint32 sdid, uint32 freq) { return 0; }
unsigned int rkclk_get_pwm_clk(uint32 pwm_id) { return 0; }
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id) { return 0; }
unsigned int rkclk_get_spi_clk(uint32 spi_bus) { return 0; }
#ifdef CONFIG_SECUREBOOT_CRYPTO
void rkclk_set_crypto_clk(uint32 rate) {}
#endif /* CONFIG_SECUREBOOT_CRYPTO*/
void rkcru_cpu_soft_reset(void) {}
void rkcru_mmc_soft_reset(uint32 sdmmcId) {}
void rkcru_i2c_soft_reset(void) {}
void rkclk_set_saradc_clk(void) {}

#endif /* CONFIG_RK_CLOCK */
