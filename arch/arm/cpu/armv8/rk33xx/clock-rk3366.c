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


/* ARM/General/Codec pll freq config */
#define CONFIG_RKCLK_APLL_FREQ		816 /* MHZ */
#define CONFIG_RKCLK_GPLL_FREQ		576 /* MHZ */
#define CONFIG_RKCLK_CPLL_FREQ		750 /* MHZ */
#define CONFIG_RKCLK_NPLL_FREQ		594 /* MHZ */

/* Cpu clock source select */
#define CPU_SRC_ARM_PLL			0
#define CPU_SRC_GENERAL_PLL		1
#define CPU_SRC_DDR_PLL			2

/* Periph clock source select */
#define PERIPH_SRC_GENERAL_PLL		1
#define PERIPH_SRC_CODEC_PLL		0

/* Bus clock source select */
#define BUS_SRC_GENERAL_PLL		1
#define BUS_SRC_CODEC_PLL		0


struct pll_clk_set {
	unsigned long	rate;
	u32	pllcon0;
	u32	pllcon1;
	u32	pllcon2;
	u32	pllcon3;
	u32	rst_dly; /* us */

	u8	a53_core_div;
	u8	axi_core_div;
	u8	atclk_core_div;
	u8	dbg_core_div;

	u8	aclk_peri_div;
	u8	hclk_peri_div;
	u8	pclk_peri_div;
	u8	pad2;

	u8	axi_bus_div;
	u8	aclk_bus_div;
	u8	hclk_bus_div;
	u8	pclk_bus_div;
};


#define _APLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, \
	_core_div, _core_aclk_div, _core_atclk_div, _dbg_pclk_div) \
{ \
	.rate	= (_mhz) * KHZ,	\
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2 = PLL_SET_FRAC(_frac),	\
	.pllcon3 = PLL_SET_DSMPD(_dsmpd), \
        .a53_core_div = CLK_DIV_##_core_div, \
	.axi_core_div = CLK_DIV_##_core_aclk_div, \
	.atclk_core_div = CLK_DIV_##_core_atclk_div, \
	.dbg_core_div = CLK_DIV_##_dbg_pclk_div, \
	.rst_dly = 0, \
}


#define _GPLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, \
	_axi_peri_div, _ahb_peri_div, _apb_peri_div, _aclk_bus_div, _ahb_bus_div, _apb_bus_div) \
{ \
	.rate	= (_mhz) * KHZ, \
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2 = PLL_SET_FRAC(_frac),	\
	.pllcon3 = PLL_SET_DSMPD(_dsmpd), \
	.aclk_peri_div	= CLK_DIV_##_axi_peri_div, \
	.hclk_peri_div	= CLK_DIV_##_ahb_peri_div, \
	.pclk_peri_div	= CLK_DIV_##_apb_peri_div, \
	.aclk_bus_div	= CLK_DIV_##_aclk_bus_div, \
	.hclk_bus_div	= CLK_DIV_##_ahb_bus_div, \
	.pclk_bus_div	= CLK_DIV_##_apb_bus_div, \
}


