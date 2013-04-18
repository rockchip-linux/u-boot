/*
 * (C) Copyright 2013
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
#include <asm/arch/rk30_drivers.h>

DECLARE_GLOBAL_DATA_PTR;

/* pll callback function */
typedef void (*callback_f)(void);


#define RK30_APLL_FREQ		600
#define RK30_GPLL_FREQ		300


#define NR(n)      ((0x3F<<(8+16)) | ((n-1)<<8))
#define NO(n)      ((0x3F<<16) | (n-1))
#define NF(n)      ((0xFFFF<<16) | (n-1))
#define NB(n)      ((0xFFF<<16) | (n-1))

static void rk30_apll_cb(void)
{
        g_cruReg->CRU_CLKSEL_CON[0] = ((0x1F | (0x3<<6) | (0x1<<8))<<16)
                                                      | (0x0<<8)     //core_clk_src = ARM PLL = 600MHz
                                                      | (0x1<<6)     //clk_core:clk_core_periph = 4:1 = 600MHz : 150MHz
                                                      | 0;           //clk_core = core_clk_src/1 = 600MHz

        g_cruReg->CRU_CLKSEL_CON[1] = (((0x3<<12) | (0x3<<8) | (0x3<<14) | 0x7)<<16)     //clk_core:aclk_cpu = 1:1 = 192MHz : 192 MHz
                                                      | (0x1<<14)    //hclk_cpu:pclken_ahb2apb = 2:1 = 150MHz : 75MHz
                                                      | (0x1<<12)    //aclk_cpu:pclk_cpu = 2:1 = 150MHz : 75MHz
                                                      | (0x0<<8)     //aclk_cpu:hclk_cpu = 1:1 = 150MHz : 150MHz
                                                      | 2;           //clk_core:aclk_cpu = 4:1 = 600MHz : 150MHz
}


static void rk30_gpll_cb(void)
{
    	g_cruReg->CRU_CLKSEL_CON[10] = (((0x1<<15)|(0x3<<12)|(0x3<<8)|0x1F)<<16) 
                                                    | (0x0<<15)     //aclk_periph = GPLL/1 = 144MHz
                                                    | (0x3<<12)     //aclk_periph:pclk_periph = 4:1 = 144MHz : 36MHz
                                                    | (0x1<<8)      //aclk_periph:hclk_periph = 1:1 = 144MHz : 144MHz
                                                    | 0x0;
}


static void rk30_dpll_cb(void)
{
    g_cruReg->CRU_CLKSEL_CON[26] = ((0x3 | (0x1<<8))<<16)
                                                  | (0x0<<8)     //clk_ddr_src = DDR PLL
                                                  | 0;           //clk_ddr_src:clk_ddrphy = 1:1
}


static int rk30_pll_clk_set_rate(rk_plls_id pll_id, uint32 MHz, callback_f cb)
{
	uint32 nr, no, nf;

	MHz += (MHz & 0x1);
        if (MHz >= 600) {
            nr = 1;
            no = 2;
            nf = MHz *2 / 24;
        } else if (MHz >= 400) {
            nr = 1;
            no = 3;
            nf = MHz * 3 / 24;
        } else if (MHz >= 250) {
            nr = 1;
            no = 5;
            nf = MHz * 5 / 24;
        } else if (MHz >= 140) {
            nr = 1;
            no = 8;
            nf = MHz * 8 / 24;
        } else {
            nr = 1;
            no = 12;
            nf = MHz * 12 / 24;
        }

	//enter slowmode
	g_cruReg->CRU_MODE_CON = (0x3<<((pll_id*4) +  16)) | (0x0<<(pll_id*4));            //PLL slow-mode
	//enter rest
        g_cruReg->CRU_PLL_CON[pll_id][3] = (((0x1<<1)<<16) | (0x1<<1));
        g_cruReg->CRU_PLL_CON[pll_id][0] = NR(nr) | NO(no);
        g_cruReg->CRU_PLL_CON[pll_id][1] = NF(nf);
	__udelay(1);
	//return form rest
	g_cruReg->CRU_PLL_CON[pll_id][3] = (((0x1<<1)<<16) | (0x0<<1));

	__udelay(1000);
	if (cb != NULL)
		cb();

	return 0;
}


#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	rk30_pll_clk_set_rate(APLL_ID, RK30_APLL_FREQ, rk30_apll_cb);
	rk30_pll_clk_set_rate(GPLL_ID, RK30_GPLL_FREQ, rk30_gpll_cb);
}
#endif

