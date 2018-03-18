/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <div64.h>
#include <asm/arch/rkplat.h>


DECLARE_GLOBAL_DATA_PTR;


/* ARM/General/Codec pll freq config */
#define CONFIG_RKCLK_APLL_FREQ		600 /* MHZ */
#define CONFIG_RKCLK_GPLL_FREQ		1200 /* MHZ */
#define CONFIG_RKCLK_CPLL_FREQ		500 /* MHZ */


/* Cpu clock source select */
#define CPU_SRC_ARM_PLL			0
#define CPU_SRC_GENERAL_PLL		1
#define CPU_SRC_DDR_PLL			2

/* Periph clock source select */
#define PERIPH_SRC_GENERAL_PLL		1
#define PERIPH_SRC_CODEC_PLL		0
#define PERIPH_SRC_HDMIPHY_PLL		2

/* bus clock source select */
#define BUS_SRC_GENERAL_PLL		1
#define BUS_SRC_CODEC_PLL		0
#define BUS_SRC_HDMIPHY_PLL		2

struct pll_clk_set {
	unsigned long	rate;
	u32	pllcon0;
	u32	pllcon1;
	u32	pllcon2; /* bit0 - bit11: nb = bwadj+1 = nf/2 */
	u32	rst_dly; /* us */

	u8	a7_core_div;
	u8	axi_core_div;
	u8	peri_core_div;
	u8	pad1;

	u8	aclk_peri_div;
	u8	hclk_peri_div;
	u8	pclk_peri_div;
	u8	pad2;

	u8	axi_bus_div;
	u8	aclk_bus_div;
	u8	hclk_bus_div;
	u8	pclk_bus_div;
};


#define _APLL_SET_CLKS(khz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,\
		_a7_div, _axi_div,  _peri_core_div) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1	= PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2	= PLL_SET_FRAC(_frac),	\
	.rst_dly	= 0, \
	.a7_core_div	= CLK_DIV_##_a7_div, \
	.axi_core_div	= CLK_DIV_##_axi_div, \
	.peri_core_div	= CLK_DIV_##_peri_core_div, \
}

#define _GPLL_SET_CLKS(khz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,\
		_axi_peri_div, _ahb_peri_div, _apb_peri_div, _aclk_bus_div, _ahb_bus_div, _apb_bus_div) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1	= PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2	= PLL_SET_FRAC(_frac),	\
	.rst_dly	= 0, \
	.aclk_peri_div	= CLK_DIV_##_axi_peri_div, \
	.hclk_peri_div	= CLK_DIV_##_ahb_peri_div, \
	.pclk_peri_div	= CLK_DIV_##_apb_peri_div, \
	.aclk_bus_div	= CLK_DIV_##_aclk_bus_div, \
	.hclk_bus_div	= CLK_DIV_##_ahb_bus_div, \
	.pclk_bus_div	= CLK_DIV_##_apb_bus_div, \
}

#define _CPLL_SET_CLKS(khz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac) \
{ \
	.rate		= khz * KHZ, \
	.pllcon0	= PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1	= PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2	= PLL_SET_FRAC(_frac),	\
	.rst_dly	= 0, \
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



/*		rk322x pll notice
 *
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
static const struct pll_clk_set apll_clks[] = {
	/* rate, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,	a7_div, axi_div, peri_div */
	_APLL_SET_CLKS(816000, 1, 68, 2, 1, 1, 0,			1, 2, 3),
	_APLL_SET_CLKS(600000, 1, 75, 3, 1, 1, 0,			1, 2, 3),
};


/* gpll clock table, should be from high to low */
static const struct pll_clk_set gpll_clks[] = {
	/* rate, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,
			aclk_peri_div, hclk_peri_div, pclk_peri_div,	aclk_bus_div, hclk_bus_div, pclk_bus_div */
	_GPLL_SET_CLKS(1200000, 1, 50, 1, 1, 1, 0,	8, 8, 16,	8, 8, 16),
	_GPLL_SET_CLKS(600000, 1, 75, 3, 1, 1, 0,	2, 2, 4,	2, 2, 4),
};


