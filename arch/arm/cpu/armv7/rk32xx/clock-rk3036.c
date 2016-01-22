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

/* ARM/General pll freq config */
#define CONFIG_RKCLK_APLL_FREQ		600 /* MHZ */
#define CONFIG_RKCLK_GPLL_FREQ		1188 /* MHZ */


/* Cpu clock source select */
#define CPU_SRC_ARM_PLL			0
#define CPU_SRC_DDR_PLL			1
#define CPU_SRC_GENERAL_PLL		2

/* core clock source select */
#define CORE_SRC_ARM_PLL		0
#define CORE_SRC_GENERAL_PLL		1

/* Periph clock source select */
#define PERIPH_SRC_ARM_PLL		0
#define PERIPH_SRC_DDR_PLL		1
#define PERIPH_SRC_GENERAL_PLL		2


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
};


#define _APLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, \
	_core_div, _core_peri_div, _core_aclk_div, _cpu_aclk_div, _cpu_hclk_div, _cpu_pclk_div) \
{ \
	.rate	= (_mhz) * KHZ,	\
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2 = PLL_SET_FRAC(_frac),	\
        .core_div = CLK_DIV_##_core_div, \
	.core_aclk_div = CLK_DIV_##_core_aclk_div, \
	.core_periph_div = CLK_DIV_##_core_peri_div, \
	.aclk_div = CLK_DIV_##_cpu_aclk_div, \
	.hclk_div = CLK_DIV_##_cpu_hclk_div, \
	.pclk_div = CLK_DIV_##_cpu_pclk_div, \
	.rst_dly = 0, \
}


#define _GPLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, \
	_aclk_div, _hclk_div, _pclk_div) \
{ \
	.rate	= (_mhz) * KHZ, \
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_DSMPD(_dsmpd) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2 = PLL_SET_FRAC(_frac),	\
	.aclk_div	= CLK_DIV_##_aclk_div, \
	.hclk_div	= CLK_DIV_##_hclk_div, \
	.pclk_div	= CLK_DIV_##_pclk_div, \
}


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


static struct pll_clk_set apll_clks[] = {
	/* _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,
		_core_div, _core_peri_div, _core_aclk_civ, _cpu_aclk_div, _cpu_hclk_div, _cpu_pclk_div */
	_APLL_SET_CLKS(816000, 1, 68, 2, 1, 1, 0,	1, 4, 4, 4, 2, 2),
	_APLL_SET_CLKS(600000, 1, 75, 3, 1, 1, 0,	1, 4, 2, 4, 2, 2),
};


static struct pll_clk_set gpll_clks[] = {
	/* _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,
		aclk_div, hclk_div, pclk_div */
	_GPLL_SET_CLKS(1188000, 2, 99, 1, 1, 1, 0,	8, 2, 2),
	_GPLL_SET_CLKS(768000, 4, 133, 1, 1, 1, 0,	6, 2, 2),
	_GPLL_SET_CLKS(594000, 2, 99, 2, 1, 1, 0,	4, 2, 2),
	_GPLL_SET_CLKS(297000, 2, 99, 4, 1, 1, 0,	2, 2, 2),
};


static struct pll_data rkpll_data[END_PLL_ID] = {
	SET_PLL_DATA(APLL_ID, apll_clks, ARRAY_SIZE(apll_clks)),
	SET_PLL_DATA(DPLL_ID, NULL, 0),
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
 * rkplat clock set periph clock from general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_periph_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* periph clock source select: 0: arm pll, 1: ddr pll, 2: general pll */
	if (pll_src == PERIPH_SRC_ARM_PLL)
		pll_sel = 0;
	else if (pll_src == PERIPH_SRC_DDR_PLL)
		pll_sel = 1;
	else
		pll_sel = 2;

	/* periph aclk - aclk_periph = periph_clk_src / n */
	a_div = aclk_div ? (aclk_div - 1) : 1;

	/* periph hclk - aclk_periph:hclk_periph */
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

	/* periph pclk - aclk_periph:pclk_periph */
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

