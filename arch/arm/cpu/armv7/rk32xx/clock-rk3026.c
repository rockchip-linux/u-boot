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
#include <div64.h>
enum rk_plls_id {
	APLL_ID = 0,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,
	END_PLL_ID,
};
#if 1
#define CRU_GET_REG_BITS_VAL(reg,bits_shift, msk)	(((reg) >> (bits_shift))&(msk))
#define CRU_W_MSK(bits_shift, msk)	((msk) << ((bits_shift) + 16))
#define CRU_SET_BITS(val,bits_shift, msk)	(((val)&(msk)) << (bits_shift))

#define CRU_W_MSK_SETBITS(val,bits_shift,msk) (CRU_W_MSK(bits_shift, msk)|CRU_SET_BITS(val,bits_shift, msk))

/*******************PLL CON0 BITS***************************/

#define PLL_CLKFACTOR_SET(val, shift, msk) \
	((((val) - 1) & (msk)) << (shift))

#define PLL_CLKFACTOR_GET(reg, shift, msk) \
	((((reg) >> (shift)) & (msk)) + 1)

#define PLL_OD_MSK		(0x3f)
#define PLL_OD_SHIFT		(0x0)

#define PLL_CLKOD(val)		PLL_CLKFACTOR_SET(val, PLL_OD_SHIFT, PLL_OD_MSK)
#define PLL_NO(reg)		PLL_CLKFACTOR_GET(reg, PLL_OD_SHIFT, PLL_OD_MSK)

#define PLL_NO_SHIFT(reg)	PLL_CLKFACTOR_GET(reg, PLL_OD_SHIFT, PLL_OD_MSK)

#define PLL_CLKOD_SET(val)	(PLL_CLKOD(val) | CRU_W_MSK(PLL_OD_SHIFT, PLL_OD_MSK))

#define PLL_NR_MSK		(0x3f)
#define PLL_NR_SHIFT		(8)
#define PLL_CLKR(val)		PLL_CLKFACTOR_SET(val, PLL_NR_SHIFT, PLL_NR_MSK)
#define PLL_NR(reg)		PLL_CLKFACTOR_GET(reg, PLL_NR_SHIFT, PLL_NR_MSK)

#define PLL_CLKR_SET(val)	(PLL_CLKR(val) | CRU_W_MSK(PLL_NR_SHIFT, PLL_NR_MSK))

/*******************PLL CON1 BITS***************************/

#define PLL_NF_MSK		(0xffff)
#define PLL_NF_SHIFT		(0)
#define PLL_CLKF(val)		PLL_CLKFACTOR_SET(val, PLL_NF_SHIFT, PLL_NF_MSK)
#define PLL_NF(reg)		PLL_CLKFACTOR_GET(reg, PLL_NF_SHIFT, PLL_NF_MSK)

#define PLL_CLKF_SET(val)	(PLL_CLKF(val) | CRU_W_MSK(PLL_NF_SHIFT, PLL_NF_MSK))

/*******************PLL CON2 BITS***************************/
// "BWADJ" Just compatible with RK3188 plus
#define PLL_BWADJ_MSK		(0xfff & 0x000)
#define PLL_BWADJ_SHIFT		(0)
#define PLL_CLK_BWADJ_SET(val)	((val) | CRU_W_MSK(PLL_BWADJ_SHIFT, PLL_BWADJ_MSK))

/*******************PLL CON3 BITS***************************/
// "RESET" Just compatible with RK3188 plus
#define PLL_RESET_MSK		((1 & 0x0) << 5)
#define PLL_RESET_W_MSK		(PLL_RESET_MSK << 16)
#define PLL_RESET		(1 << 5)
#define PLL_RESET_RESUME	(0 << 5)



#define PLL_STANDBY_MSK		(1 << 2)
#define PLL_STANDBY		(1 << 2)
#define PLL_NO_STANDBY		(0 << 2)
/*******************CLKSEL0 BITS***************************/
//core preiph div
#define CORE_PERIPH_W_MSK	(3 << 22)
#define CORE_PERIPH_MSK		(3 << 6)
#define CORE_PERIPH_2		(0 << 6)
#define CORE_PERIPH_4		(1 << 6)
#define CORE_PERIPH_8		(2 << 6)
#define CORE_PERIPH_16		(3 << 6)
//arm clk pll sel
#define CORE_SEL_PLL_MSK	(1 << 7)
#define CORE_SEL_PLL_W_MSK	(1 << 23)
#define CORE_SEL_APLL		(0 << 7)
#define CORE_SEL_GPLL		(1 << 7)

#define CORE_CLK_DIV_W_MSK	(0x1F << 24)
#define CORE_CLK_DIV_MSK	(0x1F << 8)
#define CORE_CLK_DIV(i)		((((i) - 1) & 0x1F) << 8)

#define CPU_SEL_PLL_MSK		(1 << 5)
#define CPU_SEL_PLL_W_MSK	(1 << 21)
#define CPU_SEL_APLL		(0 << 5)
#define CPU_SEL_GPLL		(1 << 5)

