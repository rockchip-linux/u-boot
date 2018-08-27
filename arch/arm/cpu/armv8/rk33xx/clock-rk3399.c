/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <div64.h>
#include <asm/arch/rkplat.h>


#define pmucru_readl(offset)		readl(RKIO_PMU_CRU_PHYS + offset)
#define pmucru_writel(v, offset)	do { writel(v, RKIO_PMU_CRU_PHYS + offset); } while (0)


/* ARM/General/Codec pll freq config */
#define RKCLK_APLLL_FREQ_HZ		816000000
#define RKCLK_GPLL_FREQ_HZ		800000000
#define RKCLK_CPLL_FREQ_HZ		800000000

#define RKCLK_PPLL_FREQ_HZ		700000000

/* ahp periph_h */
#define ACLK_PERIHP_HZ			150000000
#define HCLK_PERIHP_HZ			75000000
#define PCLK_PERIHP_HZ			37500000

/* ahp periph_l 0 */
#define ACLK_PERILP0_HZ			300000000
#define HCLK_PERILP0_HZ			100000000
#define PCLK_PERILP0_HZ			50000000

/* ahp periph_l 1 */
#define HCLK_PERILP1_HZ			100000000
#define PCLK_PERILP1_HZ			50000000

/* pclk_pmu */
#define PCLK_PMU_HZ			50000000

/* Cpu clock source select */
#define CPU_SRC_ARM_PLLL		0
#define CPU_SRC_ARM_PLLB		1
#define CPU_SRC_DDR_PLL			2
#define CPU_SRC_GENERAL_PLL		3

/* Periph_h clock source select */
#define PERIPH_H_SRC_GENERAL_PLL	1
#define PERIPH_H_SRC_CODEC_PLL		0

/* Periph_l clock source select */
#define PERIPH_L_SRC_GENERAL_PLL	1
#define PERIPH_L_SRC_CODEC_PLL		0

struct pll_clk_set {
	unsigned long	rate;
	u32	pllcon0;
	u32	pllcon1;
	u32	pllcon2;
	u32	pllcon3;
	u32	rst_dly; /* us */

	u8	arm_core_div;
	u8	axi_core_div;
	u8	atclk_core_div;
	u8	pclk_dbg_div;
};


#define _APLL_SET_CLKS(_hz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac, \
	_arm_core_div, _aclkm_core_div, _atclk_core_div, _pclk_dbg_div) \
{ \
	.rate	= _hz,	\
	.pllcon0 = PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
	.pllcon2 = PLL_SET_FRAC(_frac),	\
	.pllcon3 = PLL_SET_DSMPD(_dsmpd), \
        .arm_core_div = CLK_DIV_##_arm_core_div, \
	.axi_core_div = CLK_DIV_##_aclkm_core_div, \
	.atclk_core_div = CLK_DIV_##_atclk_core_div, \
	.pclk_dbg_div = CLK_DIV_##_pclk_dbg_div, \
	.rst_dly = 0, \
}


#define _PLL_SET_CLKS(_hz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac) \
{ \
	.rate	= _hz, \
	.pllcon0 = PLL_SET_FBDIV(_fbdiv),	\
	.pllcon1 = PLL_SET_POSTDIV1(_postdiv1) | PLL_SET_POSTDIV2(_postdiv2) | PLL_SET_REFDIV(_refdiv), \
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


/* 		rk3399 pll notice 		*/
static struct pll_clk_set apll_clks[] = {
	/*
	 * _hz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac,
	 *	_arm_core_div, _aclkm_core_div, _atclk_core_div, _pclk_dbg_div
	 */
	_APLL_SET_CLKS(816000000, 1, 68, 2, 1, 1, 0,	1, 4, 6, 6),
	_APLL_SET_CLKS(600000000, 1, 75, 3, 1, 1, 0,	1, 2, 2, 2),
};


static struct pll_clk_set gpll_clks[] = {
	/*
	 * _hz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac
	 */
	_PLL_SET_CLKS(800000000, 6, 400, 2, 1, 1, 0),
	_PLL_SET_CLKS(594000000, 2, 99, 2, 1, 1, 0),
};


/* cpll clock table, should be from high to low */
static struct pll_clk_set cpll_clks[] = {
	/* _hz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	_PLL_SET_CLKS(800000000, 6, 400, 2, 1, 1, 0),
};


/* vpll clock table, should be from high to low */
static struct pll_clk_set vpll_clks[] = {
	/* _hz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	_PLL_SET_CLKS( 594000000, 1, 123, 5, 1, 0, 12582912),	/* vco = 2970000000 */
	_PLL_SET_CLKS( 593406593, 1, 123, 5, 1, 0, 10508804), 	/* vco = 2967032965 */
	_PLL_SET_CLKS( 297000000, 1, 123, 5, 2, 0, 12582912),	/* vco = 2970000000 */
	_PLL_SET_CLKS( 296703297, 1, 123, 5, 2, 0, 10508807),	/* vco = 2967032970 */
	_PLL_SET_CLKS( 148500000, 1, 129, 7, 3, 0, 15728640),	/* vco = 3118500000 */
	_PLL_SET_CLKS( 148351648, 1, 123, 5, 4, 0, 10508800),	/* vco = 2967032960 */
	_PLL_SET_CLKS(  74250000, 1, 129, 7, 6, 0, 15728640),	/* vco = 3118500000 */
	_PLL_SET_CLKS(  74175824, 1, 129, 7, 6, 0, 13550823),	/* vco = 3115384608 */
	_PLL_SET_CLKS(  65000000, 1, 113, 7, 6, 0, 12582912),	/* vco = 2730000000 */
	_PLL_SET_CLKS(  59340659, 1, 121, 7, 7, 0, 2581098),	/* vco = 2907692291 */
	_PLL_SET_CLKS(  54000000, 1, 110, 7, 7, 0, 4194304),	/* vco = 2646000000 */
	_PLL_SET_CLKS(  27000000, 1, 55, 7, 7, 0, 2097152),	/* vco = 1323000000 */
	_PLL_SET_CLKS(  26973027, 1, 55, 7, 7, 0, 1173232),	/* vco = 1321678323 */
};


/* ppll clock table, should be from high to low */
static struct pll_clk_set ppll_clks[] = {
	/* _hz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	_PLL_SET_CLKS(676000000, 3,  169, 2, 1, 1, 0),
};

static struct pll_data rkpll_data[END_PLL_ID] = {
	SET_PLL_DATA(APLLL_ID, apll_clks, ARRAY_SIZE(apll_clks)),
	SET_PLL_DATA(APLLB_ID, NULL, 0),
	SET_PLL_DATA(DPLL_ID, NULL, 0),
	SET_PLL_DATA(CPLL_ID, cpll_clks, ARRAY_SIZE(cpll_clks)),
	SET_PLL_DATA(GPLL_ID, gpll_clks, ARRAY_SIZE(gpll_clks)),
	SET_PLL_DATA(NPLL_ID, NULL, 0),
	SET_PLL_DATA(VPLL_ID, vpll_clks, ARRAY_SIZE(vpll_clks)),

	SET_PLL_DATA(PPLL_ID, ppll_clks, ARRAY_SIZE(ppll_clks)),
};


/* Waiting for pll locked by pll id */
static void rkclk_pll_wait_lock(enum rk_plls_id pll_id)
{
	volatile unsigned int val;

	/* delay for pll lock */
	if (pll_id == PPLL_ID) {
		do {
			val = pmucru_readl(PMUCRU_PLL_CON(0, 2)) & (1 << PLL_LOCK_SHIFT);
			clk_loop_delayus(1);
		} while (!val);
	} else {
		do {
			val = cru_readl(CRU_PLL_CON(pll_id, 2)) & (1 << PLL_LOCK_SHIFT);
			clk_loop_delayus(1);
		} while (!val);
	}
}


/* Set pll mode by id, normal mode or slow mode */
static void rkclk_pll_set_mode(enum rk_plls_id pll_id, int pll_mode)
{
	if (pll_mode == RKCLK_PLL_MODE_NORMAL) {
		/* PLL enter normal-mode */
		if (pll_id == PPLL_ID)
			pmucru_writel(PLL_MODE_NORM | PLL_MODE_W_MSK, PMUCRU_PLL_CON(0, 3));
		else
			cru_writel(PLL_MODE_NORM | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));
	} else if (pll_mode == RKCLK_PLL_MODE_SLOW) {
		/* PLL enter slow-mode */
		if (pll_id == PPLL_ID)
			pmucru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, PMUCRU_PLL_CON(0, 3));
		else
			cru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));
	} else {
		/* PLL enter deep slow-mode */
		if (pll_id == PPLL_ID)
			pmucru_writel(PLL_MODE_DEEP_SLOW | PLL_MODE_W_MSK, PMUCRU_PLL_CON(0, 3));
		else
			cru_writel(PLL_MODE_DEEP_SLOW | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));
	}
}