#define _CPLL_SET_CLKS(_mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac) \
{ \
	.rate	= (_mhz) * KHZ, \
	.pllcon0 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2 = PLL_SET_FRAC(_frac),	\
	.pllcon3 = PLL_SET_DSMPD(_dsmpd), \
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


/* 		rk3366 pll notice 		*/
static struct pll_clk_set apll_clks[] = {
	/*
	 * _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,
	 *	_core_div, _core_aclk_civ, _core_atclk_div, _dbg_pclk_div
	 */
	_APLL_SET_CLKS(816000, 1, 68, 2, 1, 1, 0,	1, 2, 3, 3),
	_APLL_SET_CLKS(600000, 1, 75, 3, 1, 1, 0,	1, 2, 2, 2),
};


static struct pll_clk_set gpll_clks[] = {
	/*
	 * _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,
	 *	aclk_peri_div, hclk_peri_div, pclk_peri_div,	aclk_bus_div, hclk_bus_div, pclk_bus_div
	 */
	_GPLL_SET_CLKS(594000, 2, 99, 2, 1, 1, 0,	4, 2, 2,	4, 2, 2),
	_GPLL_SET_CLKS(576000, 1, 96, 4, 1, 1, 0,	4, 2, 2,	4, 2, 2),
	_GPLL_SET_CLKS(297000, 2, 99, 4, 1, 1, 0,	2, 2, 2,	2, 2, 2),
};


/* cpll clock table, should be from high to low */
static struct pll_clk_set cpll_clks[] = {
	/* _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	_CPLL_SET_CLKS(750000, 2, 125, 2, 1, 1, 0),
	_CPLL_SET_CLKS(594000, 2,  99, 2, 1, 1, 0),
	_CPLL_SET_CLKS(400000, 6, 200, 2, 1, 1, 0),
};


/* npll clock table, should be from high to low */
static struct pll_clk_set npll_clks[] = {
	/* _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	_CPLL_SET_CLKS(594000, 2,  99, 2, 1, 1, 0),
};


static struct pll_data rkpll_data[END_PLL_ID] = {
	SET_PLL_DATA(APLL_ID, apll_clks, ARRAY_SIZE(apll_clks)),
	SET_PLL_DATA(DPLL_ID, NULL, 0),
	SET_PLL_DATA(CPLL_ID, cpll_clks, ARRAY_SIZE(cpll_clks)),
	SET_PLL_DATA(GPLL_ID, gpll_clks, ARRAY_SIZE(gpll_clks)),
	SET_PLL_DATA(NPLL_ID, npll_clks, ARRAY_SIZE(npll_clks)),
	SET_PLL_DATA(MPLL_ID, NULL, 0),
	SET_PLL_DATA(WPLL_ID, NULL, 0),
	SET_PLL_DATA(BPLL_ID, NULL, 0),
};


/* Waiting for pll locked by pll id */
static void rkclk_pll_wait_lock(enum rk_plls_id pll_id)
{
	/* delay for pll lock */
	do {
		if (cru_readl(CRU_PLL_CON(pll_id, 2)) & (1 << PLL_LOCK_SHIFT))
			break;
		clk_loop_delayus(1);
	} while (1);
}


/* Set pll mode by id, normal mode or slow mode */
static void rkclk_pll_set_mode(enum rk_plls_id pll_id, int pll_mode)
{
	if (pll_mode == RKCLK_PLL_MODE_NORMAL) {
		/* PLL enter normal-mode */
		cru_writel(PLL_MODE_NORM | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));
	} else if (pll_mode == RKCLK_PLL_MODE_SLOW) {
		/* PLL enter slow-mode */
		cru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));
	} else {
		/* PLL enter deep slow-mode */
		cru_writel(PLL_MODE_DEEP_SLOW | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));
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
	cru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));

	/* enter rest */
	cru_writel(clkset->pllcon0, CRU_PLL_CON(pll_id, 0));
	cru_writel(clkset->pllcon1, CRU_PLL_CON(pll_id, 1));
	cru_writel(clkset->pllcon2, CRU_PLL_CON(pll_id, 2));
	cru_writel(clkset->pllcon3, CRU_PLL_CON(pll_id, 3));

	/* delay for pll setup */
	rkclk_pll_wait_lock(pll_id);
	if (cb_f != NULL)
		cb_f(clkset);

	/* PLL enter normal-mode */
	cru_writel(PLL_MODE_NORM | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));

	return 0;
}


/* Get pll rate by id */
#define FRAC_MODE	0
static uint32 rkclk_pll_get_rate(enum rk_plls_id pll_id)
{
	uint32 con;

	con = cru_readl(CRU_PLL_CON(pll_id, 3)) & PLL_MODE_MSK;
	if (con == PLL_MODE_SLOW) { /* slow mode */
		return 24 * MHZ;
	} else if (con == PLL_MODE_NORM) { /* normal mode */
		u32 pll_con0 = cru_readl(CRU_PLL_CON(pll_id, 0));
		u32 pll_con1 = cru_readl(CRU_PLL_CON(pll_id, 1));
		u32 pll_con2 = cru_readl(CRU_PLL_CON(pll_id, 2));
		u32 dsmp = PLL_GET_DSMPD(cru_readl(CRU_PLL_CON(pll_id, 3)));
		u64 rate64 = 0, frac_rate64 = 0;

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
	} else { /* deep slow mode */
		return 32768;
	}
}

