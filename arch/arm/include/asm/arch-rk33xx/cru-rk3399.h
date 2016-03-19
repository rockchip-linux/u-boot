/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3399_CRU_H
#define __RK3399_CRU_H


enum rk_plls_id {
	APLLL_ID = 0,
	APLLB_ID,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,
	NPLL_ID,
	VPLL_ID,

	PPLL_ID,

	END_PLL_ID
};


/*****pmucru reg offset*****/

#define PMUCRU_PLL_CON(id, i)	((id) * 0x20 + ((i) * 4))
#define PMUCRU_CLKSEL_CON	0x080
#define PMUCRU_CLKGATE_CON	0x100
#define PMUCRU_SOFTRST_CON	0x110

#define PMUCRU_CLKSELS_CON(i)	(PMUCRU_CLKSEL_CON + ((i) * 4))
#define PMUCRU_CLKGATES_CON(i)	(PMUCRU_CLKGATE_CON + ((i) * 4))
#define PMUCRU_SOFTRSTS_CON(i)	(PMUCRU_SOFTRST_CON + ((i) * 4))


/*****cru reg offset*****/

#define CRU_CLKSEL_CON		0x100
#define CRU_CLKGATE_CON		0x300
#define CRU_SOFTRST_CON		0x400
#define CRU_GLB_SRST_FST	0x500
#define CRU_GLB_SRST_SND	0x504

#define CRU_PLL_CON(id, i)	((id) * 0x20 + ((i) * 4))

#define CRU_CLKSELS_CON(i)	(CRU_CLKSEL_CON + ((i) * 4))
#define CRU_CLKGATES_CON(i)	(CRU_CLKGATE_CON + ((i) * 4))
#define CRU_SOFTRSTS_CON(i)	(CRU_SOFTRST_CON + ((i) * 4))

#define CRU_MISC_CON		(0x50C)
#define CRU_GLB_CNT_TH		(0x508)
#define CRU_GLB_RST_CON		(0x510)
#define CRU_GLB_RST_ST		(0x514)

#define CRU_SDMMC_CON0		(0x580)
#define CRU_SDMMC_CON1		(0x584)
#define CRU_SDIO0_CON0		(0x588)
#define CRU_SDIO0_CON1		(0x58C)
#define CRU_SDIO1_CON0		(0x590)
#define CRU_SDIO1_CON1		(0x594)


/********************************************************************/
#define CRU_GET_REG_BIT_VAL(reg, bits_shift)		(((reg) >> (bits_shift)) & (0x1))
#define CRU_GET_REG_BITS_VAL(reg, bits_shift, msk)	(((reg) >> (bits_shift)) & (msk))

#define CRU_SET_BIT(val, bits_shift) 			(((val) & (0x1)) << (bits_shift))
#define CRU_SET_BITS(val, bits_shift, msk)		(((val) & (msk)) << (bits_shift))

#define CRU_W_MSK(bits_shift, msk)			((msk) << ((bits_shift) + 16))
#define CRU_W_MSK_SETBIT(val, bits_shift) 		(CRU_W_MSK(bits_shift, 0x1) | CRU_SET_BIT(val, bits_shift))
#define CRU_W_MSK_SETBITS(val, bits_shift, msk)		(CRU_W_MSK(bits_shift, msk) | CRU_SET_BITS(val, bits_shift, msk))


#define PLL_SET_REFDIV(val)		CRU_W_MSK_SETBITS(val, PLL_REFDIV_SHIFT, PLL_REFDIV_MASK)
#define PLL_GET_REFDIV(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_REFDIV_SHIFT, PLL_REFDIV_MASK)

#define PLL_SET_FBDIV(val)		CRU_W_MSK_SETBITS(val, PLL_FBDIV_SHIFT, PLL_FBDIV_MASK)
#define PLL_GET_FBDIV(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_FBDIV_SHIFT, PLL_FBDIV_MASK)

#define PLL_SET_POSTDIV1(val)		CRU_W_MSK_SETBITS(val, PLL_POSTDIV1_SHIFT, PLL_POSTDIV1_MASK)
#define PLL_GET_POSTDIV1(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_POSTDIV1_SHIFT, PLL_POSTDIV1_MASK)

#define PLL_SET_POSTDIV2(val)		CRU_W_MSK_SETBITS(val, PLL_POSTDIV2_SHIFT, PLL_POSTDIV2_MASK)
#define PLL_GET_POSTDIV2(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_POSTDIV2_SHIFT, PLL_POSTDIV2_MASK)

#define PLL_SET_FRAC(val)		CRU_SET_BITS(val, PLL_FRAC_SHIFT, PLL_FRAC_MASK)
#define PLL_GET_FRAC(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_FRAC_SHIFT, PLL_FRAC_MASK)