/* Set pll rate by id */
static int rkclk_pll_set_rate(enum rk_plls_id pll_id, uint32 Hz, pll_callback_f cb_f)
{
	const struct pll_data *pll = NULL;
	struct pll_clk_set *clkset = NULL;
	unsigned long rate = Hz;
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

	if (pll_id == PPLL_ID) {
		/* PLL enter slow-mode */
		pmucru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, PMUCRU_PLL_CON(0, 3));

		/* pll config */
		pmucru_writel(clkset->pllcon0, PMUCRU_PLL_CON(0, 0));
		pmucru_writel(clkset->pllcon1, PMUCRU_PLL_CON(0, 1));
		pmucru_writel(clkset->pllcon2, PMUCRU_PLL_CON(0, 2));
		pmucru_writel(clkset->pllcon3, PMUCRU_PLL_CON(0, 3));

		/* delay for pll setup */
		rkclk_pll_wait_lock(pll_id);

		if (cb_f != NULL)
			cb_f(clkset);

		/* PLL enter normal-mode */
		pmucru_writel(PLL_MODE_NORM | PLL_MODE_W_MSK, PMUCRU_PLL_CON(0, 3));
	} else {
		/* PLL enter slow-mode */
		cru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, CRU_PLL_CON(pll_id, 3));

		/* pll config */
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
	}

	return 0;
}


/* Get pll rate by id */
#define FRAC_MODE	0
static uint32 rkclk_pll_get_rate(enum rk_plls_id pll_id)
{
	uint32 con, pll_con0, pll_con1, pll_con2, dsmp;

	if (pll_id == PPLL_ID) {
		con = pmucru_readl(PMUCRU_PLL_CON(0, 3)) & PLL_MODE_MSK;
		pll_con0 = pmucru_readl(PMUCRU_PLL_CON(0, 0));
		pll_con1 = pmucru_readl(PMUCRU_PLL_CON(0, 1));
		pll_con2 = pmucru_readl(PMUCRU_PLL_CON(0, 2));
		dsmp = PLL_GET_DSMPD(pmucru_readl(PMUCRU_PLL_CON(0, 3)));
	} else {
		con = cru_readl(CRU_PLL_CON(pll_id, 3)) & PLL_MODE_MSK;
		pll_con0 = cru_readl(CRU_PLL_CON(pll_id, 0));
		pll_con1 = cru_readl(CRU_PLL_CON(pll_id, 1));
		pll_con2 = cru_readl(CRU_PLL_CON(pll_id, 2));
		dsmp = PLL_GET_DSMPD(cru_readl(CRU_PLL_CON(pll_id, 3)));
	}

	if (con == PLL_MODE_SLOW) { /* slow mode */
		return 24 * MHZ;
	} else if (con == PLL_MODE_NORM) { /* normal mode */
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
		do_div(rate64, PLL_GET_POSTDIV1(pll_con1));
		do_div(rate64, PLL_GET_POSTDIV2(pll_con1));

		return rate64;
	} else { /* deep slow mode */
		return 32768;
	}
}


#define MIN_FOUTVCO_FREQ	(800 * MHZ)
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