#define CPU_CLK_DIV_W_MSK	(0x1F << 16)
#define CPU_CLK_DIV_MSK		(0x1F)
#define CPU_CLK_DIV(i)		(((i) - 1) & 0x1F)

/*******************CLKSEL1 BITS***************************/
//aclk div
#define GET_CORE_ACLK_VAL(reg) ((reg)>=4 ?8:((reg)+1))

#define CPU_ACLK_W_MSK		(7 << 16)
#define CPU_ACLK_MSK		(7 << 0)
#define CPU_ACLK_11		(0 << 0)
#define CPU_ACLK_21		(1 << 0)
#define CPU_ACLK_31		(2 << 0)
#define CPU_ACLK_41		(3 << 0)
#define CPU_ACLK_81		(4 << 0)

#define CORE_ACLK_W_MSK		(7 << 19)
#define CORE_ACLK_MSK		(7 << 3)
#define CORE_ACLK_11		(0 << 3)
#define CORE_ACLK_21		(1 << 3)
#define CORE_ACLK_31		(2 << 3)
#define CORE_ACLK_41		(3 << 3)
#define CORE_ACLK_81		(4 << 3)
//hclk div
#define ACLK_HCLK_W_MSK		(3 << 24)
#define ACLK_HCLK_MSK		(3 << 8)
#define ACLK_HCLK_11		(0 << 8)
#define ACLK_HCLK_21		(1 << 8)
#define ACLK_HCLK_41		(2 << 8)
// pclk div
#define ACLK_PCLK_W_MSK		(3 << 28)
#define ACLK_PCLK_MSK		(3 << 12)
#define ACLK_PCLK_11		(0 << 12)
#define ACLK_PCLK_21		(1 << 12)
#define ACLK_PCLK_41		(2 << 12)
#define ACLK_PCLK_81		(3 << 12)
// ahb2apb div
#define AHB2APB_W_MSK		(3 << 30)
#define AHB2APB_MSK		(3 << 14)
#define AHB2APB_11		(0 << 14)
#define AHB2APB_21		(1 << 14)
#define AHB2APB_41		(2 << 14)

/*******************MODE BITS***************************/

#define PLL_MODE_MSK(id)	(0x3 << ((id) * 4))
#define PLL_MODE_SLOW(id)	((0x0<<((id)*4))|(0x3<<(16+(id)*4)))
#define PLL_MODE_NORM(id)	((0x1<<((id)*4))|(0x3<<(16+(id)*4)))
#define PLL_MODE_DEEP(id)	((0x2<<((id)*4))|(0x3<<(16+(id)*4)))

/*******************clksel10***************************/

#define PERI_ACLK_DIV_MASK 0x1f
#define PERI_ACLK_DIV_W_MSK	(PERI_ACLK_DIV_MASK << 16)
#define PERI_ACLK_DIV(i)	(((i) - 1) & PERI_ACLK_DIV_MASK)
#define PERI_ACLK_DIV_OFF 0

#define PERI_HCLK_DIV_MASK 0x3
#define PERI_HCLK_DIV_OFF 8

#define PERI_PCLK_DIV_MASK 0x3
#define PERI_PCLK_DIV_OFF 12

/*******************gate BITS***************************/

#define CLK_GATE_CLKID(i)	(16 * (i))
#define CLK_GATE_CLKID_CONS(i)	CRU_CLKGATES_CON((i) / 16)

#define CLK_GATE(i)		(1 << ((i)%16))
#define CLK_UN_GATE(i)		(0)

#define CLK_GATE_W_MSK(i)	(1 << (((i) % 16) + 16))

DECLARE_GLOBAL_DATA_PTR;


/* Cpu clock source select */
#define CPU_SRC_ARM_PLL			0
#define CPU_SRC_GENERAL_PLL		1

/* Periph clock source select */
#define PERIPH_SRC_GENERAL_PLL		0
#define PERIPH_SRC_CODEC_PLL		1

/* DDR clock source select */
#define DDR_SRC_DDR_PLL			0
#define DDR_SRC_GENERAL_PLL		1
#endif

struct pll_clk_set {
	unsigned long	rate;
	u32	pllcon0;
	u32	pllcon1;
	u32	pllcon2;
	u32	rst_dly; //us
	u8	core_div;
	u8	core_periph_div;
	u8	core_aclk_div;
	u8	aclk_div;
	u8	hclk_div;
	u8	pclk_div;
	u8	ahb2apb_div;
	u32	clksel0;
	u32	clksel1;	
	unsigned long lpj;	//loop per jeffise
};



#if 0
#define _GPLL_SET_CLKS(khz, nr, nf, no, _axi_div, _ahb_div, _apb_div) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no), \
	.pllcon1	= PLL_CLKF_SET(nf), \
	.aclk_div	= CLK_DIV_##_axi_div, \
	.hclk_div	= CLK_DIV_##_ahb_div, \
	.pclk_div	= CLK_DIV_##_apb_div, \
	.rst_dly	= ((nr*500)/24+1), \
}

