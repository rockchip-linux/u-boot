/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>


DECLARE_GLOBAL_DATA_PTR;


/* ARM/General/Codec pll freq config */
#define CONFIG_RKCLK_APLL_FREQ		816 /* MHZ */

#ifdef CONFIG_PRODUCT_BOX
#define CONFIG_RKCLK_GPLL_FREQ		300 /* MHZ */
#define CONFIG_RKCLK_CPLL_FREQ		594 /* MHZ */
#else
#define CONFIG_RKCLK_GPLL_FREQ		297 /* MHZ */
#define CONFIG_RKCLK_CPLL_FREQ		384 /* MHZ */
#endif

#define CONFIG_RKCLK_NPLL_FREQ		500 /* MHZ */

/* Cpu clock source select */
#define CPU_SRC_ARM_PLL			0
#define CPU_SRC_GENERAL_PLL		1

/* Periph clock source select */
#define PERIPH_SRC_GENERAL_PLL		1
#define PERIPH_SRC_CODEC_PLL		0

/* bus clock source select */
#define BUS_SRC_GENERAL_PLL		1
#define BUS_SRC_CODEC_PLL		0

/* DDR clock source select */
#define DDR_SRC_DDR_PLL			0
#define DDR_SRC_GENERAL_PLL		1


struct pll_clk_set {
	unsigned long	rate;
	u32	pllcon0;
	u32	pllcon1;
	u32	pllcon2; //bit0 - bit11: nb = bwadj+1 = nf/2
	u32	rst_dly; //us

	u8	a12_core_div;		// S0_[8, 12]
	u8	aclk_core_mp_div;	// S0_[4, 7]
	u8	aclk_core_m0_div;	// S0_[0, 3]
	u8	pad0;

	u8	pclk_dbg_div;	// S37_[9, 13]
	u8	atclk_core_div;	// S37_[4, 8]
	u8	l2ram_div;	// S37_[0, 2]
	u8	pad1;

	u8	aclk_peri_div;	// S10_[0, 4]
	u8	hclk_peri_div;	// S10_[8, 9]
	u8	pclk_peri_div;	// S10_[12, 13]
	u8	pad2;

	u8	axi_bus_div;	// S1_[0, 2]
	u8	aclk_bus_div;	// S1_[3, 7]
	u8	hclk_bus_div;	// S1_[8, 9]
	u8	pclk_bus_div;	// S1_[12, 14]
};


#define _APLL_SET_CLKS(khz, nr, nf, no, _a12_div, _mp_div, _m0_div, _l2_div, _atclk_div, _pclk_dbg_div) \
{ \
	.rate			= khz * KHZ, \
	.pllcon0		= PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no), \
	.pllcon1		= PLL_CLKF_SET(nf), \
	.pllcon2		= PLL_CLK_BWADJ_SET(nf >> 1), \
	.rst_dly		= ((nr*500)/24+1), \
	.a12_core_div		= CLK_DIV_##_a12_div, \
	.aclk_core_mp_div	= CLK_DIV_##_mp_div, \
	.aclk_core_m0_div	= CLK_DIV_##_m0_div, \
	.pclk_dbg_div		= CLK_DIV_##_pclk_dbg_div, \
	.atclk_core_div		= CLK_DIV_##_atclk_div, \
	.l2ram_div		= CLK_DIV_##_l2_div, \
}

#define _GPLL_SET_CLKS(khz, nr, nf, no, _axi_peri_div, _ahb_peri_div, _apb_peri_div, _axi_bus_div, _aclk_bus_div, _ahb_bus_div, _apb_bus_div) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no), \
	.pllcon1	= PLL_CLKF_SET(nf), \
	.pllcon2	= PLL_CLK_BWADJ_SET(nf >> 1), \
	.rst_dly	= ((nr*500)/24+1), \
	.aclk_peri_div	= CLK_DIV_##_axi_peri_div, \
	.hclk_peri_div	= CLK_DIV_##_ahb_peri_div, \
	.pclk_peri_div	= CLK_DIV_##_apb_peri_div, \
	.axi_bus_div	= CLK_DIV_##_axi_bus_div, \
	.aclk_bus_div	= CLK_DIV_##_aclk_bus_div, \
	.hclk_bus_div	= CLK_DIV_##_ahb_bus_div, \
	.pclk_bus_div	= CLK_DIV_##_apb_bus_div, \
}

#define _DPLL_SET_CLKS(khz, nr, nf, no, _ddr_div) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no), \
	.pllcon1	= PLL_CLKF_SET(nf), \
	.pllcon2	= PLL_CLK_BWADJ_SET(nf >> 1), \
	.rst_dly	= ((nr*500)/24+1), \
	.core_div	= CLK_DIV_##_ddr_div, \
}


#define _CPLL_SET_CLKS(khz, nr, nf, no) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no), \
	.pllcon1	= PLL_CLKF_SET(nf), \
	.pllcon2	= PLL_CLK_BWADJ_SET(nf >> 1), \
	.rst_dly	= ((nr*500)/24+1), \
}

struct pll_data {
	u32 id;
	u32 size;
	struct pll_clk_set *clkset;
};

#define SET_PLL_DATA(_pll_id, _table, _size) \
{\
	.id = (u32)(_pll_id), \
	.size = (u32)(_size), \
	.clkset = (struct pll_clk_set *)(_table), \
}



/* 		rk3288 pll notice 		*/
/* 
 * Fref = Fin / nr
 * Fvco = Fin * nf / nr
 * Fout = Fvco / no
 *
 * Fin value range requirement:		32KHz ~ 2200MHz
 * Fref value range requirement:	32KHz ~ 50MHz
 * Fvco value range requirement:	1100MHz ~ 2200MHz
 * Fout value range requirement:	30MHz ~ 2200MHz
 *
 */

/* apll clock table, should be from high to low */
static struct pll_clk_set apll_clks[] = {
	/* rate, nr, nf, no,		a12_div, mp_div, m0_div,	l2, atclk, pclk_dbg */
	_APLL_SET_CLKS(1008000,1, 84, 2,	1, 4, 2,		2, 4, 4),
	_APLL_SET_CLKS(816000, 1, 68, 2,	1, 4, 2,		2, 4, 4),
	_APLL_SET_CLKS(600000, 1, 50, 2,	1, 4, 2,		2, 4, 4),
};


