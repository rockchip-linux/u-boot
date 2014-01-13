/*
 * (C) Copyright 2008-2013 Rockchip Electronics
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
#include <asm/arch/rk30_drivers.h>
#include <asm/arch/clock.h>

DECLARE_GLOBAL_DATA_PTR;


#define CONFIG_RKCLK_APLL_FREQ		600 /* MHZ */
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
#define CONFIG_RKCLK_GPLL_FREQ		384 /* MHZ */
#else
#define CONFIG_RKCLK_GPLL_FREQ		768 /* MHZ */
#endif
#define CONFIG_RKCLK_CPLL_FREQ		594 /* MHZ */

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


#define cru_readl(offset)	readl(RKIO_CRU_PHYS + offset)
#define cru_writel(v, offset)	do { writel(v, RKIO_CRU_PHYS + offset); dsb(); } while (0)


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


#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
	#include "clock-rk3066.c"
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
	#include "clock-rk3168.c"
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)
	#include "clock-rk3188.c"
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
	#include "clock-rk3026.c"
#endif

/*
 * rkplat clock set for arm and general pll
 */
void rkclk_set_pll(void)
{
	rkclk_pll_clk_set_rate(APLL_ID, CONFIG_RKCLK_APLL_FREQ, rkclk_apll_cb);
	rkclk_pll_clk_set_rate(GPLL_ID, CONFIG_RKCLK_GPLL_FREQ, rkclk_gpll_cb);
	rkclk_pll_clk_set_rate(CPLL_ID, CONFIG_RKCLK_CPLL_FREQ, NULL);
}


void lcdc_clk_enable(void)
{
    int clk = 300;
    uint32 div = (CONFIG_RKCLK_GPLL_FREQ-1)/clk;
    if(div>0x1f)div = 0x1f;
    g_cruReg->CRU_CLKSEL_CON[31] = (1<<31) | (0x1f<<24) | (1<<23) | (0x1f<<16) | (1<<15) | (div<<8) | (1<<7) | div;//  aclk0 = aclk1 = GPLL/(div+1)
}

void set_lcdc_dclk(int clk)
{
    uint32 div = 0;
    uint32 div1 = (CONFIG_RKCLK_GPLL_FREQ-1)/clk;          //general clk for source
    uint32 div2 = (CONFIG_RKCLK_CPLL_FREQ-1)/clk;         //codec clk for source
    if((div1+1)%2)div1+=1;
    if((div2+1)%2)div2+=1;
    div = ((CONFIG_RKCLK_GPLL_FREQ/(div1+1)) > (CONFIG_RKCLK_CPLL_FREQ/(div2+1))) ? div1 : div2;

    printf("set_lcdc_dclk: lcdc_source_clk = %d, clk = %d, div = %d\n", (div==div1)?CONFIG_RKCLK_GPLL_FREQ:CONFIG_RKCLK_CPLL_FREQ, clk, div);
    g_cruReg->CRU_CLKSEL_CON[27] = (1<<16) | (0xff<<24) | (div<<8) | ((div==div1)?1:0);     //lcdc0_dclk
    g_cruReg->CRU_CLKSEL_CON[20] = (1<<16) | (0xff<<24) | (div<<8) | ((div==div1)?1:0);     //lcdc1_dclk
}