#define _DPLL_SET_CLKS(khz, nr, nf, no, _ddr_div) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no), \
	.pllcon1	= PLL_CLKF_SET(nf), \
	.core_div	= CLK_DIV_##_ddr_div, \
	.rst_dly	= ((nr*500)/24+1), \
}


#define _CPLL_SET_CLKS(khz, nr, nf, no) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no), \
	.pllcon1	= PLL_CLKF_SET(nf), \
	.rst_dly	= ((nr*500)/24+1), \
}
#endif
struct pll_data {
	u32 id;
	u32 size;
	struct pll_clk_set *clkset;
};

#define SET_PLL_DATA(_pll_id, _table, _size) \
{\
	.id = (_pll_id), \
	.size = (_size), \
	.clkset = (_table), \
}
//#define CRU_MODE_CON		0x40
//#define CRU_CLKSEL_CON		0x44
//#define CRU_CLKGATE_CON		0xd0
#define CRU_GLB_SRST_FST	0x100
#define CRU_GLB_SRST_SND	0x104
#define CRU_SOFTRST_CON		0x110

#define PLL_CONS(id, i)		((id) * 0x10 + ((i) * 4))

#define CRU_CLKSELS_CON_CNT	(35)
#define CRU_CLKSELS_CON(i)	(CRU_CLKSEL_CON + ((i) * 4))

#define CRU_CLKGATES_CON_CNT	(10)
//#define CRU_CLKGATES_CON(i)	(CRU_CLKGATE_CON + ((i) * 4))

#define CRU_SOFTRSTS_CON_CNT	(9)
#define CRU_SOFTRSTS_CON(i)	(CRU_SOFTRST_CON + ((i) * 4))

#define CRU_MISC_CON		(0x134)
#define CRU_GLB_CNT_TH		(0x140)

/*PLL_CON 0,1,2*/
#define PLL_PWR_ON			(0)
#define PLL_PWR_DN			(1)
#define PLL_BYPASS			(1 << 15)
#define PLL_NO_BYPASS			(0 << 15)
//con0
#define PLL_BYPASS_SHIFT		(15)

#define PLL_POSTDIV1_MASK		(0x7)
#define PLL_POSTDIV1_SHIFT		(12)
#define PLL_FBDIV_MASK			(0xfff)
#define PLL_FBDIV_SHIFT			(0)

//con1
#define PLL_RSTMODE_SHIFT		(15)
#define PLL_RST_SHIFT			(14)
#define PLL_PWR_DN_SHIFT		(13)
#define PLL_DSMPD_SHIFT			(12)
#define PLL_LOCK_SHIFT			(10)

#define PLL_POSTDIV2_MASK		(0x7)
#define PLL_POSTDIV2_SHIFT		(6)
#define PLL_REFDIV_MASK			(0x3f)
#define PLL_REFDIV_SHIFT		(0)

//con2
#define PLL_FOUT4PHASE_PWR_DN_SHIFT	(27)
#define PLL_FOUTVCO_PWR_DN_SHIFT	(26)
#define PLL_FOUTPOSTDIV_PWR_DN_SHIFT	(25)
#define PLL_DAC_PWR_DN_SHIFT		(24)

#define PLL_FRAC_MASK			(0xffffff)
#define PLL_FRAC_SHIFT			(0)

/********************************************************************/
#define CRU_GET_REG_BIT_VAL(reg, bits_shift)		(((reg) >> (bits_shift)) & (0x1))
#define CRU_GET_REG_BITS_VAL(reg, bits_shift, msk)	(((reg) >> (bits_shift)) & (msk))
#define CRU_SET_BIT(val, bits_shift) 			(((val) & (0x1)) << (bits_shift))


#define CRU_W_MSK_SETBITS(val, bits_shift, msk) 	(CRU_W_MSK(bits_shift, msk)	\
							| CRU_SET_BITS(val, bits_shift, msk))
#define CRU_W_MSK_SETBIT(val, bits_shift) 		(CRU_W_MSK(bits_shift, 0x1)	\
							| CRU_SET_BIT(val, bits_shift))

#define PLL_SET_REFDIV(val)				CRU_W_MSK_SETBITS(val, PLL_REFDIV_SHIFT, PLL_REFDIV_MASK)
#define PLL_SET_FBDIV(val)				CRU_W_MSK_SETBITS(val, PLL_FBDIV_SHIFT, PLL_FBDIV_MASK)
#define PLL_SET_POSTDIV1(val)				CRU_W_MSK_SETBITS(val, PLL_POSTDIV1_SHIFT, PLL_POSTDIV1_MASK)
#define PLL_SET_POSTDIV2(val)				CRU_W_MSK_SETBITS(val, PLL_POSTDIV2_SHIFT, PLL_POSTDIV2_MASK)
#define PLL_SET_FRAC(val)				CRU_SET_BITS(val, PLL_FRAC_SHIFT, PLL_FRAC_MASK)