	cru_writel((PERI_SEL_PLL_W_MSK | (pll_sel << PERI_SEL_PLL_OFF))
			| (PERI_PCLK_DIV_W_MSK | (p_div << PERI_PCLK_DIV_OFF))
			| (PERI_HCLK_DIV_W_MSK | (h_div << PERI_HCLK_DIV_OFF))
			| (PERI_ACLK_DIV_W_MSK | (a_div << PERI_ACLK_DIV_OFF)), CRU_CLKSELS_CON(10));
}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_coreclk_set(uint32 pll_src, uint32 core_div, uint32 core_periph_div, uint32 core_axi_div)
{
	uint32_t pll_sel = 0, c_div = 0, p_div = 0, a_div;

	/* cpu clock source select: 0: arm pll, 1: general pll */
	if (pll_src == CPU_SRC_ARM_PLL)
		pll_sel = 0;
	else
		pll_sel = 1;

	/* cpu core - clk_core = core_clk_src / n */
	c_div = core_div ? (core_div - 1) : 0;

	cru_writel((CORE_SEL_PLL_W_MSK | (pll_sel << CORE_SEL_PLL_OFF))
			| (CORE_CLK_DIV_W_MSK | (c_div << CORE_CLK_DIV_OFF)), CRU_CLKSELS_CON(0));

	/* core periph - clk_core:clk_core_periph */
	p_div = core_periph_div ? (core_periph_div - 1) : 0;

	/* axi core clk - clk_core:aclk_core */
	a_div = core_axi_div ? (core_axi_div - 1) : 0;

	cru_writel((CORE_ACLK_DIV_W_MSK | (a_div << CORE_ACLK_DIV_OFF))
			| (CORE_PERI_DIV_W_MSK | (p_div << CORE_PERI_DIV_OFF)), CRU_CLKSELS_CON(1));
}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32_t pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* cpu clock source select: 0: arm pll, 1: ddr pll, 2: general pll */
	if (pll_src == CPU_SRC_ARM_PLL)
		pll_sel = 0;
	else if (pll_src == CPU_SRC_DDR_PLL)
		pll_sel = 1;
	else
		pll_sel = 2;

	/* cpu aclk - aclk_cpu = core_clk_src / n */
	a_div = aclk_div - 1;

	cru_writel((CPU_SEL_PLL_W_MSK | (pll_sel << CPU_SEL_PLL_OFF))
			| (CPU_ACLK_DIV_W_MSK | (a_div << CPU_ACLK_DIV_OFF)), CRU_CLKSELS_CON(0));

	/* cpu hclk - aclk_cpu:hclk_cpu */
	h_div = hclk_div ? (hclk_div - 1) : 0;

	/* cpu pclk - aclk_cpu:pclk_cpu */
	p_div = pclk_div ? (pclk_div - 1) : 0;

	cru_writel((CPU_HCLK_DIV_W_MSK | (h_div << CPU_HCLK_DIV_OFF))
			| (CPU_PCLK_DIV_W_MSK | (p_div << CPU_PCLK_DIV_OFF)), CRU_CLKSELS_CON(1));
}


static void rkclk_apll_cb(struct pll_clk_set *clkset)
{
	rkclk_cpu_coreclk_set(CORE_SRC_ARM_PLL, clkset->core_div, clkset->core_periph_div, clkset->core_aclk_div);
	/* cpu ahp clock source select general pll */
#if 0
	rkclk_cpu_ahpclk_set(CPU_SRC_ARM_PLL, clkset->aclk_div, clkset->hclk_div, clkset->pclk_div);
#else
	if (CONFIG_RKCLK_GPLL_FREQ > 300)
		rkclk_cpu_ahpclk_set(CPU_SRC_GENERAL_PLL, 4, 2, 2);
	else
		rkclk_cpu_ahpclk_set(CPU_SRC_GENERAL_PLL, 2, 2, 2);
#endif
}


static void rkclk_gpll_cb(struct pll_clk_set *clkset)
{
	uint32 con, div;

	rkclk_periph_ahpclk_set(PERIPH_SRC_GENERAL_PLL, clkset->aclk_div, clkset->hclk_div, clkset->pclk_div);

	/* set module clock default div from general pll */
	/* nandc default clock div */
	if (CONFIG_RKCLK_GPLL_FREQ > 300)
		div = 4;
	else
		div = 2;

	con = (((div - 1) << 10) | (0x1f << (10 + 16)));
	cru_writel(con, CRU_CLKSELS_CON(16));

	/* set gpu default div set as 4:1 */
	cru_writel((0x1f << (0 + 16)) | (3 << 0), CRU_CLKSELS_CON(34));

	/* set vpu default div set as 4:1 */
	cru_writel((0x1f << (8 + 16)) | (3 << 0), CRU_CLKSELS_CON(32));
}