static int rkclk_cal_postdiv(uint32_t fout_hz, uint32_t *postdiv1, uint32_t *postdiv2, uint32_t *foutvco)
{
	unsigned long freq;

	if (fout_hz < MIN_FOUTVCO_FREQ) {
		for (*postdiv1 = 1; *postdiv1 <= 7; (*postdiv1)++)
			for (*postdiv2 = 1; *postdiv2 <= 7; (*postdiv2)++) {
				freq = fout_hz * (*postdiv1) * (*postdiv2);
				if ((freq >= MIN_FOUTVCO_FREQ) && (freq <= MAX_FOUTVCO_FREQ)) {
					*foutvco = freq;
					return 0;
				}
			}
		printf("CANNOT FINE postdiv1/2 to make fout in range from 400M to 1600M,fout = %u\n", fout_hz);
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
static int rkclk_cal_pll_set(uint32_t fin_hz, uint32_t fout_hz, struct rk_pll_set *pll_set)
{
	uint32_t gcd, foutvco = fout_hz;
	uint64_t fin_64, frac_64;
	uint32_t frac, postdiv1, postdiv2;

	rkclk_cal_postdiv(fout_hz, &postdiv1, &postdiv2, &foutvco);
	pll_set->postdiv1 = postdiv1;
	pll_set->postdiv2 = postdiv2;

	if ((fin_hz / MHZ * MHZ == fin_hz) && (fout_hz / MHZ * MHZ == fout_hz)) {
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
		debug("******frac get refdiv=%u, fbdiv=%u\n", pll_set->refdiv, pll_set->fbdiv);

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
			/* pll con set */
			pllcon0 = PLL_SET_FBDIV(pll_set.fbdiv);
			pllcon1 = PLL_SET_POSTDIV1(pll_set.postdiv1) | PLL_SET_POSTDIV2(pll_set.postdiv2) | PLL_SET_REFDIV(pll_set.refdiv);
			pllcon2 = PLL_SET_FRAC(pll_set.frac);
			pllcon3 = PLL_SET_DSMPD(pll_set.dsmpd);
		} else { /* calcurate error. */
			return -1;
		}
	}

	if (pll_id == PPLL_ID) {
		/* PLL enter slow-mode */
		pmucru_writel(PLL_MODE_SLOW | PLL_MODE_W_MSK, PMUCRU_PLL_CON(0, 3));

		/* pll config */
		pmucru_writel(pllcon0, PMUCRU_PLL_CON(0, 0));
		pmucru_writel(pllcon1, PMUCRU_PLL_CON(0, 1));
		pmucru_writel(pllcon2, PMUCRU_PLL_CON(0, 2));
		pmucru_writel(pllcon3, PMUCRU_PLL_CON(0, 3));

		/* delay for pll setup */
		rkclk_pll_wait_lock(pll_id);

		/* PLL enter normal-mode */
		pmucru_writel(PLL_MODE_NORM | PLL_MODE_W_MSK, PMUCRU_PLL_CON(0, 3));
	} else {
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
	}

	/* update struct gd */
	rkclk_get_pll();

	return 0;
}

/*
 * rkplat clock set periph_h bus clock from codec pll or general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_perihp_ahpclk_set(uint32 pll_src)
{
	uint32 pll_sel = 0, clk_parent_hz;
	uint32 a_div = 0, h_div = 0, p_div = 0;

	/* periph_h bus clock source select: 0: codec pll, 1: general pll */
	if (pll_src == PERIPH_H_SRC_CODEC_PLL) {
		pll_sel = PERIPH_SEL_CPLL;
		clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	} else {
		pll_sel = PERIPH_SEL_GPLL;
		clk_parent_hz = RKCLK_GPLL_FREQ_HZ;
	}

	/* periph_h bus aclk - aclk = clk_src / (aclk_div_con + 1) */
	a_div = rkclk_calc_clkdiv(clk_parent_hz, ACLK_PERIHP_HZ, 0);
	a_div = a_div ? (a_div - 1) : 0;

	/* periph_h bus hclk -  hclk = clk_src / (hclk_div_con + 1) */
	h_div = rkclk_calc_clkdiv(ACLK_PERIHP_HZ, HCLK_PERIHP_HZ, 0);
	h_div = h_div ? (h_div - 1) : 0;

	/* periph_h bus pclk - pclk = clk_src / (pclk_div_con + 1) */
	p_div = rkclk_calc_clkdiv(ACLK_PERIHP_HZ, PCLK_PERIHP_HZ, 0);
	p_div = p_div ? (p_div - 1) : 0;

	cru_writel((PERIPH_SEL_PLL_W_MSK | pll_sel)
			| (PERIPH_PCLK_DIV_W_MSK | (p_div << PERIPH_PCLK_DIV_OFF))
			| (PERIPH_HCLK_DIV_W_MSK | (h_div << PERIPH_HCLK_DIV_OFF))
			| (PERIPH_ACLK_DIV_W_MSK | (a_div << PERIPH_ACLK_DIV_OFF)), CRU_CLKSELS_CON(14));
}

/*
 * rkplat clock set periph_l clock from general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_perilp_ahpclk_set(uint32 pll_src)
{
	uint32 pll_sel = 0, clk_parent_hz;
	uint32 a_div = 0, h_div = 0, p_div = 0;

	/* periph_l0 bus clock source select: 0: codec pll, 1: general pll */
	if (pll_src == PERIPH_H_SRC_CODEC_PLL) {
		pll_sel = PERIPH_SEL_CPLL;
		clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	} else {
		pll_sel = PERIPH_SEL_GPLL;
		clk_parent_hz = RKCLK_GPLL_FREQ_HZ;
	}

	/* periph_l0 bus aclk - aclk = clk_src / (aclk_div_con + 1) */
	a_div = rkclk_calc_clkdiv(clk_parent_hz, ACLK_PERILP0_HZ, 0);
	a_div = a_div ? (a_div - 1) : 0;

	/* periph_l0 bus hclk -  hclk = aclk / (hclk_div_con + 1) */
	h_div = rkclk_calc_clkdiv(ACLK_PERILP0_HZ, HCLK_PERILP0_HZ, 0);
	h_div = h_div ? (h_div - 1) : 0;

	/* periph_l0 bus pclk - pclk = aclk / (pclk_div_con + 1) */
	p_div = rkclk_calc_clkdiv(ACLK_PERILP0_HZ, PCLK_PERILP0_HZ, 0);
	p_div = p_div ? (p_div - 1) : 0;

	/* perilp_l0 */
	cru_writel((PERIPH_SEL_PLL_W_MSK | pll_sel)
			| (PERIPH_PCLK_DIV_W_MSK | (p_div << PERIPH_PCLK_DIV_OFF))
			| (PERIPH_HCLK_DIV_W_MSK | (h_div << PERIPH_HCLK_DIV_OFF))
			| (PERIPH_ACLK_DIV_W_MSK | (a_div << PERIPH_ACLK_DIV_OFF)), CRU_CLKSELS_CON(23));

	/* periph_l1 bus clock source select: 0: codec pll, 1: general pll */
	if (pll_src == PERIPH_L_SRC_CODEC_PLL) {
		pll_sel = PERI_L1_SEL_CPLL;
		clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	} else {
		pll_sel = PERI_L1_SEL_GPLL;
		clk_parent_hz = RKCLK_GPLL_FREQ_HZ;
	}

	/* periph_l1 bus hclk -  hclk = clk_src / (hclk_div_con + 1) */
	h_div = rkclk_calc_clkdiv(clk_parent_hz, HCLK_PERILP1_HZ, 0);
	h_div = h_div ? (h_div - 1) : 0;

	/* periph_l1 bus pclk - pclk = hclk / (pclk_div_con + 1) */
	p_div = rkclk_calc_clkdiv(HCLK_PERILP1_HZ, PCLK_PERILP1_HZ, 0);
	p_div = p_div ? (p_div - 1) : 0;

	/* perilp_l1 */
	cru_writel((PERI_L1_SEL_PLL_W_MSK | pll_sel)
			| (PERI_L1_PCLK_DIV_W_MSK | (p_div << PERI_L1_PCLK_DIV_OFF))
			| (PERI_L1_HCLK_DIV_W_MSK | (h_div << PERI_L1_HCLK_DIV_OFF)), CRU_CLKSELS_CON(25));
}