#define PLL_GET_REFDIV(reg)				CRU_GET_REG_BITS_VAL(reg, PLL_REFDIV_SHIFT, PLL_REFDIV_MASK)
#define PLL_GET_FBDIV(reg)				CRU_GET_REG_BITS_VAL(reg, PLL_FBDIV_SHIFT, PLL_FBDIV_MASK)
#define PLL_GET_POSTDIV1(reg)				CRU_GET_REG_BITS_VAL(reg, PLL_POSTDIV1_SHIFT, PLL_POSTDIV1_MASK)
#define PLL_GET_POSTDIV2(reg)				CRU_GET_REG_BITS_VAL(reg, PLL_POSTDIV2_SHIFT, PLL_POSTDIV2_MASK)
#define PLL_GET_FRAC(reg)				CRU_GET_REG_BITS_VAL(reg, PLL_FRAC_SHIFT, PLL_FRAC_MASK)

//#define APLL_SET_BYPASS(val)				CRU_SET_BIT(val, PLL_BYPASS_SHIFT)
#define PLL_SET_DSMPD(val)				CRU_W_MSK_SETBIT(val, PLL_DSMPD_SHIFT)
#define PLL_GET_DSMPD(reg)				CRU_GET_REG_BIT_VAL(reg, PLL_DSMPD_SHIFT)
/*******************MODE BITS***************************/
#define PLL_MODE_MSK(id)		(0x1 << ((id) * 4))
#define PLL_MODE_SHIFT(id)		((id) * 4)
#define PLL_MODE_SLOW(id)		(CRU_W_MSK_SETBIT(0x0, PLL_MODE_SHIFT(id)))
#define PLL_MODE_NORM(id)		(CRU_W_MSK_SETBIT(0x1, PLL_MODE_SHIFT(id)))
/*******************CLKSEL0 BITS***************************/
#define CLK_SET_DIV_CON_SUB1(val, bits_shift, msk)	CRU_W_MSK_SETBITS((val - 1), bits_shift, msk)

#define CPU_CLK_PLL_SEL_SHIFT		(13)
#define CORE_CLK_PLL_SEL_SHIFT		(7)
#define SEL_APLL			(0)
#define SEL_GPLL			(1)
#define CPU_SEL_PLL(plls)		CRU_W_MSK_SETBIT(plls, CPU_CLK_PLL_SEL_SHIFT)
#define CORE_SEL_PLL(plls)		CRU_W_MSK_SETBIT(plls, CORE_CLK_PLL_SEL_SHIFT)

#define ACLK_CPU_DIV_MASK		(0x1f)
#define ACLK_CPU_DIV_SHIFT		(8)
#define A9_CORE_DIV_MASK		(0x1f)
#define A9_CORE_DIV_SHIFT		(0)

#define RATIO_11		(1)
#define RATIO_21		(2)
#define RATIO_41		(4)
#define RATIO_81		(8)

#define ACLK_CPU_DIV(val)		CLK_SET_DIV_CON_SUB1(val, ACLK_CPU_DIV_SHIFT, ACLK_CPU_DIV_MASK)
#define CLK_CORE_DIV(val)		CLK_SET_DIV_CON_SUB1(val, A9_CORE_DIV_SHIFT, A9_CORE_DIV_MASK)	
/*******************CLKSEL1 BITS***************************/
#define PCLK_CPU_DIV_MASK		(0x7)
#define PCLK_CPU_DIV_SHIFT		(12)
#define HCLK_CPU_DIV_MASK		(0x3)
#define HCLK_CPU_DIV_SHIFT		(8)
#define ACLK_CORE_DIV_MASK		(0x7)
#define ACLK_CORE_DIV_SHIFT		(4)
#define CORE_PERIPH_DIV_MASK		(0xf)
#define CORE_PERIPH_DIV_SHIFT		(0)

#define PCLK_CPU_DIV(val)		CLK_SET_DIV_CON_SUB1(val, PCLK_CPU_DIV_SHIFT, PCLK_CPU_DIV_MASK)
#define HCLK_CPU_DIV(val)		CLK_SET_DIV_CON_SUB1(val, HCLK_CPU_DIV_SHIFT, HCLK_CPU_DIV_MASK)
#define ACLK_CORE_DIV(val)		CLK_SET_DIV_CON_SUB1(val, ACLK_CORE_DIV_SHIFT, ACLK_CORE_DIV_MASK)
#define CLK_CORE_PERI_DIV(val)		CLK_SET_DIV_CON_SUB1(val, CORE_PERIPH_DIV_SHIFT, CORE_PERIPH_DIV_MASK)	

/*******************clksel10***************************/
#define PERI_PLL_SEL_SHIFT	15
#define PERI_PCLK_DIV_MASK	(0x3)
#define PERI_PCLK_DIV_SHIFT	(12)
#define PERI_HCLK_DIV_MASK	(0x3)
#define PERI_HCLK_DIV_SHIFT	(8)
#define PERI_ACLK_DIV_MASK	(0x1f)
#define PERI_ACLK_DIV_SHIFT	(0)

#define SEL_2PLL_GPLL		(0)
#define SEL_2PLL_CPLL		(1)