#define MIN_FOUTVCO_FREQ	(1200 * MHZ)
#define MAX_FOUTVCO_FREQ	(2000 * MHZ)


struct rk_pll_set {
	/* fbdiv postdiv1 refdiv postdiv2 dsmpd frac */
	uint32_t fbdiv;
	uint32_t postdiv1;
	uint32_t refdiv;
	uint32_t postdiv2;
	uint32_t dsmpd;
	uint32_t frac;
};

static inline uint32 rkclk_gcd(uint32 numerator, uint32 denominator)
{
	uint32 a, b;

	if (!numerator || !denominator)
		return 0;

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


static int rkclk_cal_postdiv(uint32_t fout_hz, uint32_t *postdiv1,
			     uint32_t *postdiv2, uint32_t *foutvco)
{
	unsigned long freq;

	if (fout_hz < MIN_FOUTVCO_FREQ) {
		for (*postdiv1 = 1; *postdiv1 <= 7; (*postdiv1)++)
			for (*postdiv2 = 1; *postdiv2 <= 7; (*postdiv2)++) {
				freq = fout_hz * (*postdiv1) * (*postdiv2);
				if ((freq >= MIN_FOUTVCO_FREQ) &&
				    (freq <= MAX_FOUTVCO_FREQ)) {
					*foutvco = freq;
				return 0;
			}
		}
		printf("CANNOT FINE postdiv1/2 to make fout in range from 400M to 1600M,fout = %u\n",
		       fout_hz);
	} else {
		*postdiv1 = 1;
		*postdiv2 = 1;
	}

	return 0;
}

/*
 * rkplat rkclk_cal_pll_set
 * fin_hz: parent freq
 * fout_hz: child freq which request
 * refdiv fb postdiv1 postdiv2: pll set
 *
 */
static int rkclk_cal_pll_set(uint32_t fin_hz, uint32_t fout_hz,
			     struct rk_pll_set *pll_set)
{
	uint32_t gcd, foutvco = fout_hz;
	uint64_t fin_64, frac_64;
	uint32_t frac, postdiv1, postdiv2;

	rkclk_cal_postdiv(fout_hz, &postdiv1, &postdiv2, &foutvco);
	pll_set->postdiv1 = postdiv1;
	pll_set->postdiv2 = postdiv2;

	if ((fin_hz / MHZ * MHZ == fin_hz) &&
	    (fout_hz / MHZ * MHZ == fout_hz)) {
		fin_hz /= MHZ;
		foutvco /= MHZ;
		gcd = rkclk_gcd(fin_hz, foutvco);
		pll_set->refdiv = fin_hz / gcd;
		pll_set->fbdiv = foutvco / gcd;

		pll_set->frac = 0;
		pll_set->dsmpd = 1;

		debug("fin=%u, fout=%u, gcd=%u, refdiv=%u, fbdiv=%u, postdiv1=%u, postdiv2=%u, frac=%u\n",
		      fin_hz, fout_hz, gcd, pll_set->refdiv, pll_set->fbdiv,
		pll_set->postdiv1, pll_set->postdiv2, pll_set->frac);
	} else {
		debug("frac div running, fin_hz=%u, fout_hz=%u, fin_INT_mhz=%u, fout_INT_mhz=%u\n",
		      fin_hz, fout_hz, fin_hz / MHZ * MHZ, fout_hz / MHZ * MHZ);
		debug("******frac get postdiv1=%u, postdiv2=%u, foutvco=%u\n",
		      pll_set->postdiv1, pll_set->postdiv2, foutvco);
		gcd = rkclk_gcd(fin_hz / MHZ, foutvco / MHZ);
		pll_set->refdiv = fin_hz / MHZ / gcd;
		pll_set->fbdiv = foutvco / MHZ / gcd;
		debug("******frac get refdiv=%u, fbdiv=%u\n",
		      pll_set->refdiv, pll_set->fbdiv);

		pll_set->frac = 0;
		pll_set->dsmpd = 1;

		frac = (foutvco % MHZ);
		fin_64 = fin_hz;
		do_div(fin_64, (uint64_t)pll_set->refdiv);
		frac_64 = (uint64_t)frac << 24;
		do_div(frac_64, fin_64);
		pll_set->frac = (uint32_t)frac_64;
		if (pll_set->frac > 0)
			pll_set->dsmpd = 0;
		debug("frac=%x\n", pll_set->frac);
	}

	return 0;
}

/*
 * rkplat clock set pll with any frequency
 */
int rkclk_pll_set_any_freq(enum rk_plls_id pll_id, uint32_t pll_hz)
{
	const struct pll_data *pll = NULL;
	const struct pll_clk_set *clkset = NULL;
	struct rk_pll_set pll_set;
	uint32_t pllcon0, pllcon1, pllcon2, pllcon3;
	uint32_t i;

	debug("%s, pll_id[%d], freq=%d\n", __func__, pll_id, pll_hz);

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
		if (pll->clkset[i].rate == pll_hz) {
			clkset = &(pll->clkset[i]);
			break;
		}
	}

	if (clkset != NULL) {
		/* if request pll rate in the pll_clks set */
		pllcon0 = clkset->pllcon0;
		pllcon1 = clkset->pllcon1;
		pllcon2 = clkset->pllcon2;
		pllcon3 = clkset->pllcon3;
	} else {
		/* calcurate the request pll rate set */
		if (rkclk_cal_pll_set(24000000, pll_hz, &pll_set) == 0) {
			debug("*%s*get postdiv1=%u, postdiv2=%u, fbdiv=%u, refdiv= %u\n",
			      __func__, pll_set.postdiv1, pll_set.postdiv2,
			      pll_set.fbdiv, pll_set.refdiv);
			/* pll con set */
			pllcon0 = PLL_SET_FBDIV(pll_set.fbdiv) |
				  PLL_SET_POSTDIV1(pll_set.postdiv1);
			pllcon1 = PLL_SET_POSTDIV2(pll_set.postdiv2) |
				  PLL_SET_REFDIV(pll_set.refdiv);
			pllcon2 = PLL_SET_FRAC(pll_set.frac);
			pllcon3 = PLL_SET_DSMPD(pll_set.dsmpd);
		} else { /* calcurate error. */
			return -1;
		}
	}
	/* PLL enter slow-mode */
	cru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));

	/* pll config */
	cru_writel(pllcon0, CRU_PLL_CON(pll_id, 0));
	cru_writel(pllcon1, CRU_PLL_CON(pll_id, 1));
	cru_writel(pllcon2, CRU_PLL_CON(pll_id, 2));
	cru_writel(pllcon3, CRU_PLL_CON(pll_id, 3));

	/* delay for pll setup */
	rkclk_pll_wait_lock(pll_id);

	/* PLL enter normal-mode */
	cru_writel(PLL_MODE_NORM | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));

	/* update struct gd */
	rkclk_get_pll();

	return 0;
}


