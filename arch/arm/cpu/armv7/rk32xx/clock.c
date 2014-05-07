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

#define RKCLOCK_VERSION		"1.1"



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


#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	#include "clock-rk3288.c"
#else
	#error "PLS config chiptype for clock-rkxx.c!"
#endif


/*
 * rkplat calculate child clock div from parent
 * clk_parent: parent clock rate (HZ)
 * clk_child: child clock request rate (HZ)
 * even: if div needs even
 * return value: div
 */
uint32 rkclk_calc_clkdiv(uint32 clk_parent, uint32 clk_child, uint32 even)
{
	uint32 div = 0;

	div = (clk_parent + (clk_child - 1)) / clk_child;

	if (even) {
		div += (div % 2);
	}

	return div;
}


/*
 * rkplat pll select and clock div calcate
 * clock: device request freq HZ
 * even: if div needs even
 * return value:
 * high 16bit: 0 - codec pll, 1 - general pll
 * low 16bit : div
 */
uint32 rkclk_calc_pll_and_div(uint32 clock, uint32 even)
{
	uint32 div = 0, gdiv = 0, cdiv = 0;
	uint32 pll_sel = 0; // 0: general pll, 1: codec pll

	gdiv = rkclk_calc_clkdiv(gd->bus_clk, clock, even); // general pll div
	cdiv = rkclk_calc_clkdiv(gd->pci_clk, clock, even); // codec pll div

	pll_sel = (gd->bus_clk / gdiv) >= (gd->pci_clk / cdiv);

	div = pll_sel ? gdiv : cdiv;

	return (pll_sel << 16) | div;
}


void lcdc_clk_enable(void)
{
    int clk = 300;
    uint32 div = (CONFIG_RKCLK_GPLL_FREQ-1)/clk;
    if(div>0x1f)div = 0x1f;

    #if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
    cru_writel((1<<30) | (0x1f<<24) | (1<<22) | (0x1f<<16) | (1<<14) | (div<<8) | (1<<6) | div, CRU_CLKSELS_CON(31));//  aclk0 = aclk1 = GPLL/(div+1)  
    #else
    cru_writel((1<<31) | (0x1f<<24) | (1<<23) | (0x1f<<16) | (1<<15) | (div<<8) | (1<<7) | div, CRU_CLKSELS_CON(31));
    #endif

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
    
    #if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
    cru_writel((3<<16) | (0xff<<24) | (div<<8) | ((div==div1)?1:0), CRU_CLKSELS_CON(27)); //lcdc0_dclk
    cru_writel((3<<22) | (0xff<<24) | (div<<8) | (((div==div1)?1:0)<<6), CRU_CLKSELS_CON(29)); //lcdc1_dclk
    #else
    cru_writel((1<<16) | (0xff<<24) | (div<<8) | ((div==div1)?1:0), CRU_CLKSELS_CON(27)); //lcdc0_dclk
    cru_writel((1<<16) | (0xff<<24) | (div<<8) | ((div==div1)?1:0), CRU_CLKSELS_CON(20)); //lcdc1_dclk
    #endif

}