#define PERI_CLK_SEL_PLL(plls)	CRU_W_MSK_SETBIT(plls, PERI_PLL_SEL_SHIFT)
#define PERI_SET_ACLK_DIV(val)		CLK_SET_DIV_CON_SUB1(val, PERI_ACLK_DIV_SHIFT, PERI_ACLK_DIV_MASK)

#define CLK_LOOPS_JIFFY_REF 11996091ULL
#define CLK_LOOPS_RATE_REF (1200) //Mhz

#define _APLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, \
		_periph_div, _aclk_core_div, _axi_div, _apb_div, _ahb_div) \
{ \
	.rate	= (_mhz) * MHZ,	\
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv),	\
	.pllcon2 = PLL_SET_FRAC(_frac),	\
	.clksel1 = ACLK_CORE_DIV(RATIO_##_aclk_core_div) | CLK_CORE_PERI_DIV(RATIO_##_periph_div),	\
	.lpj	= (CLK_LOOPS_JIFFY_REF * _mhz) / CLK_LOOPS_RATE_REF,	\
	.rst_dly = 0,\
}



#define _GPLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, _aclk_div, _hclk_div, _pclk_div) \  
{ \
	.rate	= (_mhz) * KHZ, \
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv),	\
	.pllcon2 = PLL_SET_FRAC(_frac),	\
	.aclk_div	= CLK_DIV_##_aclk_div, \
	.hclk_div	= CLK_DIV_##_hclk_div, \
	.pclk_div	= CLK_DIV_##_pclk_div, \
}
#define _PLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac) \
{ \
	.rate	= (_mhz) * KHZ, \
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv),	\
	.pllcon2 = PLL_SET_FRAC(_frac),	\
}
static const struct pll_clk_set apll_clks[] = {
	//_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, 
	//	_periph_div, _aclk_core_div, _axi_div, _apb_div, _ahb_div
	_APLL_SET_CLKS( 816, 1, 34, 1, 1, 1, 0, 41, 21, 41, 21, 21),
	_APLL_SET_CLKS( 600, 1, 25, 1, 1, 1, 0, 41, 21, 41, 21, 21),
};

//_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,  aclk_div, hclk_div, pclk_div

static const struct pll_clk_set gpll_clks[] = {
	_GPLL_SET_CLKS(768000, 1, 32, 1, 1, 1, 0, 4,2,4),
	_GPLL_SET_CLKS(297000, 2, 99, 4, 1, 1, 0, 2,1,2),
};

#if 0
/* gpll clock table, should be from high to low */
static const struct pll_clk_set gpll_clks[] = {
	//rate, nr, nf, no,	axi_div, hclk_div, pclk_div
	_GPLL_SET_CLKS(768000, 1,  64, 2,    4, 2, 4),
	_GPLL_SET_CLKS(594000, 2, 198, 4,    4, 1, 2),
	_GPLL_SET_CLKS(384000, 2, 128, 4,    2, 2, 4),
	_GPLL_SET_CLKS(300000, 1,  50, 4,    2, 1, 2),
	_GPLL_SET_CLKS(297000, 2, 198, 8,    2, 1, 2),
};
#endif
static const struct pll_clk_set cpll_clks[] = {
	_PLL_SET_CLKS(798000, 4, 133, 1, 1, 1, 0),
	_PLL_SET_CLKS(594000, 2, 99, 2, 1, 1, 0),
};
#if 0
/* cpll clock table, should be from high to low */
static const struct pll_clk_set cpll_clks[] = {
	//rate, nr, nf, no
	_CPLL_SET_CLKS(798000, 2, 133, 2),
	_CPLL_SET_CLKS(594000, 2, 198, 4),
};
#endif

struct pll_data rkpll_data[END_PLL_ID] = {
	SET_PLL_DATA(APLL_ID, apll_clks, ARRAY_SIZE(apll_clks)),
	SET_PLL_DATA(DPLL_ID, NULL, 0),
	SET_PLL_DATA(CPLL_ID, cpll_clks, ARRAY_SIZE(cpll_clks)),
	SET_PLL_DATA(GPLL_ID, gpll_clks, ARRAY_SIZE(gpll_clks)),
};


static void rkclk_pll_wait_lock(enum rk_plls_id pll_id)
{
	uint32 delay = 50000000;//24000000;
	while (delay > 0) {
		if ((g_cruReg->CRU_PLL_CON[pll_id][1]  & (0x1 << PLL_LOCK_SHIFT))) {
			break;
		}
		delay--;
	}
	if (delay == 0) {
		while(1);
	}
}