/* cpll clock table, should be from high to low */
static const struct pll_clk_set cpll_clks[] = {
	/* rate, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	_CPLL_SET_CLKS(500000, 6, 250, 2, 1, 1, 0),
};


static const struct pll_data rkpll_data[] = {
	SET_PLL_DATA(APLL_ID, apll_clks, ARRAY_SIZE(apll_clks)),
	SET_PLL_DATA(CPLL_ID, cpll_clks, ARRAY_SIZE(cpll_clks)),
	SET_PLL_DATA(GPLL_ID, gpll_clks, ARRAY_SIZE(gpll_clks)),
};


/* Waiting for pll locked by pll id */
static void rkclk_pll_wait_lock(enum rk_plls_id pll_id)
{
	int delay = 24000000;

	while (delay > 0) {
		if ((cru_readl(PLL_CONS(pll_id, 1)) & (0x1 << PLL_LOCK_SHIFT)))
			break;
		delay--;
	}
	if (delay == 0)
		do {} while (1);
}


/* Set pll mode by id, normal mode or slow mode */
static void rkclk_pll_set_mode(enum rk_plls_id pll_id, int pll_mode)
{
	uint32 dly = 1500;

	if (pll_mode == RKCLK_PLL_MODE_NORMAL) {
		cru_writel(CRU_W_MSK_SETBIT(PLL_PWR_ON, PLL_BYPASS_SHIFT), PLL_CONS(pll_id, 0));
		clk_loop_delayus(dly);
		rkclk_pll_wait_lock(pll_id);
		/* PLL enter normal-mode */
		cru_writel(PLL_MODE_NORM(pll_id), CRU_MODE_CON);
	} else {
		/* PLL enter slow-mode */
		cru_writel(PLL_MODE_SLOW(pll_id), CRU_MODE_CON);
		cru_writel(CRU_W_MSK_SETBIT(PLL_PWR_DN, PLL_BYPASS_SHIFT), PLL_CONS(pll_id, 0));
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
	cru_writel(clkset->pllcon0, PLL_CONS(pll_id, 0));
	cru_writel(clkset->pllcon1, PLL_CONS(pll_id, 1));
	cru_writel(clkset->pllcon2, PLL_CONS(pll_id, 2));

	/* delay for pll setup */
	rkclk_pll_wait_lock(pll_id);
	if (cb_f != NULL)
		cb_f(clkset);

	/* PLL enter normal-mode */
	cru_writel(PLL_MODE_NORM(pll_id), CRU_MODE_CON);

	return 0;
}


/* Get pll rate by id */
#define FRAC_MODE	0
static uint32 rkclk_pll_get_rate(enum rk_plls_id pll_id)
{
	unsigned int dsmp = 0;
	u64 rate64 = 0, frac_rate64 = 0;
	uint32 con;

	dsmp = PLL_GET_DSMPD(cru_readl(PLL_CONS(pll_id, 1)));
	con = cru_readl(CRU_MODE_CON);
	con = con & PLL_MODE_MSK(pll_id);
	con = con >> (pll_id*4);
	if (con == 0) /* slow mode */
		return 24 * MHZ;
	else if (con == 1) { /* normal mode */
		u32 pll_con0 = cru_readl(PLL_CONS(pll_id, 0));
		u32 pll_con1 = cru_readl(PLL_CONS(pll_id, 1));
		u32 pll_con2 = cru_readl(PLL_CONS(pll_id, 2));

		/* integer mode */
		rate64 = (u64)(24 * MHZ) * PLL_GET_FBDIV(pll_con0);
		do_div(rate64, PLL_GET_REFDIV(pll_con1));

		if (FRAC_MODE == dsmp) {
			/* fractional mode */
			frac_rate64 = (u64)(24 * MHZ) * PLL_GET_FRAC(pll_con2);
			do_div(frac_rate64, PLL_GET_REFDIV(pll_con1));
			rate64 += frac_rate64 >> 24;
		}
		do_div(rate64, PLL_GET_POSTDIV1(pll_con0));
		do_div(rate64, PLL_GET_POSTDIV2(pll_con1));

		return rate64;
	} else /* deep slow mode */
		return 32768;
}


/*
 * rkplat clock set bus clock from codec pll or general pll
 *	when call this function, make sure pll is in slow mode
 */
static void rkclk_bus_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* bus clock source select: 0: codec pll, 1: general pll */
	if (pll_src == BUS_SRC_CODEC_PLL)
		pll_sel = BUS_SEL_CPLL;
	else if (pll_src == BUS_SRC_GENERAL_PLL)
		pll_sel = BUS_SEL_GPLL;
	else if (pll_src == BUS_SRC_HDMIPHY_PLL)
		pll_sel = BUS_SEL_HDMIPHYPLL;
	else
		pll_sel = BUS_SEL_CPLL;

	/* bus aclk - aclk_bus = clk_src / (aclk_div_con + 1) */
	a_div = (aclk_div == 0) ? 1 : (aclk_div - 1);

	/* bus hclk -  hclk_bus = clk_src / (hclk_div_con + 1) */
	h_div = (hclk_div == 0) ? 1 : (hclk_div - 1);

	/* bus pclk - pclk_bus = clk_src / (pclk_div_con + 1) */
	p_div = (pclk_div == 0) ? 1 : (pclk_div - 1);

	cru_writel(BUS_ACLK_DIV_W_MSK | (a_div << BUS_ACLK_DIV_OFF), CRU_CLKSELS_CON(0));
	cru_writel((BUS_PCLK_DIV_W_MSK | (p_div << BUS_PCLK_DIV_OFF))
			| (BUS_HCLK_DIV_W_MSK | (h_div << BUS_HCLK_DIV_OFF)), CRU_CLKSELS_CON(1));

	cru_writel(BUS_SEL_PLL_W_MSK | pll_sel, CRU_CLKSELS_CON(0));
}


/*
 * rkplat clock set periph clock from general pll
 *	when call this function, make sure pll is in slow mode
 */
static void rkclk_periph_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* periph clock source select: 0: codec pll, 1: general pll, 2: hdmiphy pll */
	if (pll_src == PERIPH_SRC_CODEC_PLL)
		pll_sel = PERI_SEL_CPLL;
	else if (pll_src == PERIPH_SRC_GENERAL_PLL)
		pll_sel = PERI_SEL_GPLL;
	else if (pll_src == PERIPH_SRC_HDMIPHY_PLL)
		pll_sel = PERI_SEL_HDMIPHYPLL;
	else
		pll_sel = PERI_SEL_CPLL;

	/* periph aclk - aclk_periph = periph_clk_src / (peri_aclk_div_con + 1) */
	a_div = (aclk_div == 0) ? 1 : (aclk_div - 1);

	/* periph hclk - aclk_bus: hclk_bus = 1:1 or 2:1 or 4:1 */
	switch (hclk_div) {
	case CLK_DIV_1:
		h_div = 0;
		break;
	case CLK_DIV_2:
		h_div = 1;
		break;
	case CLK_DIV_4:
		h_div = 3;
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
		p_div = 3;
		break;
	case CLK_DIV_8:
		p_div = 7;
		break;
	default:
		p_div = 3;
		break;
	}

	cru_writel((PERI_PCLK_DIV_W_MSK | (p_div << PERI_PCLK_DIV_OFF))
			| (PERI_HCLK_DIV_W_MSK | (h_div << PERI_HCLK_DIV_OFF))
			| (PERI_ACLK_DIV_W_MSK | (a_div << PERI_ACLK_DIV_OFF)), CRU_CLKSELS_CON(10));

	cru_writel(PERI_SEL_PLL_W_MSK | pll_sel, CRU_CLKSELS_CON(10));
}


