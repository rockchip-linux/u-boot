/*
 * (C) Copyright 2016 Fuzhou Rockchip Electronics Co., Ltd
 * William Zhang, SoftWare Engineering, <william.zhang@rock-chips.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK322XH_CRU_H
#define __RK322XH_CRU_H


enum rk_plls_id {
	APLL_ID = 0,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,
	NPLL_ID = 5,

	END_PLL_ID
};


/*****cru reg offset*****/

#define CRU_CLKSEL_CON		(0x100)
#define CRU_CLKGATE_CON		(0x200)
#define CRU_SOFTRST_CON		(0x300)

#define CRU_PLL_CON(id, i)	((id) * 0x20 + ((i) * 4))

#define CRU_CLKSELS_CON_CNT	(52)
#define CRU_CLKSELS_CON(i)	(CRU_CLKSEL_CON + ((i) * 4))

#define CRU_CLKGATES_CON_CNT	(28)
#define CRU_CLKGATES_CON(i)	(CRU_CLKGATE_CON + ((i) * 4))

#define CRU_SOFTRSTS_CON_CNT	(11)
#define CRU_SOFTRSTS_CON(i)	(CRU_SOFTRST_CON + ((i) * 4))

#define CRU_MODE_CON		(0x80)
#define CRU_MISC_CON		(0x84)
#define CRU_GLB_CNT_TH		(0x90)
#define CRU_GLB_RST_ST		(0x94)
#define CRU_GLB_SRST_SND	(0x98)
#define CRU_GLB_SRST_FST	(0x9C)

#define CRU_SDMMC_CON0		(0x380)
#define CRU_SDMMC_CON1		(0x384)
#define CRU_SDIO_CON0		(0x388)
#define CRU_SDIO_CON1		(0x38C)
#define CRU_EMMC_CON0		(0x390)
#define CRU_EMMC_CON1		(0x394)
#define CRU_SDMMC_EXT_CON0	(0x380)
#define CRU_SDMMC_EXT_CON1	(0x384)


/********************************************************************/

#define CRU_GET_REG_BIT_VAL(reg, bits_shift)		(((reg) >> (bits_shift)) & (0x1))
#define CRU_GET_REG_BITS_VAL(reg, bits_shift, msk)	(((reg) >> (bits_shift)) & (msk))

#define CRU_SET_BIT(val, bits_shift)			(((val) & (0x1)) << (bits_shift))
#define CRU_SET_BITS(val, bits_shift, msk)		(((val) & (msk)) << (bits_shift))

#define CRU_W_MSK(bits_shift, msk)			((msk) << ((bits_shift) + 16))

#define CRU_W_MSK_SETBIT(val, bits_shift)		(CRU_W_MSK(bits_shift, 0x1)	\
							| CRU_SET_BIT(val, bits_shift))
#define CRU_W_MSK_SETBITS(val, bits_shift, msk)		(CRU_W_MSK(bits_shift, msk)	\
							| CRU_SET_BITS(val, bits_shift, msk))


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


/*******************PLL CON0 BITS***************************/

#define PLL_BYPASS_SHIFT		(15)
#define PLL_SET_BYPASS			(CRU_W_MSK_SETBIT(0x1, PLL_BYPASS_SHIFT))
#define PLL_SET_NO_BYPASS		(CRU_W_MSK_SETBIT(0x0, PLL_BYPASS_SHIFT))

#define PLL_POSTDIV1_MASK		(0x7)
#define PLL_POSTDIV1_SHIFT		(12)
#define PLL_FBDIV_MASK			(0xfff)
#define PLL_FBDIV_SHIFT			(0)


/*******************PLL CON1 BITS***************************/
/* power down mode */
#define PLL_PWR_DN_SRC_SEL_SHIFT	(15)
#define PLL_SET_PWR_DN_SRC_ONLY_PD1	(CRU_W_MSK_SETBIT(0x1, PLL_PWR_DN_SRC_SEL_SHIFT))
#define PLL_SET_PWR_DN_SRC_PD0		(CRU_W_MSK_SETBIT(0x0, PLL_PWR_DN_SRC_SEL_SHIFT))