#define FRAC_MODE	0
static unsigned long pll_clk_recalc(u8 pll_id, unsigned long parent_rate)
{
	unsigned long rate;
	unsigned int dsmp = 0;
	u64 rate64 = 0, frac_rate64 = 0;
	uint32 con;
	//dsmp = PLL_GET_DSMPD(cru_readl(PLL_CONS(pll_id, 1)));
	dsmp = PLL_GET_DSMPD(g_cruReg->CRU_PLL_CON[pll_id][1]);
	con = g_cruReg->CRU_MODE_CON;
	con = con & PLL_MODE_MSK(pll_id);
	con = con >> (pll_id*4);
	if (con) {
		u32 pll_con0 = g_cruReg->CRU_PLL_CON[pll_id][0];//cru_readl(PLL_CONS(pll_id, 0));
		u32 pll_con1 = g_cruReg->CRU_PLL_CON[pll_id][1];//cru_readl(PLL_CONS(pll_id, 1));
		u32 pll_con2 = g_cruReg->CRU_PLL_CON[pll_id][2];//cru_readl(PLL_CONS(pll_id, 2));
		//integer mode
		rate64 = (u64)parent_rate * PLL_GET_FBDIV(pll_con0);
		do_div(rate64, PLL_GET_REFDIV(pll_con1));

		if (FRAC_MODE == dsmp) {
			//fractional mode
			frac_rate64 = (u64)parent_rate * PLL_GET_FRAC(pll_con2);
			do_div(frac_rate64, PLL_GET_REFDIV(pll_con1));
			rate64 += frac_rate64 >> 24;
		}
		do_div(rate64, PLL_GET_POSTDIV1(pll_con0));
		do_div(rate64, PLL_GET_POSTDIV2(pll_con1));

		rate = rate64;
	} else {
		rate = parent_rate;
	}
	return rate;
}

static int rkclk_pll_clk_set_rate(enum rk_plls_id pll_id, uint32 mHz, pll_callback_f cb_f)
{
	struct pll_data *pll = NULL;
	struct pll_clk_set *clkset = NULL;
	unsigned long rate = mHz * MHZ;
	int i = 0;
	for(i=0; i<END_PLL_ID; i++) {
		if(rkpll_data[i].id == pll_id) {
			pll = &rkpll_data[i];
			break;
		}
	}
	if((pll == NULL) || (pll->clkset == NULL)) {
		return -1;
	}

	for(i=0; i<pll->size; i++) {
		if(pll->clkset[i].rate <= rate) {
			clkset = &(pll->clkset[i]);
			break;
		}
	}
	if(clkset == NULL) {
		return -1;
	}
	/* PLL enter slow-mode */
	 g_cruReg->CRU_MODE_CON = (0x3<<((pll_id*4) + 16)) | (0x0<<(pll_id*4));
	/* enter rest */
        g_cruReg->CRU_PLL_CON[pll_id][0] = clkset->pllcon0;
        g_cruReg->CRU_PLL_CON[pll_id][1] = clkset->pllcon1;
        g_cruReg->CRU_PLL_CON[pll_id][2] = clkset->pllcon2;
	//clk_loop_delayus(clkset->rst_dly);
	clk_loop_delayus(500);
	/* delay for pll setup */
	rkclk_pll_wait_lock(pll_id);
	if (cb_f != NULL) {
		cb_f(clkset);
	}

	/* PLL enter normal-mode */
	g_cruReg->CRU_MODE_CON = (0x3<<((pll_id*4) + 16)) | (0x1<<(pll_id*4));

	return 0;
}


static uint32 rkclk_pll_clk_get_rate(enum rk_plls_id pll_id)
{
	uint32 nr, no, nf;
	uint32 con;

	con = g_cruReg->CRU_MODE_CON;
	con = con & PLL_MODE_MSK(pll_id);
	con = con >> (pll_id*4);
	if (con == 0) {
		/* slow mode */
		return (24 * MHZ);
	} else if (con == 1) {
		/* normal mode */
		con = g_cruReg->CRU_PLL_CON[pll_id][0];
		no = PLL_NO(con);
		nr = PLL_NR(con);
		con = g_cruReg->CRU_PLL_CON[pll_id][1];
		nf = PLL_NF(con);

		return (24 * nf / (nr * no)) * MHZ;
	} else {
		/* deep slow mode */
		return 32768;
	}
}