/*
 * rkplat clock set cpu clock from arm pll
 *	when call this function, make sure pll is in slow mode
 */
static void rkclk_core_clk_set(uint32 pll_src, uint32 a7_core_div, uint32 axi_core_div, uint32 peri_core_div)
{
	uint32_t pll_sel = 0, a7_div = 0, axi_div = 0, peri_div = 0;

	/* cpu clock source select: 0: arm pll, 1: general pll, 2: ddr pll */
	if (pll_src == CPU_SRC_ARM_PLL)
		pll_sel = CORE_SEL_APLL;
	else if (pll_src == CORE_SEL_GPLL)
		pll_sel = CORE_SEL_GPLL;
	else if (pll_src == CORE_SEL_DPLL)
		pll_sel = CORE_SEL_DPLL;
	else
		pll_sel = CORE_SEL_APLL;

	/* a7 core clock div: clk_core = clk_src / (div_con + 1) */
	a7_div = (a7_core_div == 0) ? 1 : (a7_core_div - 1);

	/* axi core clock div: clk = clk_src / (div_con + 1) */
	axi_div = (axi_core_div == 0) ? 1 : (axi_core_div - 1);

	/* peri core axi clock div: clk = clk_src / (div_con + 1) */
	peri_div = (peri_core_div == 0) ? 1 : (peri_core_div - 1);

	cru_writel((CORE_SEL_PLL_W_MSK | pll_sel)
			| (CORE_CLK_DIV_W_MSK | (a7_div << CORE_CLK_DIV_OFF)), CRU_CLKSELS_CON(0));

	cru_writel((CORE_ACLK_DIV_W_MSK | (axi_div << CORE_ACLK_DIV_OFF))
			| (CORE_PERI_DIV_W_MSK | (peri_div << CORE_PERI_DIV_OFF)), CRU_CLKSELS_CON(1));
}


