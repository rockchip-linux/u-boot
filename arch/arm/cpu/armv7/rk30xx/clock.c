/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#include <common.h>
#include <asm/arch/rk30_drivers.h>

DECLARE_GLOBAL_DATA_PTR;

typedef enum rk_plls_id {
	APLL_ID = 0,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,
	END_PLL_ID,
} rk_plls_id;

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
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
        g_cruReg->CRU_CLKSEL_CON[0] = ((0x1F | (0x3<<6) | (0x1<<8))<<16)
                                                      | (0x0<<8)     //core_clk_src = ARM PLL = 600MHz
                                                      | (0x1<<6)     //clk_core:clk_core_periph = 4:1 = 600MHz : 150MHz
                                                      | 0;           //clk_core = core_clk_src/1 = 600MHz

        g_cruReg->CRU_CLKSEL_CON[1] = (((0x3<<12) | (0x3<<8) | (0x3<<14) | 0x7)<<16)     //clk_core:aclk_cpu = 1:1 = 192MHz : 192 MHz
                                                      | (0x1<<14)    //hclk_cpu:pclken_ahb2apb = 2:1 = 150MHz : 75MHz
                                                      | (0x1<<12)    //aclk_cpu:pclk_cpu = 2:1 = 150MHz : 75MHz
                                                      | (0x0<<8)     //aclk_cpu:hclk_cpu = 1:1 = 150MHz : 150MHz
                                                      | 2;           //clk_core:aclk_cpu = 4:1 = 600MHz : 150MHz
#else
        g_cruReg->CRU_CLKSEL_CON[0] = (((0x1F<<9)|(1<<8)|(0x3<<6)|(1<<5)|(0x1F))<<16)
                                                      | (0x0<<9)     //core_clk : core_clk_src = APLL = 600MHz
                                                      | (0x0<<8)     //core_clk_src = APLL = 600MHz
                                                      | (0x1<<6)     //clk_cpu:clk_core_periph = 4:1 = 600MHz : 150MHz
                                                      | (0x0<<5)     //clk_cpu_src = APLL = 600MHz
                                                      | 1;           //clk_cpu = core_clk_src/2 = 300MHz
        g_cruReg->CRU_CLKSEL_CON[1] = (((0x3<<14) | (0x3<<12) | (0x3<<8) | (0x7<<3)| 0x7)<<16)     //clk_core:aclk_cpu = 1:1 = 192MHz : 192 MHz
                                                      | (0x1<<14)    //hclk_cpu:pclken_ahb2apb = 2:1 = 150MHz : 75MHz
                                                      | (0x2<<12)    //aclk_cpu:pclk_cpu = 2:1 = 300MHz : 150MHz
                                                      | (0x1<<8)     //aclk_cpu:hclk_cpu = 2:1 = 300MHz : 150MHz
                                                      | (1<<3)       //clk_core:aclk_core = 2:1 = 300MHz : 300MHz
                                                      | 1;           //clk_cpu:aclk_cpu = 1:1 = 300MHz : 300MHz
#endif
}


static void rk30_gpll_cb(void)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
    	g_cruReg->CRU_CLKSEL_CON[10] = (((0x1<<15)|(0x3<<12)|(0x3<<8)|0x1F)<<16) 
                                                    | (0x0<<15)     //aclk_periph = GPLL/1 = 144MHz
                                                    | (0x3<<12)     //aclk_periph:pclk_periph = 4:1 = 144MHz : 36MHz
                                                    | (0x1<<8)      //aclk_periph:hclk_periph = 1:1 = 144MHz : 144MHz
                                                    | 0x0;   
#elif(CONFIG_RKCHIPTYPE == CONFIG_RK3188)
    	g_cruReg->CRU_CLKSEL_CON[10] = (((0x1<<15)|(0x3<<12)|(0x3<<8)|0x1F)<<16) 
                                                    | (0x1<<15)     //periph_clk_src = GPLL = 300MHz
                                                    | (0x2<<12)     //aclk_periph:pclk_periph = 4:1 = 300MHz : 75MHz
                                                    | (0x1<<8)      //aclk_periph:hclk_periph = 1:1 = 300MHz : 150MHz
                                                    | 0x0;          //aclk_periph=periph_clk_src/1 = 300Mhz
#else
    	g_cruReg->CRU_CLKSEL_CON[10] = (((0x1<<15)|(0x3<<12)|(0x3<<8)|0x1F)<<16) 
                                                    | (0x0<<15)     //periph_clk_src = GPLL = 300MHz
                                                    | (0x2<<12)     //aclk_periph:pclk_periph = 4:1 = 300MHz : 75MHz
                                                    | (0x1<<8)      //aclk_periph:hclk_periph = 2:1 = 300MHz : 150MHz
                                                    | 0x0;          //aclk_periph=periph_clk_src/1 = 300Mhz
#endif
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

	//enter slowmode
	g_cruReg->CRU_MODE_CON = (0x3<<((pll_id*4) +  16)) | (0x0<<(pll_id*4));            //PLL slow-mode

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
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

	//enter rest
        g_cruReg->CRU_PLL_CON[pll_id][3] = PLL_RESET;
        g_cruReg->CRU_PLL_CON[pll_id][0] = NR(nr) | NO(no);
        g_cruReg->CRU_PLL_CON[pll_id][1] = NF(MHz);
        g_cruReg->CRU_PLL_CON[pll_id][2] = NB(MHz);
        clk_delayus(1);
        g_cruReg->CRU_PLL_CON[pll_id][3] = PLL_DE_RESET;
#else
        if(MHz >= 600)
        {
            nr = 1;
            no = 2;
            nf = MHz *2 / 24;
        }
        else if(MHz >= 400)
        {
            nr = 1;
            no = 3;
            nf = MHz * 3 / 24;
        }
        else if(MHz >= 250)
        {
            nr = 1;
            no = 5;
            nf = MHz * 5 / 24;
        }
        else if(MHz >= 140)
        {
            nr = 1;
            no = 8;
            nf = MHz * 8 / 24;
        }
        else    
        {
            nr = 1;
            no = 12;
            nf = MHz * 12 / 24;
        }
        g_cruReg->CRU_PLL_CON[pll_id][3] = (((0x1<<1)<<16) | (0x1<<1));
        g_cruReg->CRU_PLL_CON[pll_id][0] = NR(nr) | NO(no);
        g_cruReg->CRU_PLL_CON[pll_id][1] = NF(nf);
        clk_delayus(1);
        g_cruReg->CRU_PLL_CON[pll_id][3] = (((0x1<<1)<<16) | (0x0<<1));
#endif

	clk_delayus(1000);

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
    
    g_cruReg->CRU_CLKSEL_CON[27] = (1<<16) | (0xff<<24) | (div<<8) | 0x1;//
}

void rk_set_pll(void)
{
	rk30_pll_clk_set_rate(APLL_ID, RK30_APLL_FREQ, rk30_apll_cb);
	rk30_pll_clk_set_rate(GPLL_ID, RK30_GPLL_FREQ, rk30_gpll_cb);
}