static void rkclk_gpll_cb(struct pll_clk_set *clkset)
{
	rkclk_perihp_ahpclk_set(PERIPH_H_SRC_GENERAL_PLL);
	rkclk_perilp_ahpclk_set(PERIPH_L_SRC_GENERAL_PLL);
}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_core_clk_set(uint32 pll_src, uint32 a53_core_div, uint32 aclkm_core_div, uint32 atclk_core_div, uint32 dbg_core_div)
{
	uint32_t pll_sel, a53_div = 0, axi_div = 0, dbg_div = 0, atclk_div = 0;

	/* core clock source select: 0: aplll, 1: apllb, 2: dpll, 3: general pll */
	if (pll_src == CPU_SRC_ARM_PLLL)
		pll_sel = CORE_SEL_APLLL;
	else if (pll_src == CPU_SRC_ARM_PLLB)
		pll_sel = CORE_SEL_APLLB;
	else if (pll_src == CPU_SRC_DDR_PLL)
		pll_sel = CORE_SEL_DPLL;
	else
		pll_sel = CORE_SEL_GPLL;

	/* a53 core clock div: clk_core = clk_src / (div_con + 1) */
	a53_div = a53_core_div ? (a53_core_div - 1) : 0;

	/* aclkm core axi clock div: clk = clk_src / (div_con + 1) */
	axi_div = aclkm_core_div ? (aclkm_core_div - 1) : 0;

	/* pclk dbg core axi clock div: clk = clk_src / (div_con + 1) */
	dbg_div = dbg_core_div ? (dbg_core_div - 1) : 0;

	/* atclk core axi clock div: clk = clk_src / (div_con + 1) */
	atclk_div = atclk_core_div ? (atclk_core_div - 1) : 0;

	cru_writel((CORE_SEL_PLL_W_MSK | pll_sel)
			| (CORE_CLK_DIV_W_MSK | (a53_div << CORE_CLK_DIV_OFF))
			| (CORE_AXI_CLK_DIV_W_MSK | (axi_div << CORE_AXI_CLK_DIV_OFF)), CRU_CLKSELS_CON(0));

	cru_writel((DEBUG_PCLK_DIV_W_MSK | (dbg_div << DEBUG_PCLK_DIV_OFF))
			| (CORE_ATB_DIV_W_MSK | (atclk_div << CORE_ATB_DIV_OFF)), CRU_CLKSELS_CON(1));
}

static void rkclk_aplll_cb(struct pll_clk_set *clkset)
{
	rkclk_core_clk_set(CPU_SRC_ARM_PLLL, clkset->arm_core_div, clkset->axi_core_div, clkset->atclk_core_div, clkset->pclk_dbg_div);
}


static void rkclk_ppll_cb(struct pll_clk_set *clkset)
{
	uint32 p_div = 0;

	/*  pmu pclk - pclk = clk_src / (pclk_div_con + 1) */
	p_div = rkclk_calc_clkdiv(RKCLK_PPLL_FREQ_HZ, PCLK_PMU_HZ, 0);
	p_div = p_div ? (p_div - 1) : 0;

	pmucru_writel((0x1F << (0 + 16)) | (p_div << 0), PMUCRU_CLKSELS_CON(0));
}

static uint32 rkclk_get_periph_h_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(14));
	div = ((con & PERIPH_ACLK_DIV_MSK) >> PERIPH_ACLK_DIV_OFF) + 1;

	return div;
}

static uint32 rkclk_get_periph_h_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(14));
	div = ((con & PERIPH_HCLK_DIV_MSK) >> PERIPH_HCLK_DIV_OFF) + 1;

	return div;
}

static uint32 rkclk_get_periph_h_pclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(14));
	div = ((con & PERIPH_PCLK_DIV_MSK) >> PERIPH_PCLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_periph_l0_aclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(23));
	div = ((con & PERIPH_ACLK_DIV_MSK) >> PERIPH_ACLK_DIV_OFF) + 1;

	return div;
}

static uint32 rkclk_get_periph_l0_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(23));
	div = ((con & PERIPH_HCLK_DIV_MSK) >> PERIPH_HCLK_DIV_OFF) + 1;

	return div;
}

static uint32 rkclk_get_periph_l0_pclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(23));
	div = ((con & PERIPH_PCLK_DIV_MSK) >> PERIPH_PCLK_DIV_OFF) + 1;

	return div;
}


static uint32 rkclk_get_periph_l1_hclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(25));
	div = ((con & PERI_L1_HCLK_DIV_MSK) >> PERI_L1_HCLK_DIV_OFF) + 1;

	return div;
}

static uint32 rkclk_get_periph_l1_pclk_div(void)
{
	uint32 con, div;

	con = cru_readl(CRU_CLKSELS_CON(25));
	div = ((con & PERI_L1_PCLK_DIV_MSK) >> PERI_L1_PCLK_DIV_OFF) + 1;

	return div;
}


static void rkclk_default_init(void)
{
	uint32 div, pll_sel, clk_parent_hz, clk_child_hz;

	/* Fix maskrom cru setting error */
	cru_writel(0xFFFF4101, CRU_CLKSELS_CON(12));
	cru_writel(0xFFFF033F, CRU_CLKSELS_CON(56));
	/* hsicphy clock select USB_480M pll */
	cru_writel(0x00030003, CRU_CLKSELS_CON(19));

	/* aclk_emmc: select cpll and div = 8 */
	pll_sel = 0;
	clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	clk_child_hz = 100000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;

	cru_writel((0x1 << 23) | (0x1F << 16) | (pll_sel << 7) | (div << 0), CRU_CLKSELS_CON(21));

	/* emmc : select cpll and div = 4 */
	pll_sel = 0;
	clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	clk_child_hz = 200000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;
	cru_writel((0x7 << 24) | (0x7F << 16) | (pll_sel << 8) | (div << 0), CRU_CLKSELS_CON(22));

	/* crypto: select cpll and div = 16 */
	pll_sel = 0;
	clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	clk_child_hz = 100000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;
	cru_writel((3 << 22) | (0x1F << 16) | (pll_sel << 6) | (div << 0), CRU_CLKSELS_CON(24));
	cru_writel((3 << 22) | (0x1F << 16) | (pll_sel << 6) | (div << 0), CRU_CLKSELS_CON(26));

	/* spi: select cpll and div */
	pll_sel = 0;
	clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	clk_child_hz = 50000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;

	cru_writel((0x1 << 31) | (0x7F << 24) | (pll_sel << 15) | (div << 8), CRU_CLKSELS_CON(58));
	cru_writel((0x1 << 31) | (0x7F << 24) | (pll_sel << 15) | (div << 8), CRU_CLKSELS_CON(59));
	cru_writel((0x1 << 23) | (0x7F << 16) | (pll_sel << 7) | (div << 0), CRU_CLKSELS_CON(59));
	cru_writel((0x1 << 31) | (0x7F << 24) | (pll_sel << 15) | (div << 8), CRU_CLKSELS_CON(60));
	cru_writel((0x1 << 23) | (0x7F << 16) | (pll_sel << 7) | (div << 0), CRU_CLKSELS_CON(60));

	/* i2c(1-2-3-5-6-7): select cpll and div */
	pll_sel = 0;
	clk_parent_hz = RKCLK_CPLL_FREQ_HZ;
	clk_child_hz = 50000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;
	cru_writel((0x1 << 31) | (0x7F << 24) | (pll_sel << 15) | (div << 8), CRU_CLKSELS_CON(61));
	cru_writel((0x1 << 23) | (0x7F << 16) | (pll_sel << 7) | (div << 0), CRU_CLKSELS_CON(61));
	cru_writel((0x1 << 31) | (0x7F << 24) | (pll_sel << 15) | (div << 8), CRU_CLKSELS_CON(62));
	cru_writel((0x1 << 23) | (0x7F << 16) | (pll_sel << 7) | (div << 0), CRU_CLKSELS_CON(62));
	cru_writel((0x1 << 31) | (0x7F << 24) | (pll_sel << 15) | (div << 8), CRU_CLKSELS_CON(63));
	cru_writel((0x1 << 23) | (0x7F << 16) | (pll_sel << 7) | (div << 0), CRU_CLKSELS_CON(63));

	/* i2c(0-4-8): from ppll and div */
	clk_parent_hz = RKCLK_PPLL_FREQ_HZ;
	clk_child_hz = 50000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;
	pmucru_writel((0x7F << 16) | (div << 0), PMUCRU_CLKSELS_CON(2));
	pmucru_writel((0x7F << 24) | (div << 8), PMUCRU_CLKSELS_CON(2));
	pmucru_writel((0x7F << 16) | (div << 0), PMUCRU_CLKSELS_CON(3));

	/* saradc clock from perilp1 */
	clk_parent_hz = PCLK_PERILP1_HZ;
	clk_child_hz = 1000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;

	cru_writel((0xFF << 24) | (div << 8), CRU_CLKSELS_CON(26));

#ifdef CONFIG_RK_MCU
#ifdef CONFIG_PERILP_MCU
	/* peril m0 clk = 300MHz, select gpll as the source clock */
	clk_parent_hz = RKCLK_GPLL_FREQ_HZ;
	clk_child_hz = 300000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;

	cru_writel((1 << 31) | (0x1F << 24) | (1 << 15) | (div << 8), CRU_CLKSELS_CON(24));
#endif
#ifdef CONFIG_PMU_MCU
	/* pmu m0 clk = 150MHz: select ppll as the source clock */
	clk_parent_hz = RKCLK_PPLL_FREQ_HZ;
	clk_child_hz = 150000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;

	pmucru_writel((1 << 31) | (0x1F << 24) | (0 << 15) | (div << 8), PMUCRU_CLKSELS_CON(0));
#endif
#endif

	/* cci clock from gpll */
	clk_parent_hz = RKCLK_GPLL_FREQ_HZ;
	clk_child_hz = 100000000; /* HZ */

	div = rkclk_calc_clkdiv(clk_parent_hz, clk_child_hz, 1);
	div = div ? (div - 1) : 0;

	cru_writel((0x1f1f << 16) | (div << 0) | (div << 8), CRU_CLKSELS_CON(5));
}


