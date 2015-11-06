/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3288_CRU_H
#define __RK3288_CRU_H


enum rk_plls_id {
	APLL_ID = 0,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,
	NPLL_ID,

	END_PLL_ID
};


/*****cru reg offset*****/

#define CRU_MODE_CON		0x50
#define CRU_CLKSEL_CON		0x60
#define CRU_CLKGATE_CON		0x160
#define CRU_GLB_SRST_FST	0x1B0
#define CRU_GLB_SRST_SND	0x1B4
#define CRU_SOFTRST_CON		0x1B8

#define PLL_CONS(id, i)		((id) * 0x10 + ((i) * 4))

#define CRU_CLKSELS_CON_CNT	(43)
#define CRU_CLKSELS_CON(i)	(CRU_CLKSEL_CON + ((i) * 4))

#define CRU_CLKGATES_CON_CNT	(19)
#define CRU_CLKGATES_CON(i)	(CRU_CLKGATE_CON + ((i) * 4))

#define CRU_SOFTRSTS_CON_CNT	(12)
#define CRU_SOFTRSTS_CON(i)	(CRU_SOFTRST_CON + ((i) * 4))

#define CRU_MISC_CON		(0x1E8)
#define CRU_GLB_CNT_TH		(0x1EC)
#define CRU_GLB_RST_CON		(0x1F0)
#define CRU_GLB_RST_ST		(0x1F8)

#define CRU_SDMMC_CON0		(0x200)
#define CRU_SDMMC_CON1		(0x204)
#define CRU_SDIO0_CON0		(0x208)
#define CRU_SDIO0_CON1		(0x20C)
#define CRU_SDIO1_CON0		(0x210)
#define CRU_SDIO1_CON1		(0x214)
#define CRU_EMMC_CON0		(0x218)
#define CRU_EMMC_CON1		(0x21C)


/********************************************************************/
#define CRU_GET_REG_BITS_VAL(reg, bits_shift, msk)	(((reg) >> (bits_shift))&(msk))
#define CRU_W_MSK(bits_shift, msk)			((msk) << ((bits_shift) + 16))

#define CRU_SET_BITS(val, bits_shift, msk)		(((val)&(msk)) << (bits_shift))
#define CRU_W_MSK_SETBITS(val, bits_shift, msk) 	(CRU_W_MSK(bits_shift, msk)|CRU_SET_BITS(val, bits_shift, msk))


/*******************PLL CON0 BITS***************************/

#define PLL_CLKFACTOR_SET(val, shift, msk)	((((val) - 1) & (msk)) << (shift))
#define PLL_CLKFACTOR_GET(reg, shift, msk)	((((reg) >> (shift)) & (msk)) + 1)

#define PLL_OD_MSK		(0x0F)
#define PLL_OD_SHIFT 		(0x0)

#define PLL_CLKOD(val)		PLL_CLKFACTOR_SET(val, PLL_OD_SHIFT, PLL_OD_MSK)
#define PLL_NO(reg)		PLL_CLKFACTOR_GET(reg, PLL_OD_SHIFT, PLL_OD_MSK)
#define PLL_NO_SHIFT(reg)	PLL_CLKFACTOR_GET(reg, PLL_OD_SHIFT, PLL_OD_MSK)
#define PLL_CLKOD_SET(val)	(PLL_CLKOD(val) | CRU_W_MSK(PLL_OD_SHIFT, PLL_OD_MSK))

#define PLL_NR_MSK		(0x3F)
#define PLL_NR_SHIFT		(8)
#define PLL_CLKR(val)		PLL_CLKFACTOR_SET(val, PLL_NR_SHIFT, PLL_NR_MSK)
#define PLL_NR(reg)		PLL_CLKFACTOR_GET(reg, PLL_NR_SHIFT, PLL_NR_MSK)
#define PLL_CLKR_SET(val)	(PLL_CLKR(val) | CRU_W_MSK(PLL_NR_SHIFT, PLL_NR_MSK))


/*******************PLL CON1 BITS***************************/

#define PLL_NF_MSK		(0x1FFF)
#define PLL_NF_SHIFT		(0)
#define PLL_CLKF(val)		PLL_CLKFACTOR_SET(val, PLL_NF_SHIFT, PLL_NF_MSK)
#define PLL_NF(reg)		PLL_CLKFACTOR_GET(reg, PLL_NF_SHIFT, PLL_NF_MSK)
#define PLL_CLKF_SET(val)	(PLL_CLKF(val) | CRU_W_MSK(PLL_NF_SHIFT, PLL_NF_MSK))