static void rkclk_apll_cb(struct pll_clk_set *clkset)
{
	rkclk_core_clk_set(CPU_SRC_ARM_PLL, clkset->a7_core_div, clkset->axi_core_div, clkset->peri_core_div);
}


static void rkclk_gpll_cb(struct pll_clk_set *clkset)
{
	rkclk_bus_ahpclk_set(BUS_SRC_GENERAL_PLL, clkset->aclk_bus_div, clkset->hclk_bus_div, clkset->pclk_bus_div);
	rkclk_periph_ahpclk_set(PERIPH_SRC_GENERAL_PLL, clkset->aclk_peri_div, clkset->hclk_peri_div, clkset->pclk_peri_div);
}


static uint32 rkclk_get_bus_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(0));
	div = ((con & BUS_ACLK_DIV_MSK) >> BUS_ACLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_bus_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	div = ((con & BUS_HCLK_DIV_MSK) >> BUS_HCLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_bus_pclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	div = ((con & BUS_PCLK_DIV_MSK) >> BUS_PCLK_DIV_OFF) + 1;

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
	case 3:
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
	case 3:
		div = CLK_DIV_4;
		break;
	case 7:
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
 * rkplat clock set gpll child frequency
 */
void rkclk_set_gpll_child(void)
{
	/* sclk_rga */
	cru_writel(0x1f0003, CRU_CLKSELS_CON(22));
	/* sclk_hdcp */
	cru_writel(0x3f000300, CRU_CLKSELS_CON(23));
	/* dclk_vop */
	cru_writel(0xff000300, CRU_CLKSELS_CON(27));
	/* aclk_rkvdec sclk_vdec_cabac */
	cru_writel(0x1f1f0303, CRU_CLKSELS_CON(28));
	/* aclk_iep aclk_hdcp */
	cru_writel(0x1f1f0303, CRU_CLKSELS_CON(31));
	/* aclk_vpu */
	cru_writel(0x1f0003, CRU_CLKSELS_CON(32));
	/* aclk_rga aclk_vop */
	cru_writel(0x1f1f0303, CRU_CLKSELS_CON(33));
	/* sclk_vdec_core aclk_gpu */
	cru_writel(0x1f1f0303, CRU_CLKSELS_CON(34));
}

/*
 * rkplat clock set for arm and general pll
 */
void rkclk_set_pll(void)
{
	rkclk_pll_set_rate(APLL_ID, CONFIG_RKCLK_APLL_FREQ, rkclk_apll_cb);
	if (CONFIG_RKCLK_GPLL_FREQ > 600)
		rkclk_set_gpll_child();
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


#define VIO_ACLK_MAX	(400 * MHZ)
#define VIO_HCLK_MAX	(150 * MHZ)

/*
 * rkplat lcdc aclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0
 * pll_sel : 0 - codec pll, 1 - general pll, 2 - hdmi pll, 3 - usbphy pll
 * div (lcdc aclk div from pll) : 0x01 - 0x20
 */
static int rkclk_lcdc_aclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;

	if (lcdc_id > 0)
		return -1;

	con = 0;

	/* aclk pll source select */
	pll_sel &= 0x3;
	con |= ((3 << (5 + 16)) | (pll_sel << 5));

	/* aclk div */
	div = (div - 1) & 0x1f;
	con |= (0x1f << (0 + 16)) | (div << 0);

	cru_writel(con, CRU_CLKSELS_CON(33));

	return 0;
}

int rkclk_lcdc_aclk_set(uint32 lcdc_id, uint32 aclk_hz)
{
	uint32 aclk_info = 0;
	uint32 pll_sel = 0, div = 0;
	uint32 pll_rate = 0;

	/* lcdc aclk from general pll */
	pll_sel = 1;
	pll_rate = gd->bus_clk;

	div = rkclk_calc_clkdiv(pll_rate, aclk_hz, 0);
	aclk_info = (pll_sel << 16) | div;
	debug("rk lcdc aclk config: aclk = %dHZ, pll select = %d, div = %d\n", aclk_hz, pll_sel, div);

	rkclk_lcdc_aclk_config(lcdc_id, pll_sel, div);

	return aclk_info;
}


static int rkclk_lcdc_hclk_config(uint32 lcdc_id, uint32 div)
{
	uint32 con = 0;

	/* dclk div */
	div = (div - 1) & 0x1f;
	con = (0x1f << (0 + 16)) | (div << 0);

	cru_writel(con, CRU_CLKSELS_CON(21));

	return 0;
}

/*
 * rkplat vio hclk config from aclk vio0
 * div (lcdc hclk div from aclk) : 0x01 - 0x20
 */
static int rkclk_lcdc_hclk_set(uint32 lcdc_id, uint32 hclk_hz)
{
	uint32 div;

	div = rkclk_calc_clkdiv(VIO_ACLK_MAX, VIO_HCLK_MAX, 0);
	debug("rk lcdc hclk config: hclk = %dHZ, div = %d\n", hclk_hz, div);

	rkclk_lcdc_hclk_config(lcdc_id, div);

	return 0;
}


/*
 * rkplat lcdc dclk and aclk parent pll source
 * lcdc_id (lcdc id select) : 0 - lcdc0
 * dclk_hz: dclk rate
 * return dclk rate
 */
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz)
{
	rkclk_lcdc_aclk_set(lcdc_id, VIO_ACLK_MAX);
	rkclk_lcdc_hclk_set(lcdc_id, VIO_HCLK_MAX);

	/* rk322x dclk from hdmi pll which config in hdmi module */

	return 0;
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
	uint con = 0, div = 0;

	if (pllsrc == 0) {
		con = (0 << 14) | (1 << (14 + 16));
		parent = gd->pci_clk;
	} else {
		con = (1 << 14) | (1 << (14 + 16));
		parent = gd->bus_clk;
	}

	div = rkclk_calc_clkdiv(parent, freq, 0);
	if (div == 0)
		div = 1;
	con |= (((div - 1) << 8) | (0x1f << (8 + 16)));
	cru_writel(con, CRU_CLKSELS_CON(2));

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
 * 0: codec pll; 1: general pll; 2: usbphy 480M; 3: 24M
 */
enum {
	MMC_CODEC_PLL = 0,
	MMC_GENERAL_PLL = 1,
	MMC_USBPHY_PLL = 2,
	MMC_24M_PLL = 3,

	MMC_MAX_PLL
};

/* 0: codec pll; 1: general pll; 2: usbphy 480M; 3: 24M */
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
	case MMC_USBPHY_PLL:
		pll_set = 3;
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
	else if (pll_set == 3)
		return MMC_USBPHY_PLL;
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
	else if (pll_sel == MMC_USBPHY_PLL)
		return 480 * MHZ;
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
	else if (pll_rate == (480 * MHZ))
		return MMC_USBPHY_PLL;
	else if (pll_rate == (24 * MHZ))
		return MMC_24M_PLL;
	else
		return MMC_MAX_PLL;
}

/*
 * rkplat set mmc clock source
 * 0: codec pll; 1: general pll; 2: usbphy 480M; 3: 24M
 */
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src)
{
	uint32 set = 0;

	set = rkclk_mmc_pll_sel2set(src);

	if (0 == sdid) /* sdmmc */
		cru_writel((set << 8) | (0x03 << (8 + 16)), CRU_CLKSELS_CON(11));
	else if (1 == sdid) /* sdio0 */
		cru_writel((set << 10) | (0x03 << (10 + 16)), CRU_CLKSELS_CON(11));
	else if (2 == sdid) /* emmc */
		cru_writel((set << 12) | (0x03 << (12 + 16)), CRU_CLKSELS_CON(11));
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
		sel = rkclk_mmc_pll_set2sel((con >> 8) & 0x3);
	} else if (1 == sdid) { /* sdio0 */
		con =  cru_readl(CRU_CLKSELS_CON(11));
		sel = rkclk_mmc_pll_set2sel((con >> 10) & 0x3);
	} else if (2 == sdid) { /* emmc */
		con =  cru_readl(CRU_CLKSELS_CON(11));
		sel = rkclk_mmc_pll_set2sel((con >> 12) & 0x3);
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
		cru_writel(((0xFFul << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(11));
	else if (1 == sdid) /* sdio0 */
		cru_writel(((0xFFul << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(12));
	else if (2 == sdid) /* emmc */
		cru_writel(((0xFFul << 8) << 16) | ((div - 1) << 8), CRU_CLKSELS_CON(12));
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
	uint32 src_freqs[4];
	uint32 src_div = 0;
	uint32 clksel = 0;

	/*
	 * rkplat set mmc clock source
	 * 0: codec pll; 1: general pll; 2: usbphy 480M; 3: 24M
	 */
	src_freqs[0] = gd->pci_clk / 2;
	src_freqs[1] = gd->bus_clk / 2;
	src_freqs[2] = (480 * MHZ) / 2;
	src_freqs[3] = (24 * MHZ) / 2;

	if (freq <= (12 * MHZ)) {
		clksel = MMC_24M_PLL; /* select 24 MHZ */
		src_div = (src_freqs[3]+freq-1)/freq;
		if (((src_div & 0x1) == 1) && (src_div != 1))
			src_div++;
	} else {
		uint32 i, div, clk_freq, pre_clk_freq = 0;
		/* select best src clock */
		for (i = 0; i < 3; i++) {
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

	src_div &= 0xFF;    /* Max div is 0xFF */
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
 * rkplat get I2C clock, from pclk_bus
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id)
{
	return gd->arch.pclk_bus_rate_hz;
}


/*
 * rkplat get spi clock, spi from  cpll or gpll
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
	sel = (con >> 7) & 0x1;
	div = ((con >> 0) & 0x7F) + 1;

	/* rk322x spi clk pll can be from codec pll/general pll, default codec pll */
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

	/* rk322x crypto clk pll can be from codec pll/general pll, default codec pll */
	if ((cru_readl(CRU_CLKSELS_CON(24)) & (1 << 5)) != 0)
		parent = gd->bus_clk;
	else
		parent = gd->pci_clk;

	div = rkclk_calc_clkdiv(parent, rate, 0);
	if (div == 0)
		div = 1;

	debug("crypto clk div = %d\n", div);
	cru_writel((0x1F << (0 + 16)) | ((div - 1) << 0), CRU_CLKSELS_CON(24));
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

		clk_parent = CONFIG_RKCLK_CPLL_FREQ * MHZ;
		clk_child = 50 * MHZ;
		div = rkclk_calc_clkdiv(clk_parent, clk_child, 1);
		if (div == 0)
			div = 1;

		debug("gmac rmii mode, clock from codec pll, div = %d\n", div);

		/* gmac from codec pll */
		cru_writel((0x1 << 23) | (0x1F << 16) | (0 << 7) | ((div - 1) << 0), CRU_CLKSELS_CON(5));

		/* clock enable: mac_rx/mac_ref/mac_refout */
		cru_writel((1 << 21) | (1 << 20) | (1 << 19) | (0 << 5) | (0 << 4) | (0 << 3), CRU_CLKGATES_CON(5));
		/* clock enable: mac_tx */
		cru_writel((1 << 22) | (0 << 6), CRU_CLKGATES_CON(5));

		/* select internal divider clock from pll */
		cru_writel((1 << 21) | (0 << 5), CRU_CLKSELS_CON(5));
	} else { /* rgmii mode */
		debug("gmac rgmii mode, clock from PHY.\n");

		/* clock disable: mac_rx/mac_ref/mac_refout */
		cru_writel((1 << 21) | (1 << 20) | (1 << 19) | (1 << 5) | (1 << 4) | (1 << 3), CRU_CLKGATES_CON(5));
		/* clock enable: mac_tx */
		cru_writel((1 << 22) | (0 << 6), CRU_CLKGATES_CON(5));

		/* select external input clock from PHY */
		cru_writel((1 << 21) | (1 << 5), CRU_CLKSELS_CON(5));
	}
}
#endif

/*
 * cpu soft reset
 */
void rkcru_cpu_soft_reset(void)
{
	/* pll enter slow mode */
	writel(PLL_MODE_SLOW(APLL_ID) | PLL_MODE_SLOW(CPLL_ID) | PLL_MODE_SLOW(GPLL_ID), RKIO_GRF_PHYS + CRU_MODE_CON);

	/* soft reset */
	writel(0xeca8, RKIO_CRU_PHYS + CRU_GLB_SRST_SND);
}


/*
 * mmc soft reset
 */
void rkcru_mmc_soft_reset(uint32 sdmmcId)
{
	uint32 con = 0;

	con = (0x01 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	cru_writel(con, CRU_SOFTRSTS_CON(5));
	udelay(100);
	con = (0x00 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	cru_writel(con, CRU_SOFTRSTS_CON(5));
	udelay(200);
}


/*
 * i2c soft reset
 */
void rkcru_i2c_soft_reset(void)
{
	/* soft reset i2c0 - i2c3 */
	writel(0x7 << 11 | 0x7 << (11 + 16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
	mdelay(1);
	writel(0x00 << 11 | 0x7 << (11 + 16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
}