static uint32 rkclk_get_cpu_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(0));
	div = ((con & CPU_ACLK_DIV_MSK) >> CPU_ACLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_cpu_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	div = ((con & CPU_HCLK_DIV_MSK) >> CPU_HCLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_cpu_pclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(1));
	div = ((con & CPU_PCLK_DIV_MSK) >> CPU_PCLK_DIV_OFF) + 1;

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
	rkclk_pll_set_rate(APLL_ID, CONFIG_RKCLK_APLL_FREQ, rkclk_apll_cb);
	rkclk_pll_set_rate(GPLL_ID, CONFIG_RKCLK_GPLL_FREQ, rkclk_gpll_cb);
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

	/* cpu aclk */
	div = rkclk_get_cpu_aclk_div();
	int con = cru_readl(CRU_CLKSELS_CON(0));
	con = (con & CPU_SEL_PLL_MSK) >> CPU_SEL_PLL_OFF;
	if (con == CPU_SRC_GENERAL_PLL)
		gd->arch.aclk_cpu_rate_hz = gd->bus_clk / div;
	else if (con == CPU_SRC_DDR_PLL)
		gd->arch.aclk_cpu_rate_hz = gd->mem_clk / div;
	else
		gd->arch.aclk_cpu_rate_hz = gd->cpu_clk / div;

	/* cpu hclk */
	div = rkclk_get_cpu_hclk_div();
	gd->arch.hclk_cpu_rate_hz = gd->arch.aclk_cpu_rate_hz / div;

	/* cpu pclk */
	div = rkclk_get_cpu_pclk_div();
	gd->arch.pclk_cpu_rate_hz = gd->arch.aclk_cpu_rate_hz / div;

	/* periph aclk */
	div = rkclk_get_periph_aclk_div();
	gd->arch.aclk_periph_rate_hz = gd->bus_clk / div;

	/* periph hclk */
	div = rkclk_get_periph_hclk_div();
	gd->arch.hclk_periph_rate_hz = gd->arch.aclk_periph_rate_hz / div;

	/* periph pclk */
	div = rkclk_get_periph_pclk_div();
	gd->arch.pclk_periph_rate_hz = gd->arch.aclk_periph_rate_hz / div;
}


/*
 * rkplat clock dump pll information
 */
void rkclk_dump_pll(void)
{
	printf("CPU's clock information:\n");

	printf("    arm pll = %ldHZ", gd->cpu_clk);
	debug(", aclk_cpu = %ldHZ, hclk_cpu = %ldHZ, pclk_cpu = %ldHZ",
		gd->arch.aclk_cpu_rate_hz, gd->arch.hclk_cpu_rate_hz, gd->arch.pclk_cpu_rate_hz);
	printf("\n");

	printf("    periph pll = %ldHZ", gd->bus_clk);
	debug(", aclk_periph = %ldHZ, hclk_periph = %ldHZ, pclk_periph = %ldHZ\n",
		gd->arch.aclk_periph_rate_hz, gd->arch.hclk_periph_rate_hz, gd->arch.pclk_periph_rate_hz);
	printf("\n");

	printf("    ddr pll = %ldHZ\n", gd->mem_clk);
}


/*
 * rkplat pll select and clock div calcate
 * clock: device request freq HZ
 * even: if div needs even
 * return value:
 * high 16bit: 2 - general pll
 * low 16bit : div
 */
static uint32 rkclk_calc_pll_and_div(uint32 clock, uint32 even)
{
	uint32 div = 0, gdiv = 0;
	uint32 pll_sel = 2; /* 2: general pll */

	/* general pll div */
	gdiv = rkclk_calc_clkdiv(gd->bus_clk, clock, even);

	div = gdiv;

	return (pll_sel << 16) | div;
}


/*
 * rkplat lcdc aclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0
 * pll_sel (lcdc aclk source pll select) : 0 - codec pll, 1 - ddr pll, 2 - general pll
 * div (lcdc aclk div from pll) : 0x01 - 0x20
 */
static int rkclk_lcdc_aclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;

	if (lcdc_id > 1)
		return -1;

	con = 0;
	/* aclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (14 + 16)) | (0 << 14);
	else if (pll_sel == 1)
		con |= (3 << (14 + 16)) | (1 << 14);
	else
		con |= (3 << (14 + 16)) | (2 << 14);

	/* aclk div */
	div = div ? (div - 1) : 0;
	con |= (0x1f << (8 + 16)) | (div << 8);


	cru_writel(con, CRU_CLKSELS_CON(31));

	return 0;
}


