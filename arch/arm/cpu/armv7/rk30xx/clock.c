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

#define PLL_RESET  	(((0x1<<5)<<16) | (0x1<<5))
#define PLL_DE_RESET  	(((0x1<<5)<<16) | (0x0<<5))

#define NR(n)      ((0x3F<<(8+16)) | ((n-1)<<8))
#define NO(n)      ((0x3F<<16) | (n-1))
#define NF(n)      ((0xFFFF<<16) | (n-1))
#define NB(n)      ((0xFFF<<16) | (n-1))


/****************************************************************************
us delay function
Cpu highest frequency is 1 GHz
1 cycle = 1/1 ns
1 us = 1000 ns = 1000 * 1 cycles = 1000 cycles
*****************************************************************************/
static volatile uint32_t loops_per_us;

#define LPJ_100MHZ  1UL

void clk_delayus(uint32_t us)
{
	volatile int i;

	/* copro seems to need some delay between reading and writing */
	for (i = 0; i < 100; i++)
		nop();
	asm volatile("" : : : "memory");
}


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


void rk30_pll_clk_set_rate(rk_plls_id pll_id, uint32 MHz, callback_f cb)
{
	uint32 nr, no, nf;
	uint32 delay;

	MHz += (MHz & 0x1);
	if(MHz > 500) {
		nr = 24;
		no = 1;
	} else if(MHz > 250) {
		nr = 12;
		no = 2;
	} else if(MHz > 150) {
		nr = 6;
		no = 4;
	} else if(MHz > 100) {
		nr = 4;
		no = 6;
	} else {
		nr = 3;
		no = 8;
	}
	//enter slowmode
	g_cruReg->CRU_MODE_CON = (0x3<<((pll_id*4) +  16)) | (0x0<<(pll_id*4));            //PLL slow-mode
	//enter rest
        g_cruReg->CRU_PLL_CON[pll_id][3] = PLL_RESET;
        g_cruReg->CRU_PLL_CON[pll_id][0] = NR(nr) | NO(no);
        g_cruReg->CRU_PLL_CON[pll_id][1] = NF(MHz);
        g_cruReg->CRU_PLL_CON[pll_id][2] = NB(MHz);
        clk_delayus(1);
        g_cruReg->CRU_PLL_CON[pll_id][3] = PLL_DE_RESET;

	delay = 1000;
        while (delay > 0) {
    	    clk_delayus(1);
            if (g_grfReg->GRF_SOC_STATUS0 & (0x1<<4))
            	break;
            delay--;
    	 }


	if (cb != NULL)
		cb();

	g_cruReg->CRU_MODE_CON = (0x3<<((pll_id*4) +  16))  | (0x1<<(pll_id*4));            //PLL normal
}

void lcdc_clk_enable(void)
{
    //rk30_pll_clk_set_rate(GPLL_ID, RK30_GPLL_FREQ, rk30_gpll_cb);

    g_cruReg->CRU_CLKSEL_CON[31] = (1<<23) | (0x1f<<16) | (1<<7);//  aclk = GPLL

}

void set_lcdc_dclk(int clk)
{
    int *addr = 0;
    int div = RK30_GPLL_FREQ/clk -1;
    
    g_cruReg->CRU_CLKSEL_CON[27] = (1<<16) | (1<<20) | (0xff<<24) | (div<<8) | 0x1;//
}

void rk_set_pll(void)
{
	rk30_pll_clk_set_rate(APLL_ID, RK30_APLL_FREQ, rk30_apll_cb);
	rk30_pll_clk_set_rate(GPLL_ID, RK30_GPLL_FREQ, rk30_gpll_cb);
}