/*
 * rkplat clock set periph clock from general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_periph_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* periph clock source select: 0: general pll, 1: codec pll */
	pll_src &= 0x01;
	if(pll_src == 0) {
		pll_sel = 0;
	} else {
		pll_sel = 1;
	}

	/* periph aclk - aclk_periph = periph_clk_src / n */
	aclk_div &= PERI_ACLK_DIV_MASK;
	if(aclk_div == 0) {
		a_div = 1;
	} else {
		a_div = aclk_div - 1;
	}

	/* periph hclk - aclk_periph:hclk_periph */
	hclk_div &= PERI_HCLK_DIV_MASK;
	switch (hclk_div)
	{
		case CLK_DIV_1:
			h_div = 0;
			break;
		case CLK_DIV_2:
			h_div = 1;
			break;
		case CLK_DIV_4:
			h_div = 2;
			break;
		default:
			h_div = 1;
			break;
	}

	/* periph pclk - aclk_periph:pclk_periph */
	pclk_div &= PERI_PCLK_DIV_MASK;
	switch (pclk_div)
	{
		case CLK_DIV_1:
			p_div = 0;
			break;
		case CLK_DIV_2:
			p_div = 1;
			break;
		case CLK_DIV_4:
			p_div = 2;
			break;
		case CLK_DIV_8:
			p_div = 3;
			break;
		default:
			p_div = 2;
			break;
	}

	g_cruReg->CRU_CLKSEL_CON[10] = ((1 << (15 + 16)) | (pll_sel << 15))
				| ((PERI_PCLK_DIV_MASK << (PERI_PCLK_DIV_OFF + 16)) | (p_div << PERI_PCLK_DIV_OFF))
				| ((PERI_HCLK_DIV_MASK << (PERI_HCLK_DIV_OFF + 16)) | (h_div << PERI_HCLK_DIV_OFF))
				| ((PERI_ACLK_DIV_MASK << (PERI_ACLK_DIV_OFF + 16)) | (a_div << PERI_ACLK_DIV_OFF));
}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_coreclk_set(uint32 pll_src, uint32 core_div, uint32 core_periph_div, uint32 core_axi_div)
{
	uint32_t pll_sel = 0, c_div = 0, p_div = 0, ac_div;

	/* cpu clock source select: 0: arm pll, 1: general pll */
	pll_src &= 0x01;
	if(pll_src == 0) {
		pll_sel = CORE_SEL_APLL;
	} else {
		pll_sel = CORE_SEL_GPLL;
	}

	/* cpu core - clk_core = core_clk_src / n */
	core_div &= 0x1f;
	c_div = core_div - 1;

	/* cpu core periph - clk_core:clk_core_periph */
	core_periph_div &= 0x03;
	switch (core_periph_div)
	{
		case CLK_DIV_2:
			p_div = CORE_PERIPH_2;
			break;
		case CLK_DIV_3:
			p_div = CORE_PERIPH_4;
			break;
		case CLK_DIV_8:
			p_div = CORE_PERIPH_8;
			break;
		case CLK_DIV_16:
			p_div = CORE_PERIPH_16;
			break;
		default:
			p_div = CORE_PERIPH_4;
			break;
	}

	g_cruReg->CRU_CLKSEL_CON[0] = (CORE_SEL_PLL_W_MSK | (CORE_SEL_PLL_MSK & pll_sel))
				| (CORE_CLK_DIV_W_MSK | (CORE_CLK_DIV_MSK & c_div));


	/* axi core clk - clk_core:aclk_core */
	core_axi_div &= CORE_ACLK_MSK;
	switch (core_axi_div)
	{
		case CLK_DIV_1:
			ac_div = CORE_ACLK_11;
			break;
		case CLK_DIV_2:
			ac_div = CORE_ACLK_21;
			break;
		case CLK_DIV_4:
			ac_div = CORE_ACLK_41;
			break;
		case CLK_DIV_8:
			ac_div = CORE_ACLK_81;
			break;
		default:
			ac_div = CORE_ACLK_21;
			break;
	}

	g_cruReg->CRU_CLKSEL_CON[1] = (CORE_ACLK_W_MSK | (CORE_ACLK_MSK & ac_div));
}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_ahpclk_set(uint32 aclk_div, uint32 hclk_div, uint32 pclk_div, uint32 ahb2apb_div)
{
	uint32_t a_div = 0, h_div = 0, p_div = 0, apb_div = 0;

	/* cpu aclk - aclk_cpu = core_clk_src / n */
	aclk_div &= 0x1f;
	a_div = aclk_div - 1;
	g_cruReg->CRU_CLKSEL_CON[0] = (CPU_CLK_DIV_W_MSK | (CPU_CLK_DIV_MSK & a_div));

	/* cpu hclk - aclk_cpu:hclk_cpu */
	hclk_div &= 0x03;
	switch (hclk_div)
	{
		case CLK_DIV_1:
			h_div = ACLK_HCLK_11;
			break;
		case CLK_DIV_2:
			h_div = ACLK_HCLK_21;
			break;
		case CLK_DIV_4:
			h_div = ACLK_HCLK_41;
			break;
		default:
			h_div = ACLK_HCLK_21;
	}

	/* cpu pclk - aclk_cpu:pclk_cpu */
	pclk_div &= 0x07;
	switch (pclk_div)
	{
		case CLK_DIV_1:
			p_div = ACLK_PCLK_11;
			break;
		case CLK_DIV_2:
			p_div = ACLK_PCLK_21;
			break;
		case CLK_DIV_4:
			p_div = ACLK_PCLK_41;
			break;
		case CLK_DIV_8:
			p_div = ACLK_PCLK_81;
			break;
		default:
			p_div = ACLK_PCLK_41;
			break;
	}

	/* cpu ahb2apb clk - hclk_cpu:pclken_ahb2apb */
	ahb2apb_div &= 0x03;
	switch (ahb2apb_div)
	{
		case CLK_DIV_1:
			apb_div = AHB2APB_11;
			break;
		case CLK_DIV_2:
			apb_div = AHB2APB_21;
			break;
		case CLK_DIV_4:
			apb_div = AHB2APB_41;
			break;
		default:
			apb_div = AHB2APB_11;
	}

	g_cruReg->CRU_CLKSEL_CON[1] = (AHB2APB_W_MSK | (AHB2APB_MSK & apb_div))
				| (ACLK_PCLK_W_MSK | (ACLK_PCLK_MSK & p_div))
				| (ACLK_HCLK_W_MSK | (ACLK_HCLK_MSK & h_div));
}


