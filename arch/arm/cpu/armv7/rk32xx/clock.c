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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#define RKCLOCK_VERSION		"1.2"


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
#define LPJ_1000MHZ  100UL

static void clk_loop_delayus(uint32_t us)
{   
	volatile uint32_t i;

	/* copro seems to need some delay between reading and writing */
	for (i = 0; i < LPJ_1000MHZ * us; i++) {
		nop();
	}
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

	if (even) {
		div += (div % 2);
	}

	return div;
}


#if defined(CONFIG_RKCHIP_RK3288)
	#include "clock-rk3288.c"
#elif defined(CONFIG_RKCHIP_RK3036)
	#include "clock-rk3036.c"
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#include "clock-rk312X.c"
#else
	#error "PLS config chiptype for clock-rkxx.c!"
#endif


#else

void rkclk_pll_mode(int pll_id, int pll_mode) {}
void rkclk_set_pll(void) {}
void rkclk_get_pll(void) {}
void rkclk_dump_pll(void) {}
void rkclk_set_pll_rate_by_id(enum rk_plls_id pll_id, uint32 mHz) {}
uint32 rkclk_get_pll_rate_by_id(enum rk_plls_id pll_id) { return 24 * MHZ; }
void rkclk_set_cpll_rate(uint32 pll_hz) {}
int rkclk_lcdc_aclk_set(uint32 lcdc_id, uint32 aclk_hz) { return 0; }
int rkclk_lcdc_dclk_set(uint32 lcdc_id, uint32 dclk_hz) { return 0; }
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz) { return 0; }
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src) {}
unsigned int rkclk_get_mmc_clk(uint32 sdid) { return 24 * MHZ; }
int rkclk_set_nandc_div(uint32 nandc_id, uint32 pllsrc, uint32 freq) { return 0; }
int rkclk_set_mmc_clk_div(uint32 sdid, uint32 div) { return 0; }
unsigned int rkclk_get_pwm_clk(uint32 pwm_id) { return 0; }
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id) { return 0; }
unsigned int rkclk_get_spi_clk(uint32 spi_bus) { return 0; }
#ifdef CONFIG_SECUREBOOT_CRYPTO
void rkclk_set_crypto_clk(uint32 rate) {}
#endif /* CONFIG_SECUREBOOT_CRYPTO*/

#endif /* CONFIG_RK_CLOCK */

