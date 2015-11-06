/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3036_CRU_H
#define __RK3036_CRU_H


enum rk_plls_id {
	APLL_ID = 0,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,

	END_PLL_ID
};


/*****cru reg offset*****/

#define CRU_MODE_CON		0x40
#define CRU_CLKSEL_CON		0x44
#define CRU_CLKGATE_CON		0xD0
#define CRU_GLB_SRST_FST	0x100
#define CRU_GLB_SRST_SND	0x104
#define CRU_SOFTRST_CON		0x110

#define PLL_CONS(id, i)		((id) * 0x10 + ((i) * 4))

#define CRU_CLKSELS_CON_CNT	(35)
#define CRU_CLKSELS_CON(i)	(CRU_CLKSEL_CON + ((i) * 4))

#define CRU_CLKGATES_CON_CNT	(11)
#define CRU_CLKGATES_CON(i)	(CRU_CLKGATE_CON + ((i) * 4))

#define CRU_SOFTRSTS_CON_CNT	(9)
#define CRU_SOFTRSTS_CON(i)	(CRU_SOFTRST_CON + ((i) * 4))

#define CRU_MISC_CON		(0x134)
#define CRU_GLB_CNT_TH		(0x140)
#define CRU_GLB_RST_ST		(0x150)

#define CRU_SDMMC_CON0		(0x1C0)
#define CRU_SDMMC_CON1		(0x1C4)
#define CRU_SDIO_CON0		(0x1C8)
#define CRU_SDIO_CON1		(0x1CC)
#define CRU_EMMC_CON0		(0x1D8)
#define CRU_EMMC_CON1		(0x1DC)


#define CRU_PLL_MASK_CON	(0x1F0)


/********************************************************************/
#define CRU_GET_REG_BIT_VAL(reg, bits_shift)		(((reg) >> (bits_shift)) & (0x1))
#define CRU_GET_REG_BITS_VAL(reg, bits_shift, msk)	(((reg) >> (bits_shift)) & (msk))

#define CRU_SET_BIT(val, bits_shift) 			(((val) & (0x1)) << (bits_shift))
#define CRU_SET_BITS(val, bits_shift, msk)		(((val) & (msk)) << (bits_shift))

#define CRU_W_MSK(bits_shift, msk)			((msk) << ((bits_shift) + 16))

#define CRU_W_MSK_SETBITS(val, bits_shift, msk) 	(CRU_W_MSK(bits_shift, msk)	\
							| CRU_SET_BITS(val, bits_shift, msk))
#define CRU_W_MSK_SETBIT(val, bits_shift) 		(CRU_W_MSK(bits_shift, 0x1)	\
							| CRU_SET_BIT(val, bits_shift))


#define PLL_SET_REFDIV(val)		CRU_W_MSK_SETBITS(val, PLL_REFDIV_SHIFT, PLL_REFDIV_MASK)
#define PLL_SET_FBDIV(val)		CRU_W_MSK_SETBITS(val, PLL_FBDIV_SHIFT, PLL_FBDIV_MASK)
#define PLL_SET_POSTDIV1(val)		CRU_W_MSK_SETBITS(val, PLL_POSTDIV1_SHIFT, PLL_POSTDIV1_MASK)
#define PLL_SET_POSTDIV2(val)		CRU_W_MSK_SETBITS(val, PLL_POSTDIV2_SHIFT, PLL_POSTDIV2_MASK)
#define PLL_SET_FRAC(val)		CRU_SET_BITS(val, PLL_FRAC_SHIFT, PLL_FRAC_MASK)

#define PLL_GET_REFDIV(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_REFDIV_SHIFT, PLL_REFDIV_MASK)
#define PLL_GET_FBDIV(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_FBDIV_SHIFT, PLL_FBDIV_MASK)
#define PLL_GET_POSTDIV1(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_POSTDIV1_SHIFT, PLL_POSTDIV1_MASK)
#define PLL_GET_POSTDIV2(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_POSTDIV2_SHIFT, PLL_POSTDIV2_MASK)
#define PLL_GET_FRAC(reg)		CRU_GET_REG_BITS_VAL(reg, PLL_FRAC_SHIFT, PLL_FRAC_MASK)