/*
 * rkplat clock set pll mode
 */
void rkclk_pll_mode(int pll_mode)
{
	if (pll_mode == RKCLK_PLL_MODE_NORMAL) {
		rkclk_pll_set_mode(APLLL_ID, pll_mode);
		rkclk_pll_set_mode(APLLB_ID, pll_mode);
		rkclk_pll_set_mode(CPLL_ID, pll_mode);
		rkclk_pll_set_mode(GPLL_ID, pll_mode);
		rkclk_pll_set_mode(NPLL_ID, pll_mode);
		rkclk_pll_set_mode(VPLL_ID, pll_mode);

		rkclk_pll_set_mode(PPLL_ID, pll_mode);
	} else {
		rkclk_pll_set_mode(PPLL_ID, pll_mode);

		rkclk_pll_set_mode(CPLL_ID, pll_mode);
		rkclk_pll_set_mode(GPLL_ID, pll_mode);
		rkclk_pll_set_mode(NPLL_ID, pll_mode);
		rkclk_pll_set_mode(VPLL_ID, pll_mode);
		rkclk_pll_set_mode(APLLB_ID, pll_mode);
		rkclk_pll_set_mode(APLLL_ID, pll_mode);
	}
}


/*
 * rkplat clock set pll rate by id
 */
void rkclk_set_pll_rate_by_id(enum rk_plls_id pll_id, uint32 Hz)
{
	pll_callback_f cb_f = NULL;

	if (APLLL_ID == pll_id)
		cb_f = rkclk_aplll_cb;
	else if (GPLL_ID == pll_id)
		cb_f = rkclk_gpll_cb;
	else
		cb_f = NULL;

	rkclk_pll_set_rate(pll_id, Hz, cb_f);
}


/*
 * rkplat clock set for arm and general pll
 */
void rkclk_set_pll(void)
{
	rkclk_default_init();

	rkclk_pll_set_rate(APLLL_ID, RKCLK_APLLL_FREQ_HZ, rkclk_aplll_cb);
	rkclk_pll_set_rate(GPLL_ID, RKCLK_GPLL_FREQ_HZ, rkclk_gpll_cb);
	rkclk_pll_set_rate(CPLL_ID, RKCLK_CPLL_FREQ_HZ, NULL);
	rkclk_pll_set_rate(PPLL_ID, RKCLK_PPLL_FREQ_HZ, rkclk_ppll_cb);
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
	gd->cpu_clk = rkclk_pll_get_rate(APLLL_ID);
	gd->bus_clk = rkclk_pll_get_rate(GPLL_ID);
	gd->mem_clk = rkclk_pll_get_rate(DPLL_ID);
	gd->pci_clk = rkclk_pll_get_rate(CPLL_ID);

	/* periph_h aclk */
	div = rkclk_get_periph_h_aclk_div();
	gd->arch.aclk_periph_h_rate_hz = gd->bus_clk / div;

	/* periph_h hclk */
	div = rkclk_get_periph_h_hclk_div();
	gd->arch.hclk_periph_h_rate_hz = gd->arch.aclk_periph_h_rate_hz / div;

	/* periph_h pclk */
	div = rkclk_get_periph_h_pclk_div();
	gd->arch.pclk_periph_h_rate_hz = gd->arch.aclk_periph_h_rate_hz / div;

	/* periph_l0 aclk */
	div = rkclk_get_periph_l0_aclk_div();
	gd->arch.aclk_periph_l0_rate_hz = gd->bus_clk / div;

	/* periph_l0 hclk */
	div = rkclk_get_periph_l0_hclk_div();
	gd->arch.hclk_periph_l0_rate_hz = gd->arch.aclk_periph_l0_rate_hz / div;

	/* periph_l0 pclk */
	div = rkclk_get_periph_l0_pclk_div();
	gd->arch.pclk_periph_l0_rate_hz = gd->arch.aclk_periph_l0_rate_hz / div;

	/* periph_l1 hclk */
	div = rkclk_get_periph_l1_hclk_div();
	gd->arch.hclk_periph_l1_rate_hz = gd->bus_clk / div;

	/* periph_l1 pclk */
	div = rkclk_get_periph_l1_pclk_div();
	gd->arch.pclk_periph_l1_rate_hz = gd->arch.hclk_periph_l1_rate_hz / div;
}


/*
 * rkplat clock dump pll information
 */
void rkclk_dump_pll(void)
{
	printf("CPU's clock information:\n");

	printf("    aplll = %ldHZ\n", gd->cpu_clk);

	printf("    apllb = %dHZ\n", rkclk_pll_get_rate(APLLB_ID));

	printf("    gpll = %ldHZ\n", gd->bus_clk);
	printf("               aclk_periph_h = %ldHZ, hclk_periph_h = %ldHZ, pclk_periph_h = %ldHZ\n",
		gd->arch.aclk_periph_h_rate_hz, gd->arch.hclk_periph_h_rate_hz, gd->arch.pclk_periph_h_rate_hz);
	printf("               aclk_periph_l0 = %ldHZ, hclk_periph_l0 = %ldHZ, pclk_periph_l0 = %ldHZ\n",
		gd->arch.aclk_periph_l0_rate_hz, gd->arch.hclk_periph_l0_rate_hz, gd->arch.pclk_periph_l0_rate_hz);
	printf("               hclk_periph_l1 = %ldHZ, pclk_periph_l1 = %ldHZ\n",
		gd->arch.hclk_periph_l1_rate_hz, gd->arch.pclk_periph_l1_rate_hz);

	printf("    cpll = %ldHZ\n", gd->pci_clk);

	printf("    dpll = %ldHZ\n", gd->mem_clk);

	printf("    vpll = %dHZ\n", rkclk_pll_get_rate(VPLL_ID));

	printf("    npll = %dHZ\n", rkclk_pll_get_rate(NPLL_ID));

	printf("    ppll = %dHZ\n", rkclk_pll_get_rate(PPLL_ID));
}