#define PLL_PWR_DN1_SHIFT		(14)
#define	PLL_SET_PWR_DN1			(CRU_W_MSK_SETBIT(0x1, PLL_PWR_DN1_SHIFT))
#define	PLL_SET_PWR_ON1			(CRU_W_MSK_SETBIT(0x0, PLL_PWR_DN1_SHIFT))

#define PLL_PWR_DN0_SHIFT		(13)
#define	PLL_SET_PWR_DN0			(CRU_W_MSK_SETBIT(0x1, PLL_PWR_DN0_SHIFT))
#define	PLL_SET_PWR_ON0			(CRU_W_MSK_SETBIT(0x0, PLL_PWR_DN0_SHIFT))


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


/*******************PLL CON3 BITS***************************/


/*******************PLL CON4 BITS***************************/


/*******************MODE BITS***************************/

#define PLL_MODE_SHIFT(id)		(((id) == NPLL_ID) ? 1 : (id) * 4)
#define PLL_MODE_MSK(id)		(0x1 << PLL_MODE_SHIFT(id))

#define PLL_SET_MODE_SLOW(id)		(CRU_W_MSK_SETBIT(0x0, PLL_MODE_SHIFT(id)))
#define PLL_SET_MODE_NORM(id)		(CRU_W_MSK_SETBIT(0x1, PLL_MODE_SHIFT(id)))


/*******************CLKSEL0 BITS***************************/

/* bus clock sel: codec or general or hdmiphy */
#define BUS_SEL_PLL_OFF			13
#define BUS_SEL_PLL_W_MSK		(0x3 << (13 + 16))
#define BUS_SEL_PLL_MSK			(0x3 << 13)
#define BUS_SEL_CPLL			(0 << 13)
#define BUS_SEL_GPLL			(1 << 13)
#define BUS_SEL_HDMIPHY			(2 << 13)

/* bus aclk clock div: clk = clk_src / (div_con + 1) */
#define BUS_ACLK_DIV_OFF		8
#define BUS_ACLK_DIV_W_MSK		(0x1F << (8 + 16))
#define BUS_ACLK_DIV_MSK		(0x1F << 8)
#define BUS_ACLK_DIV(i)			((((i) - 1) & 0x1F) << 8)

/* core clk pll sel: arm or general or ddr or new */
#define CORE_SEL_PLL_OFF		6
#define CORE_SEL_PLL_W_MSK		(3 << (6 + 16))
#define CORE_SEL_PLL_MSK		(3 << 6)
#define CORE_SEL_APLL			(0 << 6)
#define CORE_SEL_GPLL			(1 << 6)
#define CORE_SEL_DPLL			(2 << 6)
#define CORE_SEL_NPLL			(3 << 6)

/* core clock div: clk_core = clk_src / (div_con + 1) */
#define CORE_CLK_DIV_OFF		0
#define CORE_CLK_DIV_W_MSK		(0x1F << (0 + 16))
#define CORE_CLK_DIV_MSK		(0x1F << 0)
#define CORE_CLK_DIV(i)			((((i) - 1) & 0x1F) << 0)


/*******************CLKSEL1 BITS***************************/

/* bus pclk pll div: pclk = aclk / (div_con + 1) */
#define BUS_PCLK_DIV_OFF		12
#define BUS_PCLK_DIV_W_MSK		(0x7 << (12 + 16))
#define BUS_PCLK_DIV_MSK		(0x7 << 12)
#define BUS_PCLK_DIV(i)			((((i) - 1) & 0x7) << 12)