int rkclk_lcdc_aclk_set(uint32 lcdc_id, uint32 aclk_hz)
{
	uint32 aclk_info = 0;
	uint32 pll_sel = 0, div = 0;

	aclk_info = rkclk_calc_pll_and_div(aclk_hz, 0);

	pll_sel = (aclk_info & 0xFFFF0000) >> 16;
	div = aclk_info & 0x0000FFFF;
	debug("rk lcdc aclk config: aclk = %dHZ, pll select = %d, div = %d\n", aclk_hz, pll_sel, div);

	rkclk_lcdc_aclk_config(lcdc_id, pll_sel, div);

	return aclk_info;
}


/*
 * rkplat lcdc dclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0
 * pll_sel (lcdc dclk source pll select) : 0 - arm pll, 1 - ddr pll, 2 - general pll
 * div (lcdc dclk div from pll) : 0x01 - 0x100
 */
static int rkclk_lcdc_dclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;

	if (lcdc_id > 1)
		return -1;

	con = 0;
	/* dclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (0 + 16)) | (0 << 0);
	else if (pll_sel == 1)
		con |= (3 << (0 + 16)) | (1 << 0);
	else
		con |= (3 << (0 + 16)) | (2 << 0);

	/* dclk div */
	div = div ? (div - 1) : 0;
	con |= (0xff << (8 + 16)) | (div << 8);

	cru_writel(con, CRU_CLKSELS_CON(28));

	return 0;
}


int rkclk_lcdc_dclk_set(uint32 lcdc_id, uint32 dclk_hz)
{
	uint32 dclk_info = 0;
	uint32 pll_sel = 0, div = 0;

	dclk_info = rkclk_calc_pll_and_div(dclk_hz, 0);

	pll_sel = (dclk_info & 0xFFFF0000) >> 16;
	div = dclk_info & 0x0000FFFF;
	debug("rk lcdc dclk set: dclk = %dHZ, pll select = %d, div = %d\n", dclk_hz, pll_sel, div);

	rkclk_lcdc_dclk_config(lcdc_id, pll_sel, div);

	return dclk_info;
}


/*
 * rkplat lcdc dclk and aclk parent pll source
 * lcdc_id (lcdc id select) : 0 - lcdc0
 * dclk_hz: dclk rate
 * return dclk rate
 */
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz)
{
	uint32 dclk_div;
	uint32 dclk_info = 0;

	rkclk_lcdc_aclk_set(lcdc_id, 297 * MHZ);
	dclk_info = rkclk_lcdc_dclk_set(lcdc_id, dclk_hz);

	dclk_div = dclk_info & 0x0000FFFF;
	// general pll
	return (rkclk_pll_get_rate(GPLL_ID) / dclk_div);
}


/*
 * rkplat set nandc clock div
 * nandc_id:	nandc id
 * 0: arm pll; 1: ddr pll; 2: general pll;
 */
static int rkclk_set_nandc_div(uint32 nandc_id, uint32 pllsrc, uint32 freq)
{
	uint32 parent = 0;
	uint con = 0, div = 0;

	nandc_id = nandc_id;

	if (pllsrc == 0) {
		con = (0 << 8) | (3 << (8 + 16));
		parent = gd->cpu_clk;
	} else if (pllsrc == 1) {
		con = (1 << 8) | (3 << (8 + 16));
		parent = gd->mem_clk;
	} else {
		con = (2 << 8) | (3 << (8 + 16));
		parent = gd->bus_clk;
	}

	div = rkclk_calc_clkdiv(parent, freq, 0);
	if (div == 0)
		div = 1;

	con |= (((div - 1) << 10) | (0x1f << (10 + 16)));
	cru_writel(con, CRU_CLKSELS_CON(16));

	debug("nandc clock src rate = %d, div = %d\n", parent, div);
	return 0;
}

int rkclk_set_nandc_freq_from_gpll(uint32 nandc_id, uint32 freq)
{
	/* nandc clock from gpll */
	return rkclk_set_nandc_div(nandc_id, 2, freq);
}