#define VIO_ACLK_MAX	(400 * MHZ)
#define VIO_HCLK_MAX	(100 * MHZ)

/*
 * rkplat lcdc aclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc aclk source pll select) : 0 - vpll, 1 - cpll, 2 - gpll, 3 - npll
 * div (lcdc aclk div from pll) : 0x01 - 0x20
 */
static void rkclk_lcdc_aclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;

	/* aclk div */
	div = (div - 1) & 0x1f;
	con = (0x1f << (0 + 16)) | (div << 0);

	/* aclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (6 + 16)) | (0 << 6);
	else if (pll_sel == 1)
		con |= (3 << (6 + 16)) | (1 << 6);
	else if (pll_sel == 2)
		con |= (3 << (6 + 16)) | (2 << 6);
	else
		con |= (3 << (6 + 16)) | (3 << 6);

	if (lcdc_id == 0)
		cru_writel(con, CRU_CLKSELS_CON(47));
	else
		cru_writel(con, CRU_CLKSELS_CON(48));
}

static int rkclk_lcdc_aclk_set(uint32 lcdc_id, uint32 aclk_hz)
{
	uint32 aclk_info = 0;
	uint32 pll_sel = 0, div = 0;
	uint32 pll_rate = 0;

	if (lcdc_id > 1)
		return -1;

#ifdef CONFIG_RK_VOP_DUAL_ANY_FREQ_PLL
	/* lcdc aclk from gpll */
	pll_sel = 2;
	pll_rate = gd->bus_clk;
#else
	/* lcdc aclk from codec pll */
	pll_sel = 1;
	pll_rate = gd->pci_clk;
#endif

	div = rkclk_calc_clkdiv(pll_rate, aclk_hz, 0);
	aclk_info = (pll_sel << 16) | div;
	debug("rk lcdc aclk config: aclk = %dHZ, pll select = %d, div = %d\n", aclk_hz, pll_sel, div);

	rkclk_lcdc_aclk_config(lcdc_id, pll_sel, div);

	return aclk_info;
}


static void rkclk_lcdc_hclk_config(uint32 lcdc_id, uint32 div)
{
	uint32 con = 0;

	/* hclk div */
	div = (div - 1) & 0x1f;
	con = (0x1f << (8 + 16)) | (div << 8);

	if (lcdc_id == 0)
		cru_writel(con, CRU_CLKSELS_CON(47));
	else
		cru_writel(con, CRU_CLKSELS_CON(48));
}

/*
 * rkplat vio hclk config from aclk vio0 vop1
 * div (lcdc hclk div from aclk) : 0x01 - 0x20
 */
static int rkclk_lcdc_hclk_set(uint32 lcdc_id, uint32 hclk_hz)
{
	uint32 div;

	if (lcdc_id > 1)
		return -1;

	div = rkclk_calc_clkdiv(VIO_ACLK_MAX, VIO_HCLK_MAX, 0);
	debug("rk lcdc hclk config: hclk = %dHZ, div = %d\n", hclk_hz, div);

	rkclk_lcdc_hclk_config(lcdc_id, div);

	return 0;
}


/*
 * rkplat lcdc dclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc dclk source pll select) : 0 - vpll, 1 - cpll, 2 - gpll
 * div (lcdc dclk div from pll) : 0x01 - 0x100
 */
static void rkclk_lcdc_dclk_config(uint32 lcdc_id, uint32 pll_sel, uint32 div)
{
	uint32 con = 0;

	/* dclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (8 + 16)) | (0 << 8);
	else if (pll_sel == 1)
		con |= (3 << (8 + 16)) | (1 << 8);
	else
		con |= (3 << (8 + 16)) | (2 << 8);

	/* dclk div */
	div = (div - 1) & 0xff;
	con |= (0xff << (0 + 16)) | (div << 0);

	if (lcdc_id == 0)
		cru_writel(con, CRU_CLKSELS_CON(49));
	else
		cru_writel(con, CRU_CLKSELS_CON(50));
}


static uint32 rkclk_lcdc_dclk_pll_src(uint32 lcdc_id)
{
	uint32 pll_sel = 0;

	/* lcdc dclk source pll : 0 - vpll, 1 - cpll, 2 - gpll */
	if (lcdc_id == 0)
		pll_sel = (cru_readl(CRU_CLKSELS_CON(49)) >> 8) & 0x3;
	else
		pll_sel = (cru_readl(CRU_CLKSELS_CON(50)) >> 8) & 0x3;

	return pll_sel;
}


static int rkclk_lcdc_dclk_set(uint32 lcdc_id, uint32 dclk_hz)
{
	uint32 dclk_info = 0;
	uint32 pll_sel = 0, pll_rate = 0, div = 0;

	if (lcdc_id > 1)
		return -1;

	pll_sel = rkclk_lcdc_dclk_pll_src(lcdc_id);

	if (pll_sel == 0) {
		/* lcdc dclk from vpll */
		div = 1;
		rkclk_pll_set_any_freq(VPLL_ID, dclk_hz);
	} else if (pll_sel == 1) {
		/* lcdc dclk from cpll */
#ifdef CONFIG_RK_VOP_DUAL_ANY_FREQ_PLL
		div = 1;
		rkclk_pll_set_any_freq(CPLL_ID, dclk_hz);
#else
		pll_rate = gd->pci_clk;
		div = rkclk_calc_clkdiv(pll_rate, dclk_hz, 0);
#endif
	} else {
		/* lcdc dclk from gpll */
		pll_rate = gd->bus_clk;
		div = rkclk_calc_clkdiv(pll_rate, dclk_hz, 0);
	}
	dclk_info = (pll_sel << 16) | div;
	printf("rk lcdc - %d dclk set: dclk = %dHZ, pll select = %d, div = %d\n", lcdc_id, dclk_hz, pll_sel, div);

	rkclk_lcdc_dclk_config(lcdc_id, pll_sel, div);

	return dclk_info;
}


/*
 * rkclk_lcdc_dclk_pll_sel
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc dclk source pll select) : 0 - vpll, 1 - cpll, 2 - gpll
 */