#define PLL_SET_DSMPD(val)		CRU_W_MSK_SETBIT(val, PLL_DSMPD_SHIFT)
#define PLL_GET_DSMPD(reg)		CRU_GET_REG_BIT_VAL(reg, PLL_DSMPD_SHIFT)


/*****************PLL CON0-2 COMMON BITS********************/
#define PLL_PWR_ON			(0)
#define PLL_PWR_DN			(1)
#define PLL_BYPASS			(1 << 15)
#define PLL_NO_BYPASS			(0 << 15)


/*******************PLL CON0 BITS***************************/
#define PLL_BYPASS_SHIFT		(15)

#define PLL_POSTDIV1_MASK		(0x7)
#define PLL_POSTDIV1_SHIFT		(12)
#define PLL_FBDIV_MASK			(0xfff)
#define PLL_FBDIV_SHIFT			(0)


/*******************PLL CON1 BITS***************************/
#define PLL_RSTMODE_SHIFT		(15)
#define PLL_RST_SHIFT			(14)
#define PLL_PWR_DN_SHIFT		(13)
#define PLL_DSMPD_SHIFT			(12)
#define PLL_LOCK_SHIFT			(10)

#define PLL_POSTDIV2_MASK		(0x7)
#define PLL_POSTDIV2_SHIFT		(6)
#define PLL_REFDIV_MASK			(0x3f)
#define PLL_REFDIV_SHIFT		(0)


/*******************PLL CON2 BITS***************************/
#define PLL_FOUT4PHASE_PWR_DN_SHIFT	(27)
#define PLL_FOUTVCO_PWR_DN_SHIFT	(26)
#define PLL_FOUTPOSTDIV_PWR_DN_SHIFT	(25)
#define PLL_DAC_PWR_DN_SHIFT		(24)

#define PLL_FRAC_MASK			(0xffffff)
#define PLL_FRAC_SHIFT			(0)


/*******************MODE BITS***************************/
#define PLL_MODE_MSK(id)		(0x1 << ((id) * 4))
#define PLL_MODE_SHIFT(id)		((id) * 4)
#define PLL_MODE_SLOW(id)		(CRU_W_MSK_SETBIT(0x0, PLL_MODE_SHIFT(id)))
#define PLL_MODE_NORM(id)		(CRU_W_MSK_SETBIT(0x1, PLL_MODE_SHIFT(id)))


/*******************CLKSEL0 BITS***************************/
/* cpu clock sel: codec or general */
#define CPU_SEL_PLL_OFF 		14
#define CPU_SEL_PLL_W_MSK		(0x3 << (14 + 16))
#define CPU_SEL_PLL_MSK			(0x3 << 14)
#define CPU_SEL_CPLL			(0 << 14)
#define CPU_SEL_GPLL			(1 << 14)
#define CPU_SEL_GPLL_DIV2		(2 << 14)
#define CPU_SEL_GPLL_DIV3		(3 << 14)

/* cpu aclk clock div: clk = clk_src / (div_con + 1) */
#define CPU_ACLK_DIV_OFF 		8
#define CPU_ACLK_DIV_W_MSK		(0x1F << (8 + 16))
#define CPU_ACLK_DIV_MSK		(0x1F << 8)
#define CPU_ACLK_DIV(i)			((((i) - 1) & 0x1F) << 8)

/* core clk pll sel: arm or general */
#define CORE_SEL_PLL_OFF 		7
#define CORE_SEL_PLL_W_MSK		(1 << (7 + 16))
#define CORE_SEL_PLL_MSK		(1 << 7)
#define CORE_SEL_APLL			(0 << 7)
#define CORE_SEL_GPLL			(1 << 7)

/* core clock div: clk_core = clk_src / (div_con + 1) */
#define CORE_CLK_DIV_OFF 		0
#define CORE_CLK_DIV_W_MSK		(0x1F << (0 + 16))
#define CORE_CLK_DIV_MSK		(0x1F << 0)
#define CORE_CLK_DIV(i)			((((i) - 1) & 0x1F) << 0)