#define PLL_SET_DSMPD(val)		CRU_W_MSK_SETBIT(val, PLL_DSMPD_SHIFT)
#define PLL_GET_DSMPD(reg)		CRU_GET_REG_BIT_VAL(reg, PLL_DSMPD_SHIFT)


/*******************PLL CON0 BITS***************************/
#define PLL_FBDIV_MASK			(0xfff)
#define PLL_FBDIV_SHIFT			(0)


/*******************PLL CON1 BITS***************************/
#define PLL_POSTDIV2_MASK		(0x7)
#define PLL_POSTDIV2_SHIFT		(12)

#define PLL_POSTDIV1_MASK		(0x7)
#define PLL_POSTDIV1_SHIFT		(8)

#define PLL_REFDIV_MASK			(0x3f)
#define PLL_REFDIV_SHIFT		(0)


/*******************PLL CON2 BITS***************************/
#define PLL_LOCK_SHIFT		(31)

#define PLL_FRAC_MASK		(0xffffff)
#define PLL_FRAC_SHIFT		(0)


/*******************PLL CON3 BITS***************************/
#define PLL_MODE_SHIFT		8
#define PLL_MODE_MSK		(3 << 8)
#define PLL_MODE_W_MSK		(PLL_MODE_MSK << 16)
#define PLL_MODE_SLOW		(0 << 8)
#define PLL_MODE_NORM		(1 << 8)
#define PLL_MODE_DEEP_SLOW	(2 << 8)

#define PLL_FOUT4PHASEPD_SHIFT	6
#define PLL_FOUT4PHASEPD_MSK	(1 << 6)
#define PLL_FOUT4PHASEPD_W_MSK	(PLL_FOUT4PHASEPD_MSK << 16)
#define PLL_FOUT4PHASEPD	(1 << 6)
#define PLL_NO_FOUT4PHASEPD	(0 << 6)

#define PLL_FOUTVCOPD_SHIFT	5
#define PLL_FOUTVCOPD_MSK	(1 << 5)
#define PLL_FOUTVCOPD_W_MSK	(PLL_FOUTVCOPD_MSK << 16)
#define PLL_FOUTVCOPD		(1 << 5)
#define PLL_NO_FOUTVCOPD	(0 << 5)

#define PLL_FOUTPOSTDIVPD_SHIFT	4
#define PLL_FOUTPOSTDIVPD_MSK	(1 << 4)
#define PLL_FOUTPOSTDIVPD_W_MSK	(PLL_FOUTPOSTDIVPD_MSK << 16)
#define PLL_FOUTPOSTDIVPD	(1 << 4)
#define PLL_NO_FOUTPOSTDIVPD	(0 << 4)

#define PLL_DSMPD_SHIFT		3
#define PLL_DSMPD_MSK		(1 << 3)
#define PLL_DSMPD_W_MSK		(PLL_DSMPD_MSK << 16)
#define PLL_DSMPD		(1 << 3)
#define PLL_NO_DSMPD		(0 << 3)

#define PLL_DACPD_SHIFT		2
#define PLL_DACPD_MSK		(1 << 2)
#define PLL_DACPD_W_MSK		(PLL_DACPD_MSK << 16)
#define PLL_DACPD		(1 << 2)
#define PLL_NO_DACPD		(0 << 2)

#define PLL_BYPASS_SHIFT	1
#define PLL_BYPASS_MSK		(1 << 1)
#define PLL_BYPASS_W_MSK	(PLL_BYPASS_MSK << 16)
#define PLL_BYPASS		(1 << 1)
#define PLL_NO_BYPASS		(0 << 1)

#define PLL_PWR_DN_SHIFT	0
#define PLL_PWR_DN_MSK		(1 << 0)
#define PLL_PWR_DN_W_MSK	(PLL_PWR_DN_MSK << 16)
#define PLL_PWR_DN		(1 << 0)
#define PLL_PWR_ON		(0 << 0)


/*******************CLKSEL0 and CLKSEL2 BITS***************************/
/* arm core axi clock div: clk = clk_src / (div_con + 1) */
#define CORE_AXI_CLK_DIV_OFF 	8
#define CORE_AXI_CLK_DIV_W_MSK	(0x1F << (8 + 16))
#define CORE_AXI_CLK_DIV_MSK	(0x1F << 8)
#define CORE_AXI_CLK_DIV(i)	((((i) - 1) & 0x1F) << 8)

/* arm core clk pll sel: aplll or apllb or or dpll or gpll */
#define CORE_SEL_PLL_W_MSK	(3 << (6 + 16))
#define CORE_SEL_PLL_MSK	(3 << 6)
#define CORE_SEL_APLLL		(0 << 6)
#define CORE_SEL_APLLB		(1 << 6)
#define CORE_SEL_DPLL		(2 << 6)
#define CORE_SEL_GPLL		(3 << 6)