__maybe_unused
int rkclk_lcdc_dclk_pll_sel(uint32 lcdc_id, uint32 pll_sel)
{
	uint32 con = 0;

	if (lcdc_id > 1)
		return -1;

	/* dclk pll source select */
	if (pll_sel == 0)
		con |= (3 << (8 + 16)) | (0 << 8);
	else if (pll_sel == 1)
		con |= (3 << (8 + 16)) | (1 << 8);
	else
		con |= (3 << (8 + 16)) | (2 << 8);

	if (lcdc_id == 0)
		cru_writel(con, CRU_CLKSELS_CON(49));
	else
		cru_writel(con, CRU_CLKSELS_CON(50));

	return 0;
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
	uint32 pll_rate = 0, pll_sel = 0;

	rkclk_lcdc_aclk_set(lcdc_id, VIO_ACLK_MAX);
	rkclk_lcdc_hclk_set(lcdc_id, VIO_HCLK_MAX);
	dclk_info = rkclk_lcdc_dclk_set(lcdc_id, dclk_hz);

	dclk_div = dclk_info & 0x0000FFFF;
	pll_sel = (dclk_info & 0xFFFF0000) >> 16;

	if (pll_sel == 0)
		pll_rate = rkclk_pll_get_rate(VPLL_ID);
	else if (pll_sel == 1)
		pll_rate = rkclk_pll_get_rate(CPLL_ID);
	else
		pll_rate = rkclk_pll_get_rate(GPLL_ID);

	return  pll_rate / dclk_div;
}


uint32 rkclk_get_sdhci_emmc_clk(void)
{
	uint32 con;
	uint32 sel;
	uint32 div;

	con =  cru_readl(CRU_CLKSELS_CON(22));

	sel = (con >> 8) & 0x7;
	div = ((con >> 0) & 0x7F) + 1;

	if (sel == 0)
		return gd->pci_clk / div;
	else if (sel == 1)
		return gd->bus_clk / div;
	else if (sel == 2)
		return rkclk_pll_get_rate(NPLL_ID) / div;
	else if (sel == 3)
		return (480 * MHZ) / div;
	else
		return (24 * MHZ) / div;
}


uint32 rkclk_set_sdhci_emmc_clk(uint32 clock)
{
	uint32 clk_base, pll_sel, div;

	if (clock == 0)
		return 0;

	if (clock <= 24000000) {
		clk_base =  24000000;
		pll_sel = 0x4; /* xin_24m */
	} else {
#ifdef CONFIG_RK_VOP_DUAL_ANY_FREQ_PLL
		clk_base = gd->bus_clk;
		pll_sel = 0x1; /* gpll */
#else
		clk_base = gd->pci_clk;
		pll_sel = 0x0; /* cpll */
#endif
	}

	div = (clk_base + clock - 1) / clock;
	if (div == 0)
		div = 1;
	if (((div & 0x1) == 1) && (div != 1))
		div++;

	if (div > 0x7f)
		div = 0x7f;

	//printf("set_sdhci_clk clock:%d, clk_base:%d, div:%d\n", clock, clk_base, div);

	cru_writel((0x7 << 24) | (0x7F << 16) | (pll_sel << 8) | ((div - 1) << 0), CRU_CLKSELS_CON(22));

	return clk_base / div;
}


/*
 * rk mmc clock source
 * 0: cpll; 1: gpll; 2: npll, 3: ppll, 4: usbphy 480M; 5: 24M
 */
enum {
	MMC_CODEC_PLL = 0,
	MMC_GENERAL_PLL = 1,
	MMC_USBPHY_PLL = 4,
	MMC_24M_PLL = 5,

	MMC_MAX_PLL
};

/* 0: cpll; 1: gpll; 2: npll, 3: ppll, 4: usbphy 480M; 5: 24M */
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
		pll_set = 4;
		break;
	case MMC_24M_PLL:
		pll_set = 5;
		break;
	default:
		pll_set = 5;
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
	else if (pll_set == 4)
		return MMC_USBPHY_PLL;
	else if (pll_set == 5)
		return MMC_24M_PLL;
	else
		return MMC_24M_PLL;
}

static inline uint32 rkclk_mmc_pll_sel2rate(uint32 pll_sel)
{
	if (pll_sel == MMC_CODEC_PLL)
		return gd->pci_clk;
	else if (pll_sel == MMC_GENERAL_PLL)
		return gd->bus_clk;
	else if (pll_sel == MMC_USBPHY_PLL)
		return (480 * MHZ);
	else
		return (24 * MHZ);
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
		return MMC_24M_PLL;
}

/*
 * rkplat set mmc clock source
 * 0: cpll; 1: gpll; 2: npll, 3: ppll, 4: usbphy 480M; 5: 24M
 */
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src)
{
	uint32 set = 0;

	set = rkclk_mmc_pll_sel2set(src);

	if (0 == sdid) /* sdmmc */
		cru_writel((set << 8) | (0x7 << (8 + 16)), CRU_CLKSELS_CON(16));
	else if (1 == sdid) /* sdio0 */
		cru_writel((set << 8) | (0x7 << (8 + 16)), CRU_CLKSELS_CON(15));
}


/*
 * rkplat get mmc clock rate
 */
uint32 rkclk_get_mmc_clk(uint32 sdid)
{
	uint32 con;
	uint32 sel;

	if (0 == sdid) { /* sdmmc */
		con =  cru_readl(CRU_CLKSELS_CON(16));
		sel = rkclk_mmc_pll_set2sel((con >> 8) & 0x7);
	} else if (1 == sdid) { /* sdio0 */
		con =  cru_readl(CRU_CLKSELS_CON(15));
		sel = rkclk_mmc_pll_set2sel((con >> 8) & 0x7);
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
		cru_writel(((0x7Ful << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(16));
	else if (1 == sdid) /* sdio0 */
		cru_writel(((0x7Ful << 0) << 16) | ((div - 1) << 0), CRU_CLKSELS_CON(15));
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

#ifdef	CONFIG_RK_VOP_DUAL_ANY_FREQ_PLL
	src_freqs[0] = 0;
#else
	src_freqs[0] = gd->pci_clk / 2;
#endif
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
	return 0;
}

/*
 * rkplat disable mmc clock tuning
 */
int rkclk_disable_mmc_tuning(uint32 sdid)
{
	return 0;
}


/*
 * rkplat get PWM clock
 * pwm[0~3] from pclk_bus
 * vop0/1_pwm from vpll, cpll, or gpll
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_pwm_clk(uint32 pwm_id)
{
	uint32 con = 0;
	uint32 div = 1;
	uint32 pll_sel = 0;
	uint32 pmu_pll = 0;

	switch (pwm_id) {
	case RK_PWM0:
	case RK_PWM1:
	case RK_PWM2:
	case RK_PWM3:
		/* from pclk_pmu */
		pmu_pll = rkclk_pll_get_rate(PPLL_ID);
		con = pmucru_readl(PMUCRU_CLKSELS_CON(0));
		div = ((con >> 0) & 0x1F) + 1;
		break;
	case RK_VOP0_PWM:
	case RK_VOP1_PWM:
		con = cru_readl(CRU_CLKSELS_CON(pwm_id - RK_VOP0_PWM + 51));
		pll_sel = (con >> 6) & 0x3;
		div = ((con >> 0) & 0x1F) + 1;
		if (0 == pll_sel)
			pmu_pll = rkclk_pll_get_rate(VPLL_ID);
		else if (1 == pll_sel)
			pmu_pll = rkclk_pll_get_rate(CPLL_ID);
		else
			pmu_pll = rkclk_pll_get_rate(GPLL_ID);
		break;
	default:
		break;
	}

	return pmu_pll / div;
}


/*
 * rkplat get I2C clock
 */
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id)
{
	uint32 con = 0;
	uint32 sel = 0;
	uint32 div = 1;

	if ((i2c_bus_id == 0) || (i2c_bus_id == 4) || (i2c_bus_id == 8)) {
		uint32 pmu_pll;

		pmu_pll = rkclk_pll_get_rate(PPLL_ID);
		if (i2c_bus_id == 0) {
			con = pmucru_readl(PMUCRU_CLKSELS_CON(2));
			div = ((con >> 0) & 0x7F) + 1;
		} else if (i2c_bus_id == 4) {
			con = pmucru_readl(PMUCRU_CLKSELS_CON(3));
			div = ((con >> 0) & 0x7F) + 1;
		} else if (i2c_bus_id == 8) {
			con = pmucru_readl(PMUCRU_CLKSELS_CON(2));
			div = ((con >> 8) & 0x7F) + 1;
		}

		return pmu_pll / div;
	} else {
		if (i2c_bus_id == 1) {
			con = cru_readl(CRU_CLKSELS_CON(61));
			sel = (con >> 7) & 0x1;
			div = ((con >> 0) & 0x7F) + 1;
		} else if (i2c_bus_id == 2) {
			con = cru_readl(CRU_CLKSELS_CON(62));
			sel = (con >> 7) & 0x1;
			div = ((con >> 0) & 0x7F) + 1;
		} else if (i2c_bus_id == 3) {
			con = cru_readl(CRU_CLKSELS_CON(63));
			sel = (con >> 7) & 0x1;
			div = ((con >> 0) & 0x7F) + 1;
		} else if (i2c_bus_id == 5) {
			con = cru_readl(CRU_CLKSELS_CON(61));
			sel = (con >> 15) & 0x1;
			div = ((con >> 8) & 0x7F) + 1;
		} else if (i2c_bus_id == 6) {
			con = cru_readl(CRU_CLKSELS_CON(62));
			sel = (con >> 15) & 0x1;
			div = ((con >> 8) & 0x7F) + 1;
		} else if (i2c_bus_id == 7) {
			con = cru_readl(CRU_CLKSELS_CON(63));
			sel = (con >> 15) & 0x1;
			div = ((con >> 8) & 0x7F) + 1;
		}

		/* i2c clk pll can be from codec pll/general pll, default codec pll */
		if (sel == 0)
			return gd->pci_clk / div;
		else
			return gd->bus_clk / div;
	}
}