/*
 * rkplat clock set bus clock from codec pll or general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_bus_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* pd bus clock source select: 0: codec pll, 1: general pll */
	if (pll_src == BUS_SRC_GENERAL_PLL)
		pll_sel = PDBUS_ACLK_SEL_CPLL;
	else
		pll_sel = PDBUS_ACLK_SEL_GPLL;

	/* pd bus aclk - aclk_pdbus = clk_src / (aclk_div_con + 1) */
	a_div = (aclk_div == 0) ? 1 : (aclk_div - 1);

	/* pd bus hclk -  hclk_pdbus = clk_src / (hclk_div_con + 1) */
	h_div = (hclk_div == 0) ? 1 : (hclk_div - 1);

	/* pd bus pclk - pclk_pdbus = clk_src / (pclk_div_con + 1) */
	p_div = (pclk_div == 0) ? 1 : (pclk_div - 1);

	cru_writel((PDBUS_ACLK_SEL_PLL_W_MSK | pll_sel)
			| (PDBUS_PCLK_DIV_W_MSK | (p_div << PDBUS_PCLK_DIV_OFF))
			| (PDBUS_HCLK_DIV_W_MSK | (h_div << PDBUS_HCLK_DIV_OFF))
			| (PDBUS_ACLK_DIV_W_MSK | (a_div << PDBUS_ACLK_DIV_OFF)), CRU_CLKSELS_CON(8));
}