/* gpll clock table, should be from high to low */
static struct pll_clk_set gpll_clks[] = {
	/* rate, nr, nf, no,	aclk_peri_div, hclk_peri_div, pclk_peri_div,	axi_bus_div, aclk_bus_div, hclk_bus_div, pclk_bus_div */
	_GPLL_SET_CLKS(768000, 1,  64, 2,    2, 2, 4,				1, 2, 2, 4),
	_GPLL_SET_CLKS(594000, 2, 198, 4,    2, 2, 4,				1, 2, 2, 4),
	_GPLL_SET_CLKS(300000, 1,  50, 4,    1, 2, 4,				1, 1, 2, 4),
	_GPLL_SET_CLKS(297000, 2, 198, 8,    1, 2, 4,				1, 1, 2, 4),
};


/* cpll clock table, should be from high to low */
static struct pll_clk_set cpll_clks[] = {
	/* rate, nr, nf, no */
	_CPLL_SET_CLKS(798000, 2, 133, 2),
#ifdef CONFIG_PRODUCT_BOX
	_CPLL_SET_CLKS(594000, 1, 198, 8),
#else
	_CPLL_SET_CLKS(594000, 2, 198, 4),
#endif
	_CPLL_SET_CLKS(384000, 2, 128, 4),
};

/* npll clock table, should be from high to low */
static struct pll_clk_set npll_clks[] = {
	/* rate, nr, nf, no */
	_CPLL_SET_CLKS(500000, 3, 125, 2),
};

static struct pll_data rkpll_data[END_PLL_ID] = {
	SET_PLL_DATA(APLL_ID, apll_clks, ARRAY_SIZE(apll_clks)),
	SET_PLL_DATA(DPLL_ID, NULL, 0),
	SET_PLL_DATA(CPLL_ID, cpll_clks, ARRAY_SIZE(cpll_clks)),
	SET_PLL_DATA(GPLL_ID, gpll_clks, ARRAY_SIZE(gpll_clks)),
	SET_PLL_DATA(NPLL_ID, npll_clks, ARRAY_SIZE(npll_clks)),
};


/* Waiting for pll locked by pll id */
static void rkclk_pll_wait_lock(enum rk_plls_id pll_id)
{
	uint32 pll_state[END_PLL_ID] = {1, 0, 2, 3, 4};
	uint32 bit = (0x20u << pll_state[pll_id]);

	/* delay for pll lock */
	while (1) {
		if (grf_readl(GRF_SOC_STATUS1) & bit) {
			break;
		}
		clk_loop_delayus(1);
	}
}


/* Set pll mode by id, normal mode or slow mode */
static void rkclk_pll_set_mode(enum rk_plls_id pll_id, int pll_mode)
{
	uint32 con;
	uint32 nr, dly;

	con = cru_readl(PLL_CONS(pll_id, 0));
	nr = PLL_NR(con);
	dly = (nr * 500) / 24 + 1;

	if (pll_mode == RKCLK_PLL_MODE_NORMAL) {
		cru_writel(PLL_PWR_ON | PLL_PWR_DN_W_MSK, PLL_CONS(pll_id, 3));
		clk_loop_delayus(dly);
		rkclk_pll_wait_lock(pll_id);
		/* PLL enter normal-mode */
		cru_writel(PLL_MODE_NORM(pll_id), CRU_MODE_CON);
	} else {
		/* PLL enter slow-mode */
		cru_writel(PLL_MODE_SLOW(pll_id), CRU_MODE_CON);
		cru_writel(PLL_PWR_DN | PLL_PWR_DN_W_MSK, PLL_CONS(pll_id, 3));
	}
}


/* Set pll rate by id */
static int rkclk_pll_set_rate(enum rk_plls_id pll_id, uint32 mHz, pll_callback_f cb_f)
{
	const struct pll_data *pll = NULL;
	struct pll_clk_set *clkset = NULL;
	unsigned long rate = mHz * MHZ;
	int i = 0;

	/* Find pll rate set */
	for (i = 0; i < END_PLL_ID; i++) {
		if (rkpll_data[i].id == pll_id) {
			pll = &rkpll_data[i];
			break;
		}
	}
	if ((pll == NULL) || (pll->clkset == NULL))
		return -1;

	/* Find clock set */
	for (i = 0; i < pll->size; i++) {
		if (pll->clkset[i].rate <= rate) {
			clkset = &(pll->clkset[i]);
			break;
		}
	}
	if (clkset == NULL)
		return -1;

	/* PLL enter slow-mode */
	cru_writel(PLL_MODE_SLOW(pll_id), CRU_MODE_CON);

	/* enter rest */
	cru_writel((PLL_RESET | PLL_RESET_W_MSK), PLL_CONS(pll_id, 3));

	cru_writel(clkset->pllcon0, PLL_CONS(pll_id, 0));
	cru_writel(clkset->pllcon1, PLL_CONS(pll_id, 1));
#ifdef CONFIG_PRODUCT_BOX
	if (pll_id == CPLL_ID)
		cru_writel(0, PLL_CONS(pll_id, 2));
	else
		cru_writel(clkset->pllcon2, PLL_CONS(pll_id, 2));
#else
	cru_writel(clkset->pllcon2, PLL_CONS(pll_id, 2));
#endif

	clk_loop_delayus(5);
	/* return form rest */
	cru_writel(PLL_RESET_RESUME | PLL_RESET_W_MSK, PLL_CONS(pll_id, 3));

	clk_loop_delayus(clkset->rst_dly);
	/* waiting for pll lock */
	rkclk_pll_wait_lock(pll_id);
	if (cb_f != NULL)
		cb_f(clkset);

	/* PLL enter normal-mode */
	cru_writel(PLL_MODE_NORM(pll_id), CRU_MODE_CON);

	return 0;
}


/* Get pll rate by id */
static uint32 rkclk_pll_get_rate(enum rk_plls_id pll_id)
{
	uint32 nr, no, nf;
	uint32 con;

	con = cru_readl(CRU_MODE_CON);
	con = con & PLL_MODE_MSK(pll_id);
	con = con >> (pll_id*4);
	if (con == 0) /* slow mode */
		return 24 * MHZ;
	else if (con == 1) { /* normal mode */
		con = cru_readl(PLL_CONS(pll_id, 0));
		no = PLL_NO(con);
		nr = PLL_NR(con);
		con = cru_readl(PLL_CONS(pll_id, 1));
		nf = PLL_NF(con);

		return (24 * nf / (nr * no)) * MHZ;
	} else /* deep slow mode */
		return 32768;
}