/*
 * rkplat get spi clock, spi0-2 from  cpll or gpll
 * here no check clkgate, because chip default is enable.
 */
unsigned int rkclk_get_spi_clk(uint32 spi_bus)
{
	uint32 con = 0;
	uint32 sel = 0;
	uint32 div = 1;

	if (spi_bus == 0) {
		con = cru_readl(CRU_CLKSELS_CON(59));
		sel = (con >> 7) & 0x1;
		div = ((con >> 0) & 0x7F) + 1;
	} else if (spi_bus == 1) {
		con = cru_readl(CRU_CLKSELS_CON(59));
		sel = (con >> 15) & 0x1;
		div = ((con >> 8) & 0x7F) + 1;
	} else if (spi_bus == 2) {
		con = cru_readl(CRU_CLKSELS_CON(60));
		sel = (con >> 7) & 0x1;
		div = ((con >> 0) & 0x7F) + 1;
	} else if (spi_bus == 4) {
		con = cru_readl(CRU_CLKSELS_CON(60));
		sel = (con >> 15) & 0x1;
		div = ((con >> 8) & 0x7F) + 1;
	} else if (spi_bus == 5) {
		con = cru_readl(CRU_CLKSELS_CON(58));
		sel = (con >> 15) & 0x1;
		div = ((con >> 8) & 0x7F) + 1;
	} else {
		return 0;
	}

	/* spi clk pll can be from codec pll/general pll, default codec pll */
	if (sel == 0)
		return gd->pci_clk / div;
	else
		return gd->bus_clk / div;
}


#ifdef CONFIG_SECUREBOOT_CRYPTO
/*
 * rkplat set crypto clock, pll_sel: 0: cpll, 1: gpll, 2: ppll
 * here no check clkgate, because chip default is enable.
 */
void rkclk_set_crypto_clk(uint32 rate)
{
	uint32 parent = 0;
	uint32 div = 1;
	uint32 pll_sel = 0;

#ifdef	CONFIG_RK_VOP_DUAL_ANY_FREQ_PLL
	/* parent select gpll */
	pll_sel = 1;
	parent = gd->bus_clk;
#else
	/* parent select codec pll */
	pll_sel = 0;
	parent = gd->pci_clk;
#endif
	div = rkclk_calc_clkdiv(parent, rate, 0);
	div = div ? (div - 1) : 0;

	debug("crypto clk div = %d\n", div);
	cru_writel((3 << 22) | (0x1F << 16) | (pll_sel << 6) | (div << 0),
		   CRU_CLKSELS_CON(24));
	cru_writel((3 << 22) | (0x1F << 16) | (pll_sel << 6) | (div << 0),
		   CRU_CLKSELS_CON(26));
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

	if (sdmmcId == 1) {
		con = (0x01 << 9) | (0x01 << (9 + 16));
	} else {
		con = (0x01 << 10) | (0x01 << (10 + 16));
	}
	cru_writel(con, CRU_SOFTRSTS_CON(7));
	udelay(100);
	if (sdmmcId == 1) {
		con = (0x00 << 9) | (0x01 << (9 + 16));
	} else {
		con = (0x00 << 10) | (0x01 << (10 + 16));
	}
	cru_writel(con, CRU_SOFTRSTS_CON(7));
	udelay(200);
}


/*
 * i2c soft reset
 */
void rkcru_i2c_soft_reset(void)
{
	/* soft reset i2c0 4 8 */
	pmucru_writel(0x7<<12 | 0x7<<(12+16), PMUCRU_SOFTRSTS_CON(1));
	mdelay(1);
	pmucru_writel(0x0<<12 | 0x7<<(12+16), PMUCRU_SOFTRSTS_CON(1));

	/* soft reset i2c1 2 3 5 6 7 */
	writel(0x3f<<9 | 0x3f<<(9+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(14));
	mdelay(1);
	writel(0x00<<9 | 0x3f<<(9+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(14));
}

/*
 * PCIe soft reset
 */
void rkcru_pcie_soft_reset(enum pcie_reset_id id, u32 val)
{
	if (id == PCIE_RESET_PHY) {
		writel((0x1 << 23) | (val << 7),
			RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(8));
	} else if (id == PCIE_RESET_ACLK) {
		writel((0x1 << 16) | (val << 0),
			RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(8));
	} else if (id == PCIE_RESET_PCLK) {
		writel((0x1 << 17) | (val << 1),
			RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(8));
	} else if (id == PCIE_RESET_PM) {
		writel((0x1 << 22) | (val << 6),
			RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(8));
	} else if (id == PCIE_RESET_NOFATAL) {
		if (val)
			val = 0xf;

		writel((0xf << 18) | (val << 2),
			RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(8));
	} else {
		printf("%s: incorrect reset ops\n", __func__);
	}
}