/*
 * rkplat clock set periph clock from general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_periph_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{
	uint32 pll_sel = 0, a_div = 0, h_div = 0, p_div = 0;

	/* periph clock source select: 0: codec pll, 1: general pll */
	if (pll_src == PERIPH_SRC_CODEC_PLL)
		pll_sel = PERI_SEL_CPLL;
	else
		pll_sel = PERI_SEL_GPLL;

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

	/* periph0 */
	cru_writel((PERI_SEL_PLL_W_MSK | pll_sel)
			| (PERI_PCLK_DIV_W_MSK | (p_div << PERI_PCLK_DIV_OFF))
			| (PERI_HCLK_DIV_W_MSK | (h_div << PERI_HCLK_DIV_OFF))
			| (PERI_ACLK_DIV_W_MSK | (a_div << PERI_ACLK_DIV_OFF)), CRU_CLKSELS_CON(9));

	/* periph1 */
	cru_writel((PERI_SEL_PLL_W_MSK | pll_sel)
			| (PERI_PCLK_DIV_W_MSK | (p_div << PERI_PCLK_DIV_OFF))
			| (PERI_HCLK_DIV_W_MSK | (h_div << PERI_HCLK_DIV_OFF))
			| (PERI_ACLK_DIV_W_MSK | (a_div << PERI_ACLK_DIV_OFF)), CRU_CLKSELS_CON(11));
}

static void rkclk_gpll_cb(struct pll_clk_set *clkset)
{
	rkclk_bus_ahpclk_set(BUS_SRC_GENERAL_PLL, clkset->aclk_bus_div, clkset->hclk_bus_div, clkset->pclk_bus_div);
	rkclk_periph_ahpclk_set(PERIPH_SRC_GENERAL_PLL, clkset->aclk_peri_div, clkset->hclk_peri_div, clkset->pclk_peri_div);
}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_core_clk_set(uint32 pll_src, uint32 a53_core_div, uint32 aclkm_core_div, uint32 dbg_core_div, uint32 atclk_core_div)
{
	uint32_t pll_sel = 0, a53_div = 0, axi_div = 0, dbg_div = 0, atclk_div = 0;

	/* cpu clock source select: 0: arm pll, 1: general pll, 2: ddr pll */
	if (pll_src == CPU_SRC_ARM_PLL)
		pll_sel = CORE_SEL_APLL;
	else if (pll_src == CPU_SRC_GENERAL_PLL)
		pll_sel = CORE_SEL_GPLL;
	else
		pll_sel = CORE_SEL_DPLL;

	/* a53 core clock div: clk_core = clk_src / (div_con + 1) */
	a53_div = (a53_core_div == 0) ? 0 : (a53_core_div - 1);

	/* aclkm core axi clock div: clk = clk_src / (div_con + 1) */
	axi_div = (aclkm_core_div == 0) ? 0 : (aclkm_core_div - 1);

	/* pclk dbg core axi clock div: clk = clk_src / (div_con + 1) */
	dbg_div = (dbg_core_div == 0) ? 0 : (dbg_core_div - 1);

	/* pclk dbg core axi clock div: clk = clk_src / (div_con + 1) */
	atclk_div = (atclk_core_div == 0) ? 0 : (atclk_core_div - 1);

	cru_writel((CORE_SEL_PLL_MSK | pll_sel)
			| (CORE_CLK_DIV_W_MSK | (a53_div << CORE_CLK_DIV_OFF))
			| (CORE_AXI_CLK_DIV_W_MSK | (axi_div << CORE_AXI_CLK_DIV_OFF)), CRU_CLKSELS_CON(0));

	cru_writel((DEBUG_PCLK_DIV_W_MSK | (dbg_div << DEBUG_PCLK_DIV_OFF))
			| (CORE_ATB_DIV_W_MSK | (atclk_div << CORE_ATB_DIV_OFF)), CRU_CLKSELS_CON(1));
}