/*******************PLL CON2 BITS***************************/

#define PLL_BWADJ_MSK		(0x0FFF)
#define PLL_BWADJ_SHIFT		(0)
#define PLL_CLK_BWADJ_SET(val)	((val) | CRU_W_MSK(PLL_BWADJ_SHIFT, PLL_BWADJ_MSK))


/*******************PLL CON3 BITS***************************/

#define PLL_BYPASS_MSK		(1 << 0)
#define PLL_BYPASS_W_MSK	(PLL_BYPASS_MSK << 16)
#define PLL_BYPASS		(1 << 0)
#define PLL_NO_BYPASS		(0 << 0)

#define PLL_PWR_DN_MSK		(1 << 1)
#define PLL_PWR_DN_W_MSK	(PLL_PWR_DN_MSK << 16)
#define PLL_PWR_DN		(1 << 1)
#define PLL_PWR_ON		(0 << 1)

#define PLL_FASTEN_MSK		(1 << 2)
#define PLL_FASTEN_W_MSK	(PLL_FASTEN_MSK << 16)
#define PLL_FASTEN		(1 << 2)
#define PLL_NO_FASTEN		(0 << 2)

#define PLL_ENSAT_MSK		(1 << 3)
#define PLL_ENSAT_W_MSK		(PLL_ENSAT_MSK << 16)
#define PLL_ENSAT		(1 << 3)
#define PLL_NO_ENSAT		(0 << 3)

#define PLL_RESET_MSK		(1 << 5)
#define PLL_RESET_W_MSK		(PLL_RESET_MSK << 16)
#define PLL_RESET		(1 << 5)
#define PLL_RESET_RESUME	(0 << 5)


/*******************MODE BITS***************************/

/*note: npll mode bit: bit14-bit15 */
#define PLL_MODE_MSK(id)	((id == NPLL_ID) ? (0x3 << 14) : (0x3 << ((id) * 4)))
#define PLL_MODE_SLOW(id)	((id == NPLL_ID) ? (0x0<<14)|(0x3<<(16+14)) : ((0x0<<((id)*4))|(0x3<<(16+(id)*4))))
#define PLL_MODE_NORM(id)	((id == NPLL_ID) ? (0x1<<14)|(0x3<<(16+14)) : ((0x1<<((id)*4))|(0x3<<(16+(id)*4))))
#define PLL_MODE_DEEP(id)	((id == NPLL_ID) ? (0x2<<14)|(0x3<<(16+14)) : ((0x2<<((id)*4))|(0x3<<(16+(id)*4))))


/*******************CLKSEL0 BITS***************************/
/* core clk pll sel: amr or general */
#define CORE_SEL_PLL_W_MSK	(1 << (15 + 16))
#define CORE_SEL_PLL_MSK	(1 << 15)
#define CORE_SEL_APLL		(0 << 15)
#define CORE_SEL_GPLL		(1 << 15)

/* a12 core clock div: clk_core = clk_src / (div_con + 1) */
#define A12_CORE_CLK_DIV_OFF 	8
#define A12_CORE_CLK_DIV_W_MSK	(0x1F << (8 + 16))
#define A12_CORE_CLK_DIV_MSK	(0x1F << 8)
#define A12_CORE_CLK_DIV(i)	((((i) - 1) & 0x1F) << 8)

/* mp core axi clock div: clk = clk_src / (div_con + 1) */
#define MP_AXI_CLK_DIV_OFF 	4
#define MP_AXI_CLK_DIV_W_MSK	(0xF << (4 + 16))
#define MP_AXI_CLK_DIV_MSK	(0xF << 4)
#define MP_AXI_CLK_DIV(i)	((((i) - 1) & 0xF) << 4)

/* m0 core axi clock div: clk = clk_src / (div_con + 1) */
#define M0_AXI_CLK_DIV_OFF 	0
#define M0_AXI_CLK_DIV_W_MSK	(0xF << (0 + 16))
#define M0_AXI_CLK_DIV_MSK	(0xF << 0)
#define M0_AXI_CLK_DIV(i)	((((i) - 1) & 0xF) << 0)


/*******************CLKSEL1 BITS***************************/