/* arm core clock div: clk_core = clk_src / (div_con + 1) */
#define CORE_CLK_DIV_OFF 	0
#define CORE_CLK_DIV_W_MSK	(0x1F << (0 + 16))
#define CORE_CLK_DIV_MSK	(0x1F << 0)
#define CORE_CLK_DIV(i)		((((i) - 1) & 0x1F) << 0)


/*******************CLKSEL1 and CLKSEL3 BITS***************************/

/* arm core debug pclk clock div: clk = clk_src / (div_con + 1) */
#define DEBUG_PCLK_DIV_OFF 	8
#define DEBUG_PCLK_DIV_W_MSK	(0x1F << (8 + 16))
#define DEBUG_PCLK_DIV_MSK	(0x1F << 8)
#define DEBUG_PCLK_DIV(i)	((((i) - 1) & 0x1F) << 8)

/* arm core ATB div: clk = clk_src / (div_con + 1) */
#define CORE_ATB_DIV_OFF 	0
#define CORE_ATB_DIV_W_MSK	(0x1F << (0 + 16))
#define CORE_ATB_DIV_MSK	(0x1F << 0)
#define CORE_ATB_DIV(i)	((((i) - 1) & 0x1F) << 0)


/*******************CLKSEL14 and CLKSEL23 BITS******************/

/* peripheral bus pclk div: clk = aclk / (div_con + 1) */
#define PERIPH_PCLK_DIV_OFF 	12
#define PERIPH_PCLK_DIV_W_MSK	(0x7 << (12 + 16))
#define PERIPH_PCLK_DIV_MSK	(0x7 << 12)
#define PERIPH_PCLK_DIV(i)	((((i) - 1) & 0x7) << 12)

/* peripheral bus hclk div: clk = aclk / (div_con + 1) */
#define PERIPH_HCLK_DIV_OFF 	8
#define PERIPH_HCLK_DIV_W_MSK	(0x3 << (8 + 16))
#define PERIPH_HCLK_DIV_MSK	(0x3 << 8)
#define PERIPH_HCLK_DIV(i)	((((i) - 1) & 0x3) << 8)

/* peripheral bus clk pll sel: codec or general */
#define PERIPH_SEL_PLL_W_MSK	(1 << (7 + 16))
#define PERIPH_SEL_PLL_MSK	(1 << 7)
#define PERIPH_SEL_CPLL		(0 << 7)
#define PERIPH_SEL_GPLL		(1 << 7)

/* peripheral bus aclk div: clk_src / (div_con + 1) */
#define PERIPH_ACLK_DIV_OFF	0
#define PERIPH_ACLK_DIV_W_MSK	(0x1F << (0 + 16))
#define PERIPH_ACLK_DIV_MSK 	(0x1F << 0)
#define PERIPH_ACLK_DIV(i)	((((i) - 1) & 0x1F) << 0)

/***********************CLKSEL25 BITS************************/

/* peripheral bus l0 pclk div: clk = hclk / (div_con + 1) */
#define PERI_L1_PCLK_DIV_OFF 	8
#define PERI_L1_PCLK_DIV_W_MSK	(0x7 << (8 + 16))
#define PERI_L1_PCLK_DIV_MSK	(0x7 << 8)
#define PERI_L1_PCLK_DIV(i)	((((i) - 1) & 0x7) << 8)

/* peripheral bus l0 hclk pll sel: codec or general */
#define PERI_L1_SEL_PLL_W_MSK	(1 << (7 + 16))
#define PERI_L1_SEL_PLL_MSK	(1 << 7)
#define PERI_L1_SEL_CPLL	(0 << 7)
#define PERI_L1_SEL_GPLL	(1 << 7)

/* peripheral bus l0 hclk div: clk = clk_src / (div_con + 1) */
#define PERI_L1_HCLK_DIV_OFF 	0
#define PERI_L1_HCLK_DIV_W_MSK	(0x1F << (0 + 16))
#define PERI_L1_HCLK_DIV_MSK	(0x1F << 0)
#define PERI_L1_HCLK_DIV(i)	((((i) - 1) & 0x1F) << 0)


/*******************GATE BITS***************************/

#define CLK_GATE_CLKID(i)	(16 * (i))
#define CLK_GATE_CLKID_CONS(i)	CRU_CLKGATES_CON((i) / 16)

#define CLK_GATE(i)		(1 << ((i)%16))
#define CLK_UN_GATE(i)		(0)

#define CLK_GATE_W_MSK(i)	(1 << (((i) % 16) + 16))


#endif /* __RK3399_CRU_H */