/*
 * rkplat clock set bus clock from general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_bus_ahpclk_set(uint32 pll_src, uint32 axi_div, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, axi_bus_div = 0, a_div = 0, h_div = 0, p_div = 0;

	/* pd bus clock source select: 0: codec pll, 1: general pll */
	if (pll_src == 0)
		pll_sel = PDBUS_SEL_CPLL;
	else
		pll_sel = PDBUS_SEL_GPLL;

	/* pd bus axi - clk = clk_src / (axi_div_con + 1) */
	if (axi_div == 0)
		axi_bus_div = 1;
	else
		axi_bus_div = axi_div - 1;

	/* pd bus aclk - aclk_pdbus = clk_src / (aclk_div_con + 1) */
	if (aclk_div == 0)
		a_div = 1;
	else
		a_div = aclk_div - 1;

	/* pd bus hclk - aclk_bus: hclk_bus = 1:1 or 2:1 or 4:1 */
	switch (hclk_div) {
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

	/* pd bus pclk - pclk_pdbus = clk_src / (pclk_div_con + 1) */
	if (pclk_div == 0)
		p_div = 1;
	else
		p_div = pclk_div - 1;

	cru_writel((PDBUS_SEL_PLL_W_MSK | pll_sel)
			| (PDBUS_PCLK_DIV_W_MSK | (p_div << PDBUS_PCLK_DIV_OFF))
			| (PDBUS_HCLK_DIV_W_MSK | (h_div << PDBUS_HCLK_DIV_OFF))
			| (PDBUS_ACLK_DIV_W_MSK | (a_div << PDBUS_ACLK_DIV_OFF))
			| (PDBUS_AXI_DIV_W_MSK | (axi_bus_div << PDBUS_AXI_DIV_OFF)), CRU_CLKSELS_CON(1));
}


/*
 * rkplat clock set periph clock from general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_periph_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* periph clock source select: 0: codec pll, 1: general pll */
	if (pll_src == 0)
		pll_sel = PERI_SEL_CPLL;
	else
		pll_sel = PERI_SEL_GPLL;

	/* periph aclk - aclk_periph = periph_clk_src / (peri_aclk_div_con + 1) */
	if (aclk_div == 0)
		a_div = 1;
	else
		a_div = aclk_div - 1;

	/* periph hclk - aclk_bus: hclk_bus = 1:1 or 2:1 or 4:1 */
	switch (hclk_div) {
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

	/* periph pclk - aclk_bus: pclk_bus = 1:1 or 2:1 or 4:1 or 8:1 */
	switch (pclk_div) {
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

	cru_writel((PERI_SEL_PLL_W_MSK | pll_sel)
			| (PERI_PCLK_DIV_W_MSK | (p_div << PERI_PCLK_DIV_OFF))
			| (PERI_HCLK_DIV_W_MSK | (h_div << PERI_HCLK_DIV_OFF))
			| (PERI_ACLK_DIV_W_MSK | (a_div << PERI_ACLK_DIV_OFF)), CRU_CLKSELS_CON(10));
}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_coreclk_set(uint32 pll_src, uint32 a12_core_div, uint32 aclk_core_mp_div, uint32 aclk_core_m0_div)
{
	uint32_t pll_sel = 0, a12_div = 0, mp_div = 0, m0_div = 0;

	/* cpu clock source select: 0: arm pll, 1: general pll */
	if (pll_src == 0)
		pll_sel = CORE_SEL_APLL;
	else
		pll_sel = CORE_SEL_GPLL;

	/* a12 core clock div: clk_core = clk_src / (div_con + 1) */
	if (a12_core_div == 0)
		a12_div = 1;
	else
		a12_div = a12_core_div - 1;

	/* mp core axi clock div: clk = clk_src / (div_con + 1) */
	if (aclk_core_mp_div == 0)
		mp_div = 1;
	else
		mp_div = aclk_core_mp_div - 1;

	/* m0 core axi clock div: clk = clk_src / (div_con + 1) */
	if (aclk_core_m0_div == 0)
		m0_div = 1;
	else
		m0_div = aclk_core_m0_div - 1;

	cru_writel((CORE_SEL_PLL_W_MSK | pll_sel)
			| (A12_CORE_CLK_DIV_W_MSK | (a12_div << A12_CORE_CLK_DIV_OFF))
			| (MP_AXI_CLK_DIV_W_MSK | (mp_div << MP_AXI_CLK_DIV_OFF))
			| (M0_AXI_CLK_DIV_W_MSK | (m0_div << M0_AXI_CLK_DIV_OFF)), CRU_CLKSELS_CON(0));
}


/*
 * rkplat clock set l2ram/pclk_dbg/atclk from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_l2dbgatclk_set(uint32 l2ram_div, uint32 atclk_core_div, uint32 pclk_core_dbg_div)
{
	uint32_t l2_div = 0, atclk_div = 0, pclk_dbg_div = 0;

	/* l2 clock div: clk = clk_src / (clk_div_con + 1) */
	if (l2ram_div == 0)
		l2_div = 1;
	else
		l2_div = l2ram_div - 1;

	/* atclk clock div: clk = clk_src / (clk_div_con + 1) */
	if (atclk_core_div == 0)
		atclk_div = 1;
	else
		atclk_div = atclk_core_div - 1;

	/* pclk dbg clock div: clk = clk_src / (clk_div_con + 1) */
	if (pclk_core_dbg_div == 0)
		pclk_dbg_div = 1;
	else
		pclk_dbg_div = pclk_core_dbg_div - 1;

	cru_writel(((0x7 << (0 + 16)) | (l2_div << 0))
			| ((0x1f << (4 + 16)) | (atclk_div << 4))
			| ((0x1f << (9 + 16)) | (pclk_dbg_div << 9)), CRU_CLKSELS_CON(37));
}

static void rkclk_apll_cb(struct pll_clk_set *clkset)
{
	rkclk_cpu_coreclk_set(CPU_SRC_ARM_PLL, clkset->a12_core_div, clkset->aclk_core_mp_div, clkset->aclk_core_m0_div);
	rkclk_cpu_l2dbgatclk_set(clkset->l2ram_div, clkset->atclk_core_div, clkset->pclk_dbg_div);
}


static void rkclk_gpll_cb(struct pll_clk_set *clkset)
{
	rkclk_bus_ahpclk_set(BUS_SRC_GENERAL_PLL, clkset->axi_bus_div, clkset->aclk_bus_div, clkset->hclk_bus_div, clkset->pclk_bus_div);
	rkclk_periph_ahpclk_set(PERIPH_SRC_GENERAL_PLL, clkset->aclk_peri_div, clkset->hclk_peri_div, clkset->pclk_peri_div);
}


static uint32 rkclk_get_cpu_mp_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(0));
	div = ((con & MP_AXI_CLK_DIV_MSK) >> MP_AXI_CLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_cpu_m0_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(0));
	div = ((con & M0_AXI_CLK_DIV_MSK) >> M0_AXI_CLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_cpu_l2ram_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(37));
	div = (con & 0x07) + 1;

	return div;
}