/*******************CLKSEL1 BITS***************************/
/* cpu pclk pll div: pclk = aclk / (div_con + 1) */
#define CPU_PCLK_DIV_OFF 		12
#define CPU_PCLK_DIV_W_MSK		(0x7 << (12 + 16))
#define CPU_PCLK_DIV_MSK		(0x7 << 12)
#define CPU_PCLK_DIV(i)			((((i) - 1) & 0x7) << 12)

/* cpu hclk div: hclk = aclk / (div_con + 1) */
#define CPU_HCLK_DIV_OFF 		8
#define CPU_HCLK_DIV_W_MSK		(0x3 << (8 + 16))
#define CPU_HCLK_DIV_MSK		(0x3 << 8)

/* core aclk div: clk = clk_src / (div_con + 1) */
#define CORE_ACLK_DIV_OFF 		4
#define CORE_ACLK_DIV_W_MSK		(0x3 << (4 + 16))
#define CORE_ACLK_DIV_MSK		(0x3 << 4)
#define CORE_ACLK_DIV(i)		((((i) - 1) & 0x3) << 4)

/* debug pclk div: clk = clk_src / (div_con + 1) */
#define DBG_PCLK_DIV_OFF 		0
#define DBG_PCLK_DIV_W_MSK		(0xF << (0 + 16))
#define DBG_PCLK_DIV_MSK		(0xF << 0)
#define DBG_PCLK_DIV(i)			((((i) - 1) & 0xF) << 0)


/*******************CLKSEL10 BITS***************************/

/* peripheral bus clk pll sel: codec or general */
#define PERI_SEL_PLL_OFF 		14
#define PERI_SEL_PLL_W_MSK		(3 << (14 + 16))
#define PERI_SEL_PLL_MSK		(3 << 14)
#define PERI_SEL_GPLL			(0 << 14)
#define PERI_SEL_CPLL			(1 << 14)
#define PERI_SEL_GPLL_DVI2		(2 << 14)
#define PERI_SEL_GPLL_DVI3		(2 << 14)

/* peripheral bus pclk div: aclk_bus: pclk_bus = 1:1 or 2:1 or 4:1 or 8:1 */
#define PERI_PCLK_DIV_OFF 		12
#define PERI_PCLK_DIV_W_MSK		(0x3 << (12 + 16))
#define PERI_PCLK_DIV_MSK		(0x3 << 12)
#define PERI_ACLK2PCLK_11		(0 << 12)
#define PERI_ACLK2PCLK_21		(1 << 12)
#define PERI_ACLK2PCLK_41		(2 << 12)
#define PERI_ACLK2PCLK_81		(3 << 12)

/* peripheral bus hclk div: aclk_bus: hclk_bus = 1:1 or 2:1 or 4:1 */
#define PERI_HCLK_DIV_OFF 		8
#define PERI_HCLK_DIV_W_MSK		(0x3 << (8 + 16))
#define PERI_HCLK_DIV_MSK		(0x3 << 8)
#define PERI_ACLK2HCLK_11		(0 << 8)
#define PERI_ACLK2HCLK_21		(1 << 8)
#define PERI_ACLK2HCLK_41		(2 << 8)

/* peripheral bus aclk div: aclk_periph = periph_clk_src / (peri_aclk_div_con + 1) */
#define PERI_ACLK_DIV_OFF		0
#define PERI_ACLK_DIV_W_MSK		(0x1F << (0 + 16))
#define PERI_ACLK_DIV_MSK 		(0x1F << 0)
#define PERI_ACLK_DIV(i)		((((i) - 1) & 0x1F) << 0)


/*******************GATE BITS***************************/

#define CLK_GATE_CLKID(i)		(16 * (i))
#define CLK_GATE_CLKID_CONS(i)		CRU_CLKGATES_CON((i) / 16)

#define CLK_GATE(i)			(1 << ((i)%16))
#define CLK_UN_GATE(i)			(0)

#define CLK_GATE_W_MSK(i)		(1 << (((i) % 16) + 16))


#endif /* __RK312X_CRU_H */