/* bus hclk div: hclk = aclk / (div_con + 1) */
#define BUS_HCLK_DIV_OFF		8
#define BUS_HCLK_DIV_W_MSK		(0x3 << (8 + 16))
#define BUS_HCLK_DIV_MSK		(0x3 << 8)
#define BUS_HCLK_DIV(i)			((((i) - 1) & 0x3) << 8)

/* core aclk div: clk = clk_src / (div_con + 1) */
#define CORE_ACLK_DIV_OFF		4
#define CORE_ACLK_DIV_W_MSK		(0x7 << (4 + 16))
#define CORE_ACLK_DIV_MSK		(0x7 << 4)
#define CORE_ACLK_DIV(i)		((((i) - 1) & 0x7) << 4)

/* clk core dbg div: clk = clk_src / (div_con + 1) */
#define CORE_PCLK_DBG_DIV_OFF		0
#define CORE_PCLK_DBG_DIV_W_MSK		(0xF << (0 + 16))
#define CORE_PCLK_DBG_DIV_MSK		(0xF << 0)
#define CORE_PCLK_DBG_DIV(i)		((((i) - 1) & 0xF) << 0)


/*******************CLKSEL2 BITS***************************/

/* func 24m div: clk = clk_src / (div_con + 1) */
#define FUNC_24M_DIV_OFF		8
#define FUNC_24M_DIV_W_MSK		(0x1F << (8 + 16))
#define FUNC_24M_DIV_MSK		(0x1F << 8)
#define FUNC_24M_DIV(i)			((((i) - 1) & 0x1F) << 8)

/* test div: clk = clk_src / (div_con + 1) */
#define TEST_DIV_OFF		0
#define TEST_DIV_W_MSK		(0x1F << (0 + 16))
#define TEST_DIV_MSK		(0x1F << 0)
#define TEST_DIV(i)		((((i) - 1) & 0x1F) << 0)


/*******************CLKSEL28 BITS***************************/

/* peripheral bus clk pll sel: codec or general */
#define PERI_SEL_PLL_W_MSK	(0x3 << (6 + 16))
#define PERI_SEL_PLL_MSK	(0x3 << 6)
#define PERI_SEL_CPLL		(0 << 6)
#define PERI_SEL_GPLL		(1 << 6)
#define PERI_SEL_HDMIPHY	(2 << 6)

/* peripheral bus aclk div: clk_src / (div_con + 1) */
#define PERI_ACLK_DIV_OFF	0
#define PERI_ACLK_DIV_W_MSK	(0x1F << (0 + 16))
#define PERI_ACLK_DIV_MSK 	(0x1F << 0)
#define PERI_ACLK_DIV(i)	((((i) - 1) & 0x1F) << 0)


/*******************CLKSEL29 BITS***************************/

/* peripheral bus pclk div: aclk_bus / (div_con + 1)  */
#define PERI_PCLK_DIV_OFF 	4
#define PERI_PCLK_DIV_W_MSK	(0x7 << (4 + 16))
#define PERI_PCLK_DIV_MSK	(0x7 << 4)
#define PERI_PCLK_DIV(i)	((((i) - 1) & 0x7) << 4)

/* peripheral bus hclk div: aclk_bus / (div_con + 1) */
#define PERI_HCLK_DIV_OFF 	0
#define PERI_HCLK_DIV_W_MSK	(0x3 << (0 + 16))
#define PERI_HCLK_DIV_MSK	(0x3 << 0)
#define PERI_HCLK_DIV(i)	((((i) - 1) & 0x3) << 0)


/*******************GATE BITS***************************/

#define CLK_GATE_CLKID(i)	(16 * (i))
#define CLK_GATE_CLKID_CONS(i)	CRU_CLKGATES_CON((i) / 16)

#define CLK_GATE(i)		(1 << ((i)%16))
#define CLK_UN_GATE(i)		(0)

#define CLK_GATE_W_MSK(i)	(1 << (((i) % 16) + 16))


#endif /* __RK322XH_CRU_H */