/*
 * rkplat clock set ddr clock from ddr pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_ddr_clk_set(uint32 pll_src, uint32 ddr_div)
{
	uint32_t pll_sel = 0, div = 0;

	/* cpu clock source select: 0: ddr pll, 1: general pll */
	pll_src &= 0x01;
	if(pll_src == 0) {
		pll_sel = 0;
	} else {
		pll_sel = 1;
	}

	/* ddrphy clk - clk_ddr_src:clk_ddrphy */
	ddr_div &= 0x03;
	switch (ddr_div)
	{
		case CLK_DIV_1:
			div = 0;
			break;
		case CLK_DIV_2:
			div = 1;
			break;
		case CLK_DIV_4:
			div = 2;
			break;
		default:
			div = 0;
			break;
	}

	g_cruReg->CRU_CLKSEL_CON[26] = (0x01 << (8 + 16) | (pll_sel << 8))
				| (0x03 << (0 + 16) | (div << 0));
}


static void rkclk_apll_cb(struct pll_clk_set *clkset)
{
	//rkclk_cpu_coreclk_set(CPU_SRC_ARM_PLL, clkset->core_div, clkset->core_periph_div, clkset->core_aclk_div);
	//rkclk_cpu_ahpclk_set(clkset->aclk_div, clkset->hclk_div, clkset->pclk_div, clkset->ahb2apb_div);
	  g_cruReg->CRU_CLKSEL_CON[1] = clkset->clksel1;
}


static void rkclk_gpll_cb(struct pll_clk_set *clkset)
{
	rkclk_periph_ahpclk_set(PERIPH_SRC_GENERAL_PLL, clkset->aclk_div, clkset->hclk_div, clkset->pclk_div);
}


static void rkclk_dpll_cb(struct pll_clk_set *clkset)
{
	rkclk_ddr_clk_set(DDR_SRC_DDR_PLL, clkset->core_div);
}


static uint32 rkclk_get_cpu_aclk_div(void)
{
	uint32 con, div;

	con = g_cruReg->CRU_CLKSEL_CON[0];
	div = (con & CPU_CLK_DIV_MSK) + 1;

	return div;
}


static uint32 rkclk_get_cpu_hclk_div(void)
{
	uint32 con, div;

	con = g_cruReg->CRU_CLKSEL_CON[1];
	switch (con & ACLK_HCLK_MSK)
	{
		case ACLK_HCLK_11:
			div = CLK_DIV_1;
			break;
		case ACLK_HCLK_21:
			div = CLK_DIV_2;
			break;
		case ACLK_HCLK_41:
			div = CLK_DIV_4;
			break;
		default:
			div = CLK_DIV_2;
			break;
	}

	return div;
}


static uint32 rkclk_get_cpu_pclk_div(void)
{
	uint32 con, div;

	con = g_cruReg->CRU_CLKSEL_CON[1];
	switch (con & ACLK_PCLK_MSK)
	{
		case ACLK_PCLK_11:
			div = CLK_DIV_1;
			break;
		case ACLK_PCLK_21:
			div = CLK_DIV_2;
			break;
		case ACLK_PCLK_41:
			div = CLK_DIV_4;
			break;
		case ACLK_PCLK_81:
			div = CLK_DIV_8;
			break;
		default:
			div = CLK_DIV_4;
	}

	return div;
}


static uint32 rkclk_get_periph_aclk_div(void)
{
	uint32 con, div;

	con = g_cruReg->CRU_CLKSEL_CON[10];
	div = ((con >> PERI_ACLK_DIV_OFF) & PERI_ACLK_DIV_MASK) + 1;

	return div;
}


static uint32 rkclk_get_periph_hclk_div(void)
{
	uint32 con, div;

	con = g_cruReg->CRU_CLKSEL_CON[10];
	switch ((con >> PERI_HCLK_DIV_OFF) & PERI_HCLK_DIV_MASK)
	{
		case 0:
			div = CLK_DIV_1;
			break;
		case 1:
			div = CLK_DIV_2;
			break;
		case 2:
			div = CLK_DIV_4;
			break;
		default:
			div = CLK_DIV_2;
			break;
	}

	return div;
}


static uint32 rkclk_get_periph_pclk_div(void)
{
	uint32 con, div;

	con = g_cruReg->CRU_CLKSEL_CON[10];
	switch ((con >> PERI_PCLK_DIV_OFF) & PERI_PCLK_DIV_MASK)
	{
		case 0:
			div = CLK_DIV_1;
			break;
		case 1:
			div = CLK_DIV_2;
			break;
		case 2:
			div = CLK_DIV_4;
			break;
		case 3:
			div = CLK_DIV_8;
			break;
		default:
			div = CLK_DIV_4;
			break;
	}

	return div;
}