static void rkclk_apll_cb(struct pll_clk_set *clkset)
{
	rkclk_core_clk_set(CPU_SRC_ARM_PLL, clkset->a53_core_div, clkset->axi_core_div, clkset->dbg_core_div, clkset->atclk_core_div);
}


static uint32 rkclk_get_bus_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(8));
	div = ((con & PDBUS_ACLK_DIV_MSK) >> PDBUS_ACLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_bus_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(8));
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

	con = cru_readl(CRU_CLKSELS_CON(8));
	switch ((con & PDBUS_PCLK_DIV_MSK) >> PDBUS_PCLK_DIV_OFF) {
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
	}

	return div;
}


static uint32 rkclk_get_periph_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(9));
	div = ((con & PERI_ACLK_DIV_MSK) >> PERI_ACLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_periph_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(9));
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

	con = cru_readl(CRU_CLKSELS_CON(9));
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
void rkclk_pll_mode(int pll_mode)
{
	if (pll_mode == RKCLK_PLL_MODE_NORMAL) {
		rkclk_pll_set_mode(APLL_ID, pll_mode);
		rkclk_pll_set_mode(CPLL_ID, pll_mode);
		rkclk_pll_set_mode(GPLL_ID, pll_mode);
		rkclk_pll_set_mode(NPLL_ID, pll_mode);
		rkclk_pll_set_mode(MPLL_ID, pll_mode);
		rkclk_pll_set_mode(WPLL_ID, pll_mode);
		rkclk_pll_set_mode(BPLL_ID, pll_mode);
	} else {
		rkclk_pll_set_mode(CPLL_ID, pll_mode);
		rkclk_pll_set_mode(GPLL_ID, pll_mode);
		rkclk_pll_set_mode(NPLL_ID, pll_mode);
		rkclk_pll_set_mode(MPLL_ID, pll_mode);
		rkclk_pll_set_mode(WPLL_ID, pll_mode);
		rkclk_pll_set_mode(BPLL_ID, pll_mode);
		rkclk_pll_set_mode(APLL_ID, pll_mode);
	}
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
	rkclk_pll_set_rate(CPLL_ID, CONFIG_RKCLK_CPLL_FREQ, NULL);
	rkclk_pll_set_rate(NPLL_ID, CONFIG_RKCLK_NPLL_FREQ, NULL);

	/* SARADC div */
	cru_writel((0x19 << 8) | (0xFF << 24), CRU_CLKSELS_CON(25));
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
#define VIO_HCLK_MAX	(100 * MHZ)

/*
 * rkplat lcdc aclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc aclk source pll select) : 0 - codec pll, 1 - general pll, 2 - usbphy pll
 * div (lcdc aclk div from pll) : 0x01 - 0x20
 */
static int rkclk_lcdc_aclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;

	if (lcdc_id > 0)
		return -1;

	/* lcdc0 register bit offset */
	con = 0;

	/* aclk div */
	div = (div - 1) & 0x1f;
	con |= (0x1f << (0 + 16)) | (div << 0);

	/* aclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (6 + 16)) | (0 << 6);
	else if (pll_sel == 1)
		con |= (3 << (6 + 16)) | (1 << 6);
	else
		con |= (3 << (6 + 16)) | (2 << 6);

	cru_writel(con, CRU_CLKSELS_CON(19));

	return 0;
}

static int rkclk_lcdc_aclk_set(uint32 lcdc_id, uint32 aclk_hz)
{
	uint32 aclk_info = 0;
	uint32 pll_sel = 0, div = 0;
	uint32 pll_rate = 0;

	/* lcdc aclk from codec pll */
	pll_sel = 0;
	pll_rate = gd->pci_clk;

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
 * rkplat lcdc dclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0
 * pll_sel (lcdc dclk source pll select) : 0 - codec pll, 1 - general pll, 2 - new pll, 3 - HDMI pll
 * div (lcdc dclk div from pll) : 0x01 - 0x100
 */
static int rkclk_lcdc_dclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;

	if (lcdc_id > 0)
		return -1;

	con = 0;

	/* dclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (8 + 16)) | (0 << 8);
	else if (pll_sel == 1)
		con |= (3 << (8 + 16)) | (1 << 8);
	else if (pll_sel == 2)
		con |= (3 << (8 + 16)) | (2 << 8);
	else
		con |= (3 << (8 + 16)) | (3 << 8);

	/* dclk div */
	div = (div - 1) & 0xff;
	con |= (0xff << (0 + 16)) | (div << 0);

	cru_writel(con, CRU_CLKSELS_CON(20));

	return 0;
}