static uint32 rkclk_get_bus_axi_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	div = ((con & PDBUS_AXI_DIV_MSK) >> PDBUS_AXI_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_bus_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	div = ((con & PDBUS_ACLK_DIV_MSK) >> PDBUS_ACLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_bus_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	switch ((con & PDBUS_HCLK_DIV_MSK) >> PDBUS_HCLK_DIV_OFF) {
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


static uint32 rkclk_get_bus_pclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	div = ((con & PDBUS_PCLK_DIV_MSK) >> PDBUS_PCLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_periph_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(10));
	div = ((con & PERI_ACLK_DIV_MSK) >> PERI_ACLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_periph_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(10));
	switch ((con & PERI_HCLK_DIV_MSK) >> PERI_HCLK_DIV_OFF) {
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

	con = cru_readl(CRU_CLKSELS_CON(10));
	switch ((con & PERI_PCLK_DIV_MSK) >> PERI_PCLK_DIV_OFF) {
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


/*
 * rkplat clock set pll mode
 */
void rkclk_pll_mode(int pll_id, int pll_mode)
{
	rkclk_pll_set_mode(pll_id, pll_mode);
}


/*
 * rkplat clock set pll rate by id
 */
void rkclk_set_pll_rate_by_id(enum rk_plls_id pll_id, uint32 mHz)
{
	pll_callback_f cb_f = NULL;

	if (APLL_ID == pll_id)
		cb_f = rkclk_apll_cb;
	else if (GPLL_ID == pll_id)
		cb_f = rkclk_gpll_cb;
	else
		cb_f = NULL;

	rkclk_pll_set_rate(pll_id, mHz, cb_f);
}


/*
 * rkplat clock set for arm and general pll
 */
void rkclk_set_pll(void)
{
	/*
	 * init timer7 for core, which will be used in following
	 * clk_loop_delayus().
	 */
	writel(0, 0xFF810000 + 0x30);
	writel(0xFFFFFFFF, 0xFF810000 + 0x20);
	writel(1, 0xFF810000 + 0x30);

	rkclk_pll_set_rate(APLL_ID, CONFIG_RKCLK_APLL_FREQ, rkclk_apll_cb);
	rkclk_pll_set_rate(GPLL_ID, CONFIG_RKCLK_GPLL_FREQ, rkclk_gpll_cb);
	rkclk_pll_set_rate(CPLL_ID, CONFIG_RKCLK_CPLL_FREQ, NULL);
}


/*
 * rkplat clock get pll rate by id
 */
uint32 rkclk_get_pll_rate_by_id(enum rk_plls_id pll_id)
{
	return rkclk_pll_get_rate(pll_id);
}


/*
 * rkplat clock get arm pll, general pll and so on
 */
void rkclk_get_pll(void)
{
	uint32 div;

	/* cpu / periph / ddr freq */
	gd->cpu_clk = rkclk_pll_get_rate(APLL_ID);
	gd->bus_clk = rkclk_pll_get_rate(GPLL_ID);
	gd->mem_clk = rkclk_pll_get_rate(DPLL_ID);
	gd->pci_clk = rkclk_pll_get_rate(CPLL_ID);

	/* cpu mp */
	div = rkclk_get_cpu_mp_div();
	gd->arch.cpu_mp_rate_hz = gd->cpu_clk / div;

	/* cpu mo */
	div = rkclk_get_cpu_m0_div();
	gd->arch.cpu_m0_rate_hz = gd->cpu_clk / div;

	/* cpu l2ram */
	div = rkclk_get_cpu_l2ram_div();
	gd->arch.cpu_l2ram_rate_hz = gd->cpu_clk / div;

	/* periph aclk */
	div = rkclk_get_periph_aclk_div();
	gd->arch.aclk_periph_rate_hz = gd->bus_clk / div;

	/* periph hclk */
	div = rkclk_get_periph_hclk_div();
	gd->arch.hclk_periph_rate_hz = gd->arch.aclk_periph_rate_hz / div;

	/* periph pclk */
	div = rkclk_get_periph_pclk_div();
	gd->arch.pclk_periph_rate_hz = gd->arch.aclk_periph_rate_hz / div;

	/* bus aclk */
	div = rkclk_get_bus_aclk_div();
	div *= rkclk_get_bus_axi_div();
	gd->arch.aclk_bus_rate_hz = gd->bus_clk / div;

	/* bus hclk */
	div = rkclk_get_bus_hclk_div();
	gd->arch.hclk_bus_rate_hz = gd->arch.aclk_bus_rate_hz / div;

	/* bus pclk */
	div = rkclk_get_bus_pclk_div();
	gd->arch.pclk_bus_rate_hz = gd->arch.aclk_bus_rate_hz / div;
}


/*
 * rkplat clock dump pll information
 */
void rkclk_dump_pll(void)
{
	printf("CPU's clock information:\n");

	printf("    arm pll = %ldHZ", gd->cpu_clk);
	debug(", mp_cpu = %ldHZ, m0_cpu = %ldHZ, l2ram_cpu = %ldHZ",
		gd->arch.cpu_mp_rate_hz, gd->arch.cpu_m0_rate_hz, gd->arch.cpu_l2ram_rate_hz);
	printf("\n");

	printf("    periph pll = %ldHZ", gd->bus_clk);
	debug(", aclk_periph = %ldHZ, hclk_periph = %ldHZ, pclk_periph = %ldHZ\n",
		gd->arch.aclk_periph_rate_hz, gd->arch.hclk_periph_rate_hz, gd->arch.pclk_periph_rate_hz);
	debug("               aclk_bus = %ldHZ, hclk_bus = %ldHZ, pclk_bus = %ldHZ",
		gd->arch.aclk_bus_rate_hz, gd->arch.hclk_bus_rate_hz, gd->arch.pclk_bus_rate_hz);
	printf("\n");

	printf("    ddr pll = %ldHZ\n", gd->mem_clk);

	printf("    codec pll = %ldHZ\n", gd->pci_clk);
}


static inline uint32 rkclk_gcd(uint32 numerator, uint32 denominator)
{
        uint32 a, b;

        if (!numerator || !denominator) {
                return 0;
	}

        if (numerator > denominator) {
                a = numerator;
                b = denominator;
        } else {
                a = denominator;
                b = numerator;
        }

        while (b != 0) {
                int r = b;
                b = a % b;
                a = r;
        }

        return a;
}


#define PLL_FREF_MIN_KHZ	(269)
#define PLL_FREF_MAX_KHZ	(2200*1000)

#define PLL_FVCO_MIN_KHZ	(440*1000)
#define PLL_FVCO_MAX_KHZ	(2200*1000)

#define PLL_FOUT_MIN_KHZ	(27500)
#define PLL_FOUT_MAX_KHZ	(2200*1000)


#define PLL_NF_MAX		(4096)
#define PLL_NR_MAX		(64)
#define PLL_NO_MAX		(16)

/*
 * rkplat rkclk_cal_pll_set
 * fin_hz: parent freq
 * fout_hz: child freq which request
 * nr, nf, no: pll set
 *
 */
static int rkclk_cal_pll_set(uint32 fin_khz, uint32 fout_khz, uint32 *nr_set, uint32 *nf_set, uint32 *no_set)
{
	uint32 nr = 0, nf = 0, no = 0, nonr = 0;
	uint32 nr_out = 0, nf_out = 0, no_out = 0;
	uint32 YFfenzi;
	uint32 YFfenmu;
	uint32 fref, fvco, fout;
	uint32 gcd_val = 0;
	uint32 n;

	if (!fin_khz || !fout_khz || fout_khz == fin_khz)
		return -1;

	nr_out = PLL_NR_MAX + 1;
	no_out = 0;

//	printf("rkclk_cal_pll_set fin_khz = %u, fout_khz = %u\n", fin_khz, fout_khz);
	gcd_val = rkclk_gcd(fin_khz, fout_khz);
//	printf("gcd_val = %d\n", gcd_val);
	YFfenzi = fout_khz / gcd_val;
	YFfenmu = fin_khz / gcd_val;
//	printf("YFfenzi = %d, YFfenmu = %d\n", YFfenzi, YFfenmu);

	for (n = 1; ; n++) {
		nf = YFfenzi * n;
		nonr = YFfenmu * n;
		if ((nf > PLL_NF_MAX) || (nonr > (PLL_NO_MAX * PLL_NR_MAX)))
			break;

		for (no = 1; no <= PLL_NO_MAX; no++) {
			if (!(no == 1 || !(no % 2)))
				continue;
			if (nonr % no)
				continue;

			nr = nonr / no;
			if (nr > PLL_NR_MAX)
				continue;

			fref = fin_khz / nr;
//			printf("fref = %u, PLL_FREF_MAX_KHZ = %u\n", fref, PLL_FREF_MAX_KHZ);
			if (fref < PLL_FREF_MIN_KHZ || fref > PLL_FREF_MAX_KHZ)
			       continue;

			fvco = fref * nf;
//			printf("fvco = %u, PLL_FVCO_MAX_KHZ = %u\n", fvco, PLL_FVCO_MAX_KHZ);
			if (fvco < PLL_FVCO_MIN_KHZ || fvco > PLL_FVCO_MAX_KHZ)
				continue;

			fout = fvco / no;
//			printf("fout = %u, PLL_FOUT_MAX_KHZ = %u\n", fout, PLL_FOUT_MAX_KHZ);
			if (fout < PLL_FOUT_MIN_KHZ || fout > PLL_FOUT_MAX_KHZ)
				continue;

			/* output all available PLL settings */
//			printf("rate = %luKHZ, \tnr = %d, \tnf = %d, \tno = %d\n", fout_khz, nr, nf, no);

			/* select the best from all available PLL settings */
			if ((no > no_out) || ((no == no_out) && (nr < nr_out))) {
				nr_out = nr;
				nf_out = nf;
				no_out = no;
			}
		}
	}

	/* output the best PLL setting */
	if ((nr_out <= PLL_NR_MAX) && (no_out > 0)) {
//		printf("nr_out = %d, \tnf_out = %d, \tno_out = %d\n", nr_out, nf_out, no_out);
		if (nr_set && nf_set && no_set) {
			*nr_set = nr_out;
			*nf_set = nf_out;
			*no_set = no_out;
		}

		return 0;
	}

	return -1;
}


/*
 * rkplat clock set codec pll
 */
void rkclk_set_cpll_rate(uint32 pll_hz)
{
	uint32 no = 0, nr = 0, nf = 0;
	uint32 pllcon = 0;

	if (rkclk_cal_pll_set(24000000/KHZ, pll_hz/KHZ, &nr, &nf, &no) == 0) {
//		printf("pll_hz = %d, nr = %d, nf = %d, no = %d\n", pll_hz, nr, nf, no);
		/* PLL enter slow-mode */
		cru_writel(PLL_MODE_SLOW(CPLL_ID), CRU_MODE_CON);
		/* enter rest */
		cru_writel((PLL_RESET | PLL_RESET_W_MSK), PLL_CONS(CPLL_ID, 3));

		/* pll con set */
		pllcon = PLL_CLKR_SET(nr) | PLL_CLKOD_SET(no);
		cru_writel(pllcon, PLL_CONS(CPLL_ID, 0));

		pllcon = PLL_CLKF_SET(nf);
		cru_writel(pllcon, PLL_CONS(CPLL_ID, 1));

		pllcon = PLL_CLK_BWADJ_SET(nf >> 1);
		cru_writel(pllcon, PLL_CONS(CPLL_ID, 2));

		clk_loop_delayus(5);
		/* return form rest */
		cru_writel(PLL_RESET_RESUME | PLL_RESET_W_MSK, PLL_CONS(CPLL_ID, 3));

		clk_loop_delayus((nr*500)/24+1);
		/* waiting for pll lock */
		rkclk_pll_wait_lock(CPLL_ID);

		/* PLL enter normal-mode */
		cru_writel(PLL_MODE_NORM(CPLL_ID), CRU_MODE_CON);
	}
}


/*
 * rkplat lcdc aclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc aclk source pll select) : 0 - codec pll, 1 - general pll, 2 - usbphy pll
 * div (lcdc aclk div from pll) : 0x01 - 0x20
 */
static int rkclk_lcdc_aclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;
	uint32 offset = 0;

	if (lcdc_id > 1)
		return -1;

	/* lcdc0 and lcdc1 register bit offset */
	offset = lcdc_id * 8;
	con = 0;

	/* aclk div */
	div = (div - 1) & 0x1f;
	con |= (0x1f << (offset + 16)) | (div << offset);

	/* aclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (6 + offset + 16)) | (0 << (6 + offset));
	else if (pll_sel == 1)
		con |= (3 << (6 + offset + 16)) | (1 << (6 + offset));
	else
		con |= (3 << (6 + offset + 16)) | (2 << (6 + offset));

	cru_writel(con, CRU_CLKSELS_CON(31));

	return 0;
}




/*
 * rkplat vio hclk config from aclk vio0
 * div (lcdc hclk div from aclk) : 0x01 - 0x20
 */
static int rkclk_vio_hclk_set(uint32 lcdc_id, uint32 div)
{
	uint32 con = 0;

	/* only when lcdc id is 0 */
	if (lcdc_id != 0)
		return -1;

	/* dclk div */
	div = (div - 1) & 0x1f;
	con = (0x1f << (8 + 16)) | (div << 8);

	cru_writel(con, CRU_CLKSELS_CON(28));

	return 0;
}

/*
 * rkplat lcdc dclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc dclk source pll select) : 0 - codec pll, 1 - general pll, 2 - new pll
 * div (lcdc dclk div from pll) : 0x01 - 0x100
 */
static int rkclk_lcdc_dclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;
	uint32 offset = 0;

	if (lcdc_id > 1)
		return -1;

	offset = 0;
	if (lcdc_id == 1)
		offset = 6;

	con = 0;
	/* dclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (offset + 16)) | (0 << offset);
	else if (pll_sel == 1)
		con |= (3 << (offset + 16)) | (1 << offset);
	else
		con |= (3 << (offset + 16)) | (2 << offset);

	/* dclk div */
	div = (div - 1) & 0xff;
	con |= (0xff << (8 + 16)) | (div << 8);

	if (lcdc_id == 0)
		cru_writel(con, CRU_CLKSELS_CON(27));
	else
		cru_writel(con, CRU_CLKSELS_CON(29));

	return 0;
}


/*
 * rkplat lcdc dclk and aclk parent pll source
 * 0 - codec pll, 1 - general pll
 */
/* lcdc0 as prmry (LCD) and lcdc1 as extend (HDMI) */
#ifdef CONFIG_PRODUCT_BOX
#define RK3288_LIMIT_PLL_VIO0	(594*MHZ)
#else
#define RK3288_LIMIT_PLL_VIO0	(410*MHZ)
#endif
#define RK3288_LIMIT_PLL_VIO1	(350*MHZ)

static uint32 rkclk_lcdc_dclk_to_pll(uint32 lcdc_id, uint32 rate_hz, uint32 *dclk_div)
{
	uint32 div;
	uint32 pll_hz;
	uint32 vio_limit_freq = 0;

	/* make sure general pll is 297MHz or 594MHz */
	if ((rate_hz <= gd->bus_clk) && (gd->bus_clk % rate_hz == 0)) {
		pll_hz = gd->bus_clk;
		div = rkclk_calc_clkdiv(pll_hz, rate_hz, 0);
		*dclk_div =  div;

		return 1; /* general pll */
	} else {
		// vio0 and vio linit freq select
		vio_limit_freq = (lcdc_id != 0) ? RK3288_LIMIT_PLL_VIO1 : RK3288_LIMIT_PLL_VIO0;

		div = vio_limit_freq / rate_hz;
		pll_hz = div * rate_hz;
#ifdef CONFIG_PRODUCT_BOX
		if (pll_hz == CONFIG_RKCLK_CPLL_FREQ * MHZ)
			rkclk_pll_set_rate(CPLL_ID, CONFIG_RKCLK_CPLL_FREQ, NULL);
		else
			rkclk_set_cpll_rate(pll_hz);
#else
		rkclk_set_cpll_rate(pll_hz);
#endif

		pll_hz = rkclk_pll_get_rate(CPLL_ID);
		/* codec pll rate reconfig, so we should set new rate to gd->pci_clk */
		gd->pci_clk = pll_hz;
		div = rkclk_calc_clkdiv(pll_hz, rate_hz, 0);
		*dclk_div = div;

		return 0; /* codec pll */
	}
}


/*
 * rkplat lcdc dclk and aclk parent pll source
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * dclk_hz: dclk rate
 * return dclk rate
 */
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz)
{
	uint32 pll_src;
	uint32 aclk_div, dclk_div;

	pll_src = rkclk_lcdc_dclk_to_pll(lcdc_id, dclk_hz, &dclk_div);
	rkclk_lcdc_dclk_config(lcdc_id, pll_src, dclk_div);
	if (pll_src != 0) /* gpll */
		aclk_div = rkclk_calc_clkdiv(gd->bus_clk, 300 * MHZ, 0);
	else /* cpll */
		aclk_div = 1;

	rkclk_lcdc_aclk_config(lcdc_id, pll_src, aclk_div);
	/* when set lcdc0, should vio hclk */
	if (lcdc_id == 0) {
		uint32 pll_hz;
		uint32 hclk_div;

		/* rk3288 eco chip, also set lcdc1 aclk for isp aclk0 and aclk1 should same source */
		rkclk_lcdc_aclk_config(1, pll_src, aclk_div);

		if (pll_src == 0)
			pll_hz = rkclk_pll_get_rate(CPLL_ID);
		else
			pll_hz = rkclk_pll_get_rate(GPLL_ID);

		hclk_div = rkclk_calc_clkdiv(pll_hz, 100 * MHZ, 0);
		rkclk_vio_hclk_set(lcdc_id, hclk_div);
	}

	printf("pll_src = %d, dclk_hz = %d, dclk_div = %d\n", pll_src, dclk_hz, dclk_div);
	/* return dclk rate */
	if (pll_src == 0)	/* codec pll */
		return (rkclk_pll_get_rate(CPLL_ID) / dclk_div);
	else /* general pll */
		return (rkclk_pll_get_rate(GPLL_ID) / dclk_div);
}


/*
 * rkplat set nandc clock div
 * nandc_id:	nandc id
 * pllsrc:	0: codec pll; 1: general pll;
 * freq:	nandc max freq request
 */
static int rkclk_set_nandc_div(uint32 nandc_id, uint32 pllsrc, uint32 freq)
{
	uint32 parent = 0;
	uint con = 0, div = 0, offset = 0;

	if (nandc_id == 0)
		offset = 0;
	else
		offset = 8;

	if (pllsrc == 0) {
		con = (0 << (7 + offset)) | (1 << (7 + offset + 16));
		parent = gd->pci_clk;
	} else {
		con = (1 << (7 + offset)) | (1 << (7 + offset + 16));
		parent = gd->bus_clk;
	}

	div = rkclk_calc_clkdiv(parent, freq, 0);
	if (div == 0)
		div = 1;
	con |= (((div - 1) << (0 + offset)) | (0x1f << (0 + offset + 16)));
	cru_writel(con, CRU_CLKSELS_CON(38));

	debug("nandc clock src rate = %d, div = %d\n", parent, div);
	return 0;
}


int rkclk_set_nandc_freq_from_gpll(uint32 nandc_id, uint32 freq)
{
	/* nandc clock from gpll */
	return rkclk_set_nandc_div(nandc_id, 1, freq);
}


/*
 * rk mmc clock source
 * 0: codec pll; 1: general pll; 2: 24M
 */
enum {
	MMC_CODEC_PLL = 0,
	MMC_GENERAL_PLL = 1,
	MMC_24M_PLL = 2,

	MMC_MAX_PLL
};

/* 0: codec pll; 1: general pll; 2: 24M */
static inline uint32 rkclk_mmc_pll_sel2set(uint32 pll_sel)
{
	uint32 pll_set;

	switch (pll_sel) {
	case MMC_CODEC_PLL:
		pll_set = 0;
		break;
	case MMC_GENERAL_PLL:
		pll_set = 1;
		break;
	case MMC_24M_PLL:
		pll_set = 2;
		break;
	default:
		pll_set = 2;
		break;
	}

	return pll_set;
}

static inline uint32 rkclk_mmc_pll_set2sel(uint32 pll_set)
{
	if (pll_set == 0)
		return MMC_CODEC_PLL;
	else if (pll_set == 1)
		return MMC_GENERAL_PLL;
	else if (pll_set == 2)
		return MMC_24M_PLL;
	else
		return MMC_MAX_PLL;
}

static inline uint32 rkclk_mmc_pll_sel2rate(uint32 pll_sel)
{
	if (pll_sel == MMC_CODEC_PLL)
		return gd->pci_clk;
	else if (pll_sel == MMC_GENERAL_PLL)
		return gd->bus_clk;
	else if (pll_sel == MMC_24M_PLL)
		return 24 * MHZ;
	else
		return 0;
}

static inline uint32 rkclk_mmc_pll_rate2sel(uint32 pll_rate)
{
	if (pll_rate == gd->pci_clk)
		return MMC_CODEC_PLL;
	else if (pll_rate == gd->bus_clk)
		return MMC_GENERAL_PLL;
	else if (pll_rate == (24 * MHZ))
		return MMC_24M_PLL;
	else
		return MMC_MAX_PLL;
}

/*
 * rkplat set mmc clock source
 * 0: codec pll; 1: general pll; 2: 24M
 */
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src)
{
	uint32 set = 0;

	set = rkclk_mmc_pll_sel2set(src);

	if (0 == sdid) /* sdmmc */
		cru_writel((set << 6) | (0x03 << (6 + 16)), CRU_CLKSELS_CON(11));
	else if (1 == sdid) /* sdio0 */
		cru_writel((set << 6) | (0x03 << (6 + 16)), CRU_CLKSELS_CON(12));
	else if (2 == sdid) /* emmc */
		cru_writel((set << 14) | (0x03 << (14 + 16)), CRU_CLKSELS_CON(12));
}


/*
 * rkplat get mmc clock rate
 */
uint32 rkclk_get_mmc_clk(uint32 sdid)
{
	uint32 con;
	uint32 sel;

	if (0 == sdid) { /* sdmmc */
		con =  cru_readl(CRU_CLKSELS_CON(11));
		sel = rkclk_mmc_pll_set2sel((con >> 6) & 0x3);
	} else if (1 == sdid) { /* sdio0 */
		con =  cru_readl(CRU_CLKSELS_CON(12));
		sel = rkclk_mmc_pll_set2sel((con >> 6) & 0x3);
	} else if (2 == sdid) { /* emmc */
		con =  cru_readl(CRU_CLKSELS_CON(12));
		sel = rkclk_mmc_pll_set2sel((con >> 14) & 0x3);
	} else {
		return 0;
	}

	return rkclk_mmc_pll_sel2rate(sel);
}


/*
 * rkplat get mmc clock rate from gpll
 */
uint32 rkclk_get_mmc_freq_from_gpll(uint32 sdid)
{
	/* set general pll */
	rkclk_set_mmc_clk_src(sdid, 1);

	/* emmc automic divide freq to 1/2, so here divide freq to 1/2 */
	return rkclk_get_mmc_clk(sdid) / 2;
}


/*
 * rkplat set mmc clock div
 * here no check clkgate, because chip default is enable.
 */
int rkclk_set_mmc_clk_div(uint32 sdid, uint32 div)
{
	if (div == 0)
		return -1;

	if (0 == sdid) /* sdmmc */
		cru_writel(((0x3Ful<<0)<<16) | ((div-1)<<0), CRU_CLKSELS_CON(11));
	else if (1 == sdid) /* sdio0 */
		cru_writel(((0x3Ful<<0)<<16) | ((div-1)<<0), CRU_CLKSELS_CON(12));
	else if (2 == sdid) /* emmc */
		cru_writel(((0x3Ful<<8)<<16) | ((div-1)<<8), CRU_CLKSELS_CON(12));
	else
		return -1;

	return 0;
}

/*
 * rkplat set mmc clock freq
 * here no check clkgate, because chip default is enable.
 */
int32 rkclk_set_mmc_clk_freq(uint32 sdid, uint32 freq)
{
	uint32 src_freqs[3];
	uint32 src_div = 0;
	uint32 clksel = 0;

	/*
	 * rkplat set mmc clock source
	 * 0: codec pll; 1: general pll; 2: 24M
	 */
	src_freqs[0] = gd->pci_clk / 2;
	src_freqs[1] = gd->bus_clk / 2;
	src_freqs[2] = (24 * MHZ) / 2;

	if (freq <= (12 * MHZ)) {
		clksel = MMC_24M_PLL; /* select 24 MHZ */
		src_div = (src_freqs[2] + freq - 1) / freq;
		if (((src_div & 0x1) == 1) && (src_div != 1))
			src_div++;
	} else {
		uint32 i, div, clk_freq, pre_clk_freq = 0;
		/*select best src clock*/
		for (i = 0; i < 2; i++) {
			if (0 == src_freqs[i])
				continue;

			div = (src_freqs[i] + freq - 1) / freq;
			if (((div & 0x1) == 1) && (div != 1))
				div++;
			clk_freq = src_freqs[i] / div;
			if (clk_freq > pre_clk_freq) {
				pre_clk_freq = clk_freq;
				clksel = rkclk_mmc_pll_rate2sel(src_freqs[i] * 2);
				src_div = div;
			}
		}
	}

	debug("rkclk_set_mmc_clk_freq: sdid = %d, clksel = %d, src_div = %d\n", sdid, clksel, src_div);
	if (0 == src_div)
		return 0;

	src_div &= 0x3F;    /* Max div is 0x3F */
	rkclk_set_mmc_clk_src(sdid, clksel);
	rkclk_set_mmc_clk_div(sdid, src_div);

	return rkclk_mmc_pll_sel2rate(clksel) / 2 / src_div;
}


/*
 * rkplat set mmc clock tuning
 *
 */
int rkclk_set_mmc_tuning(uint32 sdid, uint32 degree, uint32 delay_num)
{
	uint32 con;

	if (degree > 3 || delay_num > 255)
		return -1;

	if (2 != sdid)
		return -1;

	/* emmc */
	con = ((0x1ul << 0) << 16) | (1 << 0);
	cru_writel(con, CRU_EMMC_CON0);
	con = (((1 << 10) | (0xff << 2) | (3 << 0)) << 16) | (1 << 10) | (delay_num << 2) | (degree << 0);
	cru_writel(con, CRU_EMMC_CON1);
	con = ((0x1ul << 0) << 16) | (0 << 0);
	cru_writel(con, CRU_EMMC_CON0);

	return 0;
}

/*
 * rkplat disable mmc clock tuning
 */
int rkclk_disable_mmc_tuning(uint32 sdid)
{
	uint32 con;

	if (2 != sdid)
		return -1;

	/* emmc */
	con = ((0x1ul << 0) << 16) | (1 << 0);
	cru_writel(con, CRU_EMMC_CON0);
	con = (((1 << 10) | (0xff << 2) | (3 << 0)) << 16) | (0 << 10) | (0 << 2) | (0 << 0);
	cru_writel(con, CRU_EMMC_CON1);
	con = ((0x1ul << 0) << 16) | (0 << 0);
	cru_writel(con, CRU_EMMC_CON0);

	return 0;
}


/*
 * rkplat get PWM clock, from pclk_bus
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_pwm_clk(uint32 pwm_id)
{
	return gd->arch.pclk_bus_rate_hz;
}


/*
 * rkplat get I2C clock, I2c0 and i2c1 from pclk_cpu, I2c2 and i2c3 from pclk_periph
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id)
{
	if (i2c_bus_id == 0 || i2c_bus_id == 1)
		return gd->arch.pclk_bus_rate_hz;
	else
		return gd->arch.pclk_periph_rate_hz;
}


/*
 * rkplat get spi clock, spi0 and spi1 from  cpll or gpll
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_spi_clk(uint32 spi_bus)
{
	uint32 con;
	uint32 sel;
	uint32 div;

	if (spi_bus > 1)
		return 0;

	con = cru_readl(CRU_CLKSELS_CON(25));
	sel = (con >> (7 + 8 * spi_bus)) & 0x1;
	div = ((con >> (0 + 8 * spi_bus)) & 0x7F) + 1;

	/* rk3288 sd clk pll can be from codec pll/general pll, defualt codec pll */
	if (sel == 0)
		return gd->pci_clk / div;
	else
		return gd->bus_clk / div;
}


#ifdef CONFIG_SECUREBOOT_CRYPTO
/*
 * rkplat set crypto clock
 * here no check clkgate, because chip default is enable.
 */
void rkclk_set_crypto_clk(uint32 rate)
{
	uint32 parent = 0;
	uint32 div;

	parent = gd->arch.aclk_bus_rate_hz;
	div = rkclk_calc_clkdiv(parent, rate, 0);
	if (div == 0)
		div = 1;

	debug("crypto clk div = %d\n", div);
	cru_writel((3 << (6 + 16)) | ((div - 1) << 6), CRU_CLKSELS_CON(26));
}
#endif /* CONFIG_SECUREBOOT_CRYPTO */


#ifdef CONFIG_RK_GMAC
/*
 * rkplat set gmac clock
 * mode: 0 - rmii, 1 - rgmii
 * rmii gmac clock 50MHZ from rk pll, rgmii gmac clock 125MHZ from PHY
 */
void rkclk_set_gmac_clk(uint32_t mode)
{
	if (mode == 0) { /* rmii mode */
		uint32 clk_parent, clk_child;
		uint32 div;

		clk_parent = CONFIG_RKCLK_NPLL_FREQ * MHZ;
		clk_child = 50 * MHZ;
		div = rkclk_calc_clkdiv(clk_parent, clk_child, 1);
		if (div == 0)
			div = 1;

		debug("gmac rmii mode, clock from new pll, div = %d\n", div);

		/* gmac from new pll */
		cru_writel((0x1F << (8 + 16)) | (0x3 << (0 + 16)) | ((div - 1) << 8) | (0 << 0), CRU_CLKSELS_CON(21));

		rkclk_pll_set_rate(NPLL_ID, CONFIG_RKCLK_NPLL_FREQ, NULL);

		/* clock enable: mac_rx/mac_ref/mac_refout */
		cru_writel((1 << 19) | (1 << 18) | (1 << 16) | (0 << 3) | (0 << 2) | (0 << 0), CRU_CLKGATES_CON(5));
		/* clock enable: mac_tx */
		cru_writel((1 << 17) | (0 << 1), CRU_CLKGATES_CON(5));

		/* select internal divider clock from pll */
		cru_writel((1 << 20) | (0 << 4), CRU_CLKSELS_CON(21));
	} else { /* rgmii mode */
		debug("gmac rgmii mode, clock from PHY.\n");

		/* clock disable: mac_rx/mac_ref/mac_refout */
		cru_writel((1 << 19) | (1 << 18) | (1 << 16) | (1 << 3) | (1 << 2) | (1 << 0), CRU_CLKGATES_CON(5));
		/* clock enable: mac_tx */
		cru_writel((1 << 17) | (0 << 1), CRU_CLKGATES_CON(5));

		/* select external input clock from PHY */
		cru_writel((1 << 20) | (1 << 4), CRU_CLKSELS_CON(21));
	}
}
#endif

/*
 * cpu soft reset
 */
void rkcru_cpu_soft_reset(void)
{
	/* pll enter slow mode */
	writel(PLL_MODE_SLOW(APLL_ID) | PLL_MODE_SLOW(GPLL_ID) | PLL_MODE_SLOW(CPLL_ID) | PLL_MODE_SLOW(NPLL_ID), RKIO_GRF_PHYS + CRU_MODE_CON);

	/* soft reset */
	writel(0xeca8, RKIO_CRU_PHYS + CRU_GLB_SRST_SND);
}


/*
 * mmc soft reset
 */
void rkcru_mmc_soft_reset(uint32 sdmmcId)
{
	uint32 con = 0;

	if (sdmmcId == 2) {
		con = (0x01 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	} else {
		con = (0x01 << sdmmcId) | (0x01 << (sdmmcId + 16));
	}
	cru_writel(con, CRU_SOFTRSTS_CON(8));
	udelay(100);
	if (sdmmcId == 2) {
		con = (0x00 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	} else {
		con = (0x00 << sdmmcId) | (0x01 << (sdmmcId + 16));
	}
	cru_writel(con, CRU_SOFTRSTS_CON(8));
	udelay(200);
}


/*
 * i2c soft reset
 */
void rkcru_i2c_soft_reset(void)
{
	/* soft reset i2c0 - i2c5 */
	writel(0x3f << 10 | 0x3f << (10 + 16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
	mdelay(1);
	writel(0x00 << 10 | 0x3f << (10 + 16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
}