/* pd bus clk pll sel: codec or general */
#define PDBUS_SEL_PLL_W_MSK	(1 << (15 + 16))
#define PDBUS_SEL_PLL_MSK	(1 << 15)
#define PDBUS_SEL_CPLL		(0 << 15)
#define PDBUS_SEL_GPLL		(1 << 15)

/* pd bus pclk div: clk = clk_src / (div_con + 1) */
#define PDBUS_PCLK_DIV_OFF 	12
#define PDBUS_PCLK_DIV_W_MSK	(0x7 << (12 + 16))
#define PDBUS_PCLK_DIV_MSK	(0x7 << 12)
#define PDBUS_PCLK_DIV(i)	((((i) - 1) & 0x7) << 12)

/* pd bus hclk div: aclk_bus: hclk_bus = 1:1 or 2:1 or 4:1 */
#define PDBUS_HCLK_DIV_OFF 	8
#define PDBUS_HCLK_DIV_W_MSK	(0x3 << (8 + 16))
#define PDBUS_HCLK_DIV_MSK	(0x3 << 8)
#define PDBUS_ACLK2HCLK_11	(0 << 8)
#define PDBUS_ACLK2HCLK_21	(1 << 8)
#define PDBUS_ACLK2HCLK_41	(2 << 8)

/* pd bus aclk div: clk = clk_src / (div_con + 1) */
#define PDBUS_ACLK_DIV_OFF 	3
#define PDBUS_ACLK_DIV_W_MSK	(0x1F << (3 + 16))
#define PDBUS_ACLK_DIV_MSK	(0x1F << 3)
#define PDBUS_ACLK_DIV(i)	((((i) - 1) & 0x1F) << 3)

/* pd bus axi div: clk = clk_src / (div_con + 1) */
#define PDBUS_AXI_DIV_OFF 	0
#define PDBUS_AXI_DIV_W_MSK	(0x3 << (0 + 16))
#define PDBUS_AXI_DIV_MSK	(0x3 << 0)
#define PDBUS_AXI_DIV(i)	((((i) - 1) & 0x3) << 0)


/*******************CLKSEL10 BITS***************************/

/* peripheral bus clk pll sel: codec or general */
#define PERI_SEL_PLL_W_MSK	(1 << (15 + 16))
#define PERI_SEL_PLL_MSK	(1 << 15)
#define PERI_SEL_CPLL		(0 << 15)
#define PERI_SEL_GPLL		(1 << 15)

/* peripheral bus pclk div: aclk_bus: pclk_bus = 1:1 or 2:1 or 4:1 or 8:1 */
#define PERI_PCLK_DIV_OFF 	12
#define PERI_PCLK_DIV_W_MSK	(0x7 << (12 + 16))
#define PERI_PCLK_DIV_MSK	(0x7 << 12)
#define PERI_ACLK2PCLK_11	(0 << 12)
#define PERI_ACLK2PCLK_21	(1 << 12)
#define PERI_ACLK2PCLK_41	(2 << 12)
#define PERI_ACLK2PCLK_81	(3 << 12)

/* peripheral bus hclk div: aclk_bus: hclk_bus = 1:1 or 2:1 or 4:1 */
#define PERI_HCLK_DIV_OFF 	8
#define PERI_HCLK_DIV_W_MSK	(0x3 << (8 + 16))
#define PERI_HCLK_DIV_MSK	(0x3 << 8)
#define PERI_ACLK2HCLK_11	(0 << 8)
#define PERI_ACLK2HCLK_21	(1 << 8)
#define PERI_ACLK2HCLK_41	(2 << 8)

/* peripheral bus aclk div: aclk_periph = periph_clk_src / (peri_aclk_div_con + 1) */
#define PERI_ACLK_DIV_OFF	0
#define PERI_ACLK_DIV_W_MSK	(0x1F << (0 + 16))
#define PERI_ACLK_DIV_MSK 	(0x1F << 0)
#define PERI_ACLK_DIV(i)	((((i) - 1) & 0x1F) << 0)


/*******************GATE BITS***************************/

#define CLK_GATE_CLKID(i)	(16 * (i))
#define CLK_GATE_CLKID_CONS(i)	CRU_CLKGATES_CON((i) / 16)

#define CLK_GATE(i)		(1 << ((i)%16))
#define CLK_UN_GATE(i)		(0)

#define CLK_GATE_W_MSK(i)	(1 << (((i) % 16) + 16))


#endif /* __RK3288_CRU_H */