static int rkclk_lcdc_dclk_set(uint32 lcdc_id, uint32 dclk_hz)
{
	uint32 dclk_info = 0;
	uint32 pll_sel = 0, div = 0;

	/* lcdc dclk from npll */
	pll_sel = 2;
	if (dclk_hz > 100000000) {
		div = 1;
		rkclk_pll_set_any_freq(NPLL_ID, dclk_hz);
	} else {
		div = 4;
		rkclk_pll_set_any_freq(NPLL_ID, dclk_hz * 4);
	}
	dclk_info = (pll_sel << 16) | div;
	debug("rk lcdc dclk set: dclk = %dHZ, pll select = %d, div = %d\n", dclk_hz, pll_sel, div);

	rkclk_lcdc_dclk_config(lcdc_id, pll_sel, div);

	return dclk_info;
}


/*
 * rkplat lcdc dclk and aclk parent pll source
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * dclk_hz: dclk rate
 * return dclk rate
 */
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz)
{
	uint32 dclk_div;
	uint32 dclk_info = 0;

	rkclk_lcdc_aclk_set(lcdc_id, VIO_ACLK_MAX);
	rkclk_lcdc_hclk_set(lcdc_id, VIO_HCLK_MAX);
	dclk_info = rkclk_lcdc_dclk_set(lcdc_id, dclk_hz);

	dclk_div = dclk_info & 0x0000FFFF;
	return rkclk_pll_get_rate(NPLL_ID) / dclk_div;
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
		con = (0 << 7) | (1 << (7 + 16));
		parent = gd->pci_clk;
	} else {
		con = (1 << 7) | (1 << (7 + 16));
		parent = gd->bus_clk;
	}

	div = rkclk_calc_clkdiv(parent, freq, 0);
	if (div == 0)
		div = 1;
	con |= (((div - 1) << 0) | (0x1f << (0 + 16)));
	cru_writel(con, CRU_CLKSELS_CON(47));

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
		pll_set = 2;
		break;
	case MMC_24M_PLL:
		pll_set = 3;
		break;
	default:
		pll_set = 3;
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
		return MMC_USBPHY_PLL;
	else if (pll_set == 3)
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
		return (480 * MHZ);
	else if (pll_sel == MMC_24M_PLL)
		return (24 * MHZ);
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
		cru_writel((set << 8) | (0x03 << (8 + 16)), CRU_CLKSELS_CON(50));
	else if (1 == sdid) /* sdio0 */
		cru_writel((set << 8) | (0x03 << (8 + 16)), CRU_CLKSELS_CON(48));
	else if (2 == sdid) /* emmc */
		cru_writel((set << 8) | (0x03 << (8 + 16)), CRU_CLKSELS_CON(51));
}


/*
 * rkplat get mmc clock rate
 */