/*
 * rkplat set mmc clock src
 * 0: arm pll; 1: ddr pll; 2: general pll; 3: 24M
 */
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src)
{
	src &= 0x03;
	if (0 == sdid) /* sdmmc */
		cru_writel((src << 8) | (0x03 << (8 + 16)), CRU_CLKSELS_CON(12));
	else if (1 == sdid) /* sdio0 */
		cru_writel((src << 10) | (0x03 << (10 + 16)), CRU_CLKSELS_CON(12));
	else if (2 == sdid) /* emmc */
		cru_writel((src << 12) | (0x03 << (12 + 16)), CRU_CLKSELS_CON(12));
}


/*
 * rkplat get mmc clock src rate
 */
uint32 rkclk_get_mmc_clk(uint32 sdid)
{
	uint32 con;
	uint32 sel;

	if (0 == sdid) { /* sdmmc */
		con =  cru_readl(CRU_CLKSELS_CON(12));
		sel = (con >> 8) & 0x3;
	} else if (1 == sdid) { /* sdio0 */
		con =  cru_readl(CRU_CLKSELS_CON(12));
		sel = (con >> 10) & 0x3;
	} else if (2 == sdid) { /* emmc */
		con =  cru_readl(CRU_CLKSELS_CON(12));
		sel = (con >> 12) & 0x3;
	} else {
		return 0;
	}

	/* rk3036 sd clk pll can be from arm pll/ddr pll/general pll/24M, defualt general pll */
	if (sel == 0)
		return gd->cpu_clk;
	else if (sel == 1)
		return gd->mem_clk;
	else if (sel == 2)
		return gd->bus_clk;
	else if (sel == 3)
		return (24 * MHZ);
	else
		return 0;
}


/*
 * rkplat get mmc clock rate from gpll
 */
uint32 rkclk_get_mmc_freq_from_gpll(uint32 sdid)
{
	/* set general pll */
	rkclk_set_mmc_clk_src(sdid, 2);

	return rkclk_get_mmc_clk(sdid);
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
		cru_writel(((0x3Ful << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(11));
	else if (1 == sdid) /* sdio0 */
		cru_writel(((0x3Ful << 8) << 16) | ((div - 1) << 8), CRU_CLKSELS_CON(11));
	else if (2 == sdid) /* emmc */
		cru_writel(((0x3Ful << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(12));
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
	 * rkplat set mmc clock src
	 * 0: arm pll; 1: ddr pll; 2: general pll; 3: 24M
	 */
	src_freqs[0] = 0;
	src_freqs[1] = 0;
	src_freqs[2] = gd->bus_clk / 2;
	src_freqs[3] = (24 * MHZ) / 2;

	if (freq <= (12 * MHZ)) {
		clksel = 3; /* select 24 MHZ */
		src_div = (src_freqs[3] + freq - 1) / freq;
		if (((src_div & 0x1) == 1) && (src_div != 1))
			src_div++;
	} else {
		uint32 i, div, clk_freq, pre_clk_freq = 0;
		/*select best src clock*/
		for (i = 0; i < 3; i++) {
			if (0 == src_freqs[i])
				continue;

			div = (src_freqs[i] + freq - 1) / freq;
			if (((div & 0x1) == 1) && (div != 1))
				div++;
			clk_freq = src_freqs[i]/div;
			if (clk_freq > pre_clk_freq) {
				pre_clk_freq = clk_freq;
				clksel = i;
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

	return src_freqs[clksel] / src_div;
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
	return gd->arch.pclk_periph_rate_hz;
}


/*
 * rkplat get I2C clock, from pclk_periph
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id)
{
	return gd->arch.pclk_periph_rate_hz;
}


/*
 * rkplat get spi clock, spi0 can be from arm pll/ddr pll/general pll
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_spi_clk(uint32 spi_bus)
{
	uint32 con;
	uint32 sel;
	uint32 div;

	con =  cru_readl(CRU_CLKSELS_CON(25));
	sel = (con >> 8) & 0x3;
	div = (con & 0x7F) + 1;

	/* rk3036 sd clk pll can be from arm pll/ddr pll/general pll, defualt general pll */
	if (sel == 0)
		return gd->cpu_clk / div;
	else if (sel == 1)
		return gd->mem_clk / div;
	else if (sel == 2)
		return gd->bus_clk / div;
	else
		return 0;
}


/*
 * cpu soft reset
 */
void rkcru_cpu_soft_reset(void)
{
	/* pll enter slow mode */
	writel(PLL_MODE_SLOW(APLL_ID) | PLL_MODE_SLOW(GPLL_ID), RKIO_GRF_PHYS + CRU_MODE_CON);

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