uint32 rkclk_get_mmc_clk(uint32 sdid)
{
	uint32 con;
	uint32 sel;

	if (0 == sdid) { /* sdmmc */
		con =  cru_readl(CRU_CLKSELS_CON(50));
		sel = rkclk_mmc_pll_set2sel((con >> 8) & 0x3);
	} else if (1 == sdid) { /* sdio0 */
		con =  cru_readl(CRU_CLKSELS_CON(48));
		sel = rkclk_mmc_pll_set2sel((con >> 8) & 0x3);
	} else if (2 == sdid) { /* emmc */
		con =  cru_readl(CRU_CLKSELS_CON(51));
		sel = rkclk_mmc_pll_set2sel((con >> 8) & 0x3);
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

	/* emmc automic divide freq to 1/2, so here divide this to 1/2 */
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
		cru_writel(((0x7Ful << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(50));
	else if (1 == sdid) /* sdio0 */
		cru_writel(((0x7Ful << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(48));
	else if (2 == sdid) /* emmc */
		cru_writel(((0x7Ful << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(51));
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

	src_freqs[0] = gd->pci_clk / 2;
	src_freqs[1] = gd->bus_clk / 2;
	src_freqs[2] = (480 * MHZ) / 2;
	src_freqs[3] = (24 * MHZ) / 2;

	if (freq <= (12 * MHZ)) {
		clksel = MMC_24M_PLL; /* select 24 MHZ */
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

	src_div &= 0x7F;    /* Max div is 0x7F */
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
 * rkplat get PWM clock
 * pwm[0~3] from pclk_bus
 * vop0_pwm from xin24m, cpll, gpll, or npll
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_pwm_clk(uint32 pwm_id)
{
	uint32 con = 0;
	uint32 div = 1;
	uint32 pll_sel = 0;
	uint32 pmu_pll = 0;
	uint32 rate = 0;

	switch (pwm_id) {
	case RK_PWM0:
	case RK_PWM1:
	case RK_PWM2:
	case RK_PWM3:
		/* from pclk_pmu */
		rate = gd->arch.pclk_bus_rate_hz;
		break;
	case RK_VOP0_PWM:
		con = cru_readl(CRU_CLKSELS_CON(23));
		pll_sel = (con >> 6) & 0x3;
		div = ((con >> 0) & 0x1F) + 1;
		if (0 == pll_sel)
			pmu_pll = 24 * MHZ;
		else if (1 == pll_sel)
			pmu_pll = rkclk_pll_get_rate(CPLL_ID);
		else if (2 == pll_sel)
			pmu_pll = rkclk_pll_get_rate(GPLL_ID);
		else
			pmu_pll = rkclk_pll_get_rate(NPLL_ID);
		rate = pmu_pll / div;
		break;
	default:
		break;
	}

	return rate;
}

/*
 * rkplat get I2C clock, I2c0 and i2c1 from pclk_cpu, I2c2 and i2c3 from pclk_periph
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id)
{
	if (i2c_bus_id == 0) {
		uint32 con;
		uint32 div;

		/* pclk_pmu from general pll */
		con = cru_readl(CRU_CLKSELS_CON(10));
		div = (con & 0x1F) + 1;

		return gd->bus_clk / div;
	} else if (i2c_bus_id == 1) {
		return gd->arch.pclk_bus_rate_hz;
	} else {
		return gd->arch.pclk_periph_rate_hz;
	}
}


/*
 * rkplat get spi clock, spi0-2 from  cpll or gpll
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_spi_clk(uint32 spi_bus)
{
	uint32 con;
	uint32 sel;
	uint32 div;

	if (spi_bus > 1)
		return 0;

	con = cru_readl(CRU_CLKSELS_CON(45));
	sel = (con >> (7 + 8 * spi_bus)) & 0x1;
	div = ((con >> (0 + 8 * spi_bus)) & 0x7F) + 1;

	/* rk3366 spi clk pll can be from codec pll/general pll, defualt codec pll */
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

	/* parent select general pll */
	parent = gd->pci_clk;
	div = rkclk_calc_clkdiv(parent, rate, 0);
	if (div == 0)
		div = 1;

	debug("crypto clk div = %d\n", div);
	cru_writel((1 << 23) | (0xF << 16) | (1 << 7) | (div - 1), CRU_CLKSELS_CON(6));
}
#endif /* CONFIG_SECUREBOOT_CRYPTO */


/*
 * cpu soft reset
 */
void rkcru_cpu_soft_reset(void)
{
	/* PLL enter slow-mode */
	rkclk_pll_mode(RKCLK_PLL_MODE_SLOW);

	/* soft reset */
	cru_writel(0xfdb9, CRU_GLB_SRST_FST);
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
	writel(0x3f<<10 | 0x3f<<(10+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
	mdelay(1);
	writel(0x00<<10 | 0x3f<<(10+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(2));
}
