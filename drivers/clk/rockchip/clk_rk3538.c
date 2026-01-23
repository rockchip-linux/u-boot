// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 * Author: Elaine Zhang <zhangqing@rock-chips.com>
 */

#include <bitfield.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <syscon.h>
#include <asm/arch-rockchip/cru_rk3538.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/hardware.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dt-bindings/clock/rockchip,rk3538-cru.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

#define DIV_TO_RATE(input_rate, div)    ((input_rate) / ((div) + 1))

#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
#ifndef BITS_WITH_WMASK
#define BITS_WITH_WMASK(bits, msk, shift) \
	((bits) << (shift)) | ((msk) << ((shift) + 16))
#endif
#endif

static struct rockchip_pll_rate_table rk3538_pll_rates[] = {
	/* _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	RK3036_PLL_RATE(1416000000, 1, 59, 1, 1, 1, 0),
	RK3036_PLL_RATE(1296000000, 1, 54, 1, 1, 1, 0),
	RK3036_PLL_RATE(1200000000, 1, 50, 1, 1, 1, 0),
	RK3036_PLL_RATE(1188000000, 1, 99, 2, 1, 1, 0),
	RK3036_PLL_RATE(1092000000, 2, 91, 1, 1, 1, 0),
	RK3036_PLL_RATE(1008000000, 1, 42, 1, 1, 1, 0),
	RK3036_PLL_RATE(1000000000, 1, 125, 3, 1, 1, 0),
	RK3036_PLL_RATE(960000000, 1, 40, 1, 1, 1, 0),
	RK3036_PLL_RATE(912000000, 1, 76, 2, 1, 1, 0),
	RK3036_PLL_RATE(816000000, 1, 68, 2, 1, 1, 0),
	RK3036_PLL_RATE(600000000, 1, 50, 2, 1, 1, 0),
	RK3036_PLL_RATE(594000000, 2, 99, 2, 1, 1, 0),
	{ /* sentinel */ },
};

static struct rockchip_pll_clock rk3538_pll_clks[] = {
	[CPLL] = PLL(pll_rk3328, PLL_CPLL, RK3538_PLL_CON(0),
		     RK3538_PMUCRU_MODE_CON00, 0, 10, 0, rk3538_pll_rates),

	[GPLL] = PLL(pll_rk3328, PLL_GPLL, RK3538_PLL_CON(16),
		     RK3538_PMUCRU_MODE_CON00, 2, 10, 0, rk3538_pll_rates),
};

#ifndef CONFIG_SPL_BUILD
#define RK3538_CLK_DUMP(_id, _name)		\
{						\
	.id = _id,				\
	.name = _name,				\
}

static const struct rk3538_clk_info clks_dump[] = {
	RK3538_CLK_DUMP(PLL_GPLL, "gpll"),
	RK3538_CLK_DUMP(PLL_CPLL, "cpll"),
	RK3538_CLK_DUMP(PCLK_TOP_ROOT,   "pclk_top_root"),
	RK3538_CLK_DUMP(PCLK_BUS_ROOT,  "pclk_bus_root"),
	RK3538_CLK_DUMP(HCLK_BUS_ROOT,  "hclk_bus_root"),
	RK3538_CLK_DUMP(ACLK_BUS_ROOT,  "aclk_bus_root"),
};
#endif

/*
 *
 * rational_best_approximation(31415, 10000,
 *		(1 << 8) - 1, (1 << 5) - 1, &n, &d);
 *
 * you may look at given_numerator as a fixed point number,
 * with the fractional part size described in given_denominator.
 *
 * for theoretical background, see:
 * http://en.wikipedia.org/wiki/Continued_fraction
 */
static void rational_best_approximation(unsigned long given_numerator,
					unsigned long given_denominator,
					unsigned long max_numerator,
					unsigned long max_denominator,
					unsigned long *best_numerator,
					unsigned long *best_denominator)
{
	unsigned long n, d, n0, d0, n1, d1;

	n = given_numerator;
	d = given_denominator;
	n0 = 0;
	d1 = 0;
	n1 = 1;
	d0 = 1;
	for (;;) {
		unsigned long t, a;

		if (n1 > max_numerator || d1 > max_denominator) {
			n1 = n0;
			d1 = d0;
			break;
		}
		if (d == 0)
			break;
		t = d;
		a = n / d;
		d = n % d;
		n = t;
		t = n0 + a * n1;
		n0 = n1;
		n1 = t;
		t = d0 + a * d1;
		d0 = d1;
		d1 = t;
	}
	*best_numerator = n1;
	*best_denominator = d1;
}

static ulong rk3538_bus_get_rate(struct rk3538_clk_priv *priv,
				 ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, con;
	ulong rate = 0;

	switch (clk_id) {
	case PCLK_BUS_ROOT:
	case PCLK_TOP_ROOT:
		rate = priv->cpll_hz / 10;
		break;
	case HCLK_BUS_ROOT:
		rate = priv->gpll_hz / 8;
		break;
	case ACLK_BUS_ROOT:
		con = readl(&cru->clksel_con[33]);
		sel = (con & ACLK_BUS_ROOT_SEL_MASK) >> ACLK_BUS_ROOT_SEL_SHIFT;
		if (sel == ACLK_BUS_ROOT_SEL_300M)
			rate = 300 * MHz;
		else if (sel == ACLK_BUS_ROOT_SEL_250M)
			rate = 250 * MHz;
		else
			rate = 200 * MHz;
		break;
	default:
		return -ENOENT;
	}
	return rate;
}

static ulong rk3538_bus_set_rate(struct rk3538_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel;

	switch (clk_id) {
	case PCLK_BUS_ROOT:
	case PCLK_TOP_ROOT:
		break;
	case HCLK_BUS_ROOT:
		break;
	case ACLK_BUS_ROOT:
		if (rate >= 297 * MHz)
			sel = ACLK_BUS_ROOT_SEL_300M;
		else if (rate >= 250 * MHz)
			sel = ACLK_BUS_ROOT_SEL_250M;
		else
			sel = ACLK_BUS_ROOT_SEL_200M;
		rk_clrsetreg(&cru->clksel_con[33],
			     ACLK_BUS_ROOT_SEL_MASK,
			     sel << ACLK_BUS_ROOT_SEL_SHIFT);
		break;
	default:
		return -ENOENT;
	}

	return rk3538_bus_get_rate(priv, clk_id);
}

static ulong rk3538_i2c_get_clk(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, con;
	ulong rate = 0;

	switch (clk_id) {
	case CLK_I2C0:
		con = readl(&cru->pmuclksel_con[1]);
		sel = (con & CLK_I2C0_SEL_MASK) >> CLK_I2C0_SEL_SHIFT;
		if (sel == CLK_I2C0_SEL_100M)
			rate = 100 * MHz;
		else
			rate = 24 * MHz;
		break;
	case CLK_I2C_BUS_ROOT:
	case CLK_I2C1:
	case CLK_I2C2:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
		con = readl(&cru->clksel_con[33]);
		sel = (con & CLK_I2C_BUS_ROOT_SEL_MASK) >> CLK_I2C_BUS_ROOT_SEL_SHIFT;
		if (sel == CLK_I2C_BUS_ROOT_SEL_200M)
			rate = 198 * MHz;
		else
			rate = 24 * MHz;
		break;

	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rk3538_i2c_set_clk(struct rk3538_clk_priv *priv, ulong clk_id,
				ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel;

	switch (clk_id) {
	case CLK_I2C0:
		if (rate >= 99 * MHz)
			sel = CLK_I2C0_SEL_100M;
		else
			sel = CLK_I2C0_SEL_24M;
		rk_clrsetreg(&cru->pmuclksel_con[1],
			     CLK_I2C0_SEL_MASK,
			     sel << CLK_I2C0_SEL_SHIFT);
		break;
	case CLK_I2C_BUS_ROOT:
	case CLK_I2C1:
	case CLK_I2C2:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
		if (rate >= 198 * MHz)
			sel = CLK_I2C_BUS_ROOT_SEL_200M;
		else
			sel = CLK_I2C_BUS_ROOT_SEL_24M;
		rk_clrsetreg(&cru->clksel_con[33],
			     CLK_I2C_BUS_ROOT_SEL_MASK,
			     sel << CLK_I2C_BUS_ROOT_SEL_SHIFT);
		break;

	default:
		return -ENOENT;
	}

	return rk3538_i2c_get_clk(priv, clk_id);
}

static ulong rk3538_spi_get_clk(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 id, sel, con, mask, shift;
	ulong rate;

	switch (clk_id) {
	case CLK_SPI0:
		id = 32;
		mask = CLK_SPI0_SEL_MASK;
		shift = CLK_SPI0_SEL_SHIFT;
		break;

	case CLK_SPI1:
		id = 33;
		mask = CLK_SPI1_SEL_MASK;
		shift = CLK_SPI1_SEL_SHIFT;
		break;
	default:
		return -ENOENT;
	}

	con = readl(&cru->clksel_con[id]);
	sel = (con & mask) >> shift;
	if (sel == CLK_SPI1_SEL_300M)
		rate = 297 * MHz;
	else if (sel == CLK_SPI1_SEL_200M)
		rate = 198 * MHz;
	else if (sel == CLK_SPI1_SEL_100M)
		rate = 100 * MHz;
	else
		rate = OSC_HZ;

	return rate;
}

static ulong rk3538_spi_set_clk(struct rk3538_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 id, sel, mask, shift;

	if (rate >= 297 * MHz)
		sel = CLK_SPI1_SEL_300M;
	else if (rate >= 198 * MHz)
		sel = CLK_SPI1_SEL_200M;
	else if (rate >= 99 * MHz)
		sel = CLK_SPI1_SEL_100M;
	else
		sel = CLK_SPI1_SEL_24M;

	switch (clk_id) {
	case CLK_SPI0:
		id = 32;
		mask = CLK_SPI0_SEL_MASK;
		shift = CLK_SPI0_SEL_SHIFT;
		break;

	case CLK_SPI1:
		id = 33;
		mask = CLK_SPI1_SEL_MASK;
		shift = CLK_SPI1_SEL_SHIFT;
		break;
	default:
		return -ENOENT;
	}

	rk_clrsetreg(&cru->clksel_con[id], mask, sel << shift);

	return rk3538_spi_get_clk(priv, clk_id);
}

static ulong rk3538_pwm_get_clk(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, con;
	ulong rate;

	switch (clk_id) {
	case CLK_PWM0:
		con = readl(&cru->pmuclksel_con[2]);
		sel = (con & CLK_PWM0_SEL_MASK) >> CLK_PWM0_SEL_SHIFT;
		if (sel == CLK_PWM0_SEL_100M)
			rate = 100 * MHz;
		else
			rate = OSC_HZ;

		break;

	case CLK_PWM1:
		con = readl(&cru->clksel_con[33]);
		sel = (con & CLK_PWM1_SEL_MASK) >> CLK_PWM1_SEL_SHIFT;
		if (sel == CLK_PWM1_SEL_100M)
			rate = 100 * MHz;
		else
			rate = OSC_HZ;
		break;

	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rk3538_pwm_set_clk(struct rk3538_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel;

	switch (clk_id) {
	case CLK_PWM0:
		if (rate == 100 * MHz)
			sel = CLK_PWM0_SEL_100M;
		else
			sel = CLK_PWM0_SEL_24M;
		rk_clrsetreg(&cru->pmuclksel_con[2], CLK_PWM0_SEL_MASK, sel << CLK_PWM0_SEL_SHIFT);
		break;

	case CLK_PWM1:
		if (rate == 100 * MHz)
			sel = CLK_PWM1_SEL_100M;
		else
			sel = CLK_PWM1_SEL_24M;
		rk_clrsetreg(&cru->clksel_con[33], CLK_PWM1_SEL_MASK, sel << CLK_PWM1_SEL_SHIFT);
		break;

	default:
		return -ENOENT;
	}

	return rk3538_pwm_get_clk(priv, clk_id);
}

static ulong rk3538_adc_get_clk(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, div, con;
	ulong rate;

	switch (clk_id) {
	case CLK_SARADC:
		con = readl(&cru->clksel_con[24]);
		sel = (con & CLK_SARADC_SRC_SEL_MASK) >> CLK_SARADC_SRC_SEL_SHIFT;
		div = (con & CLK_SARADC_SRC_DIV_MASK) >>
			CLK_SARADC_SRC_DIV_SHIFT;
		if (sel)
			rate = DIV_TO_RATE(OSC_HZ, div);
		else
			rate = DIV_TO_RATE(198 * MHz, div);
		break;

	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		rate = OSC_HZ;
		break;

	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rk3538_adc_set_clk(struct rk3538_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, div;
	ulong p_rate;

	switch (clk_id) {
	case CLK_SARADC:
		if ((OSC_HZ % rate) == 0) {
			sel = CLK_SARADC_SRC_SEL_24M;
			p_rate = OSC_HZ;
		} else {
			sel = CLK_SARADC_SRC_SEL_200M;
			p_rate = 198 * MHz;
		}
		div = DIV_ROUND_UP(p_rate, rate);
		rk_clrsetreg(&cru->clksel_con[24],
			     CLK_SARADC_SRC_SEL_MASK | CLK_SARADC_SRC_DIV_MASK,
			     ((div - 1) << CLK_SARADC_SRC_DIV_SHIFT) |
			     (sel << CLK_SARADC_SRC_SEL_SHIFT));
		break;

	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		break;

	default:
		return -ENOENT;
	}

	return rk3538_adc_get_clk(priv, clk_id);
}

static ulong rk3538_mmc_get_clk(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 div_mask, div_shift, sel_mask, sel_shift, con, div, sel;
	ulong prate;

	switch (clk_id) {
	case CCLK_SDMMC0:
	case HCLK_SDMMC0:
		con = 26;
		sel_mask = CCLK_SDMMC0_MASK;
		sel_shift = CCLK_SDMMC0_SEL_SHIFT;
		div_mask = CCLK_SDMMC0_DIV_MASK;
		div_shift = CCLK_SDMMC0_DIV_SHIFT;
		break;
	case CCLK_SDMMC1:
	case HCLK_SDMMC1:
		con = 23;
		sel_mask = CCLK_SDMMC1_MASK;
		sel_shift = CCLK_SDMMC1_SEL_SHIFT;
		div_mask = CCLK_SDMMC1_DIV_MASK;
		div_shift = CCLK_SDMMC1_DIV_SHIFT;
		break;
	case CCLK_SDIO:
	case HCLK_SDIO:
		con = 24;
		sel_mask = CCLK_SDIO_MASK;
		sel_shift = CCLK_SDIO_SEL_SHIFT;
		div_mask = CCLK_SDIO_DIV_MASK;
		div_shift = CCLK_SDIO_DIV_SHIFT;
		break;
	case CCLK_EMMC:
	case HCLK_EMMC:
		con = 27;
		sel_mask = CCLK_EMMC_MASK;
		sel_shift = CCLK_EMMC_SEL_SHIFT;
		div_mask = CCLK_EMMC_DIV_MASK;
		div_shift = CCLK_EMMC_DIV_SHIFT;
		break;
	case SCLK_2X_FSPI:
	case HCLK_FSPI:
		con = 28;
		sel_mask = SCLK_2X_FSPI_MASK;
		sel_shift = SCLK_2X_FSPI_SEL_SHIFT;
		div_mask = SCLK_2X_FSPI_DIV_MASK;
		div_shift = SCLK_2X_FSPI_DIV_SHIFT;
		break;

	default:
		return -ENOENT;
	}

	con = readl(&cru->clksel_con[con]);
	div = (con & div_mask) >> div_shift;
	sel = (con & sel_mask) >> sel_shift;

	if (sel == CCLK_SDMMC0_GPLL)
		prate = priv->gpll_hz;
	else if (sel == CCLK_SDMMC0_CPLL)
		prate = priv->cpll_hz;
	else
		prate = OSC_HZ;

	return DIV_TO_RATE(prate, div);
}

static ulong rk3538_mmc_set_clk(struct rk3538_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 div_mask, div_shift, sel_mask, sel_shift, con, div, sel;

	if (OSC_HZ % rate == 0) {
		div = DIV_ROUND_UP(OSC_HZ, rate);
		sel = CCLK_SDMMC0_24M;
	} else if ((priv->cpll_hz % rate) == 0) {
		div = DIV_ROUND_UP(priv->cpll_hz, rate);
		sel = CCLK_SDMMC0_CPLL;
	} else {
		div = DIV_ROUND_UP(priv->gpll_hz, rate);
		sel = CCLK_SDMMC0_GPLL;
	}

	switch (clk_id) {
	case CCLK_SDMMC0:
	case HCLK_SDMMC0:
		con = 26;
		sel_mask = CCLK_SDMMC0_MASK;
		sel_shift = CCLK_SDMMC0_SEL_SHIFT;
		div_mask = CCLK_SDMMC0_DIV_MASK;
		div_shift = CCLK_SDMMC0_DIV_SHIFT;
		break;
	case CCLK_SDMMC1:
	case HCLK_SDMMC1:
		con = 23;
		sel_mask = CCLK_SDMMC1_MASK;
		sel_shift = CCLK_SDMMC1_SEL_SHIFT;
		div_mask = CCLK_SDMMC1_DIV_MASK;
		div_shift = CCLK_SDMMC1_DIV_SHIFT;
		break;
	case CCLK_SDIO:
	case HCLK_SDIO:
		con = 24;
		sel_mask = CCLK_SDIO_MASK;
		sel_shift = CCLK_SDIO_SEL_SHIFT;
		div_mask = CCLK_SDIO_DIV_MASK;
		div_shift = CCLK_SDIO_DIV_SHIFT;
		break;
	case CCLK_EMMC:
	case HCLK_EMMC:
		con = 27;
		sel_mask = CCLK_EMMC_MASK;
		sel_shift = CCLK_EMMC_SEL_SHIFT;
		div_mask = CCLK_EMMC_DIV_MASK;
		div_shift = CCLK_EMMC_DIV_SHIFT;
		break;
	case SCLK_2X_FSPI:
	case HCLK_FSPI:
		con = 28;
		sel_mask = SCLK_2X_FSPI_MASK;
		sel_shift = SCLK_2X_FSPI_SEL_SHIFT;
		div_mask = SCLK_2X_FSPI_DIV_MASK;
		div_shift = SCLK_2X_FSPI_DIV_SHIFT;
		break;

	default:
		return -ENOENT;
	}

	assert(div - 1 <= 255);
	rk_clrsetreg(&cru->clksel_con[con],
		     sel_mask |
		     div_mask,
		     sel << sel_shift |
		     (div - 1) << div_shift);

	return rk3538_mmc_get_clk(priv, clk_id);
}

static ulong rk3538_nandc_get_clk(struct rk3538_clk_priv *priv)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, con, rate;

	con = readl(&cru->clksel_con[33]);
	sel = (con & NCLK_NANDC_SEL_MASK) >>
		NCLK_NANDC_SEL_SHIFT;
	if (sel == NCLK_NANDC_SEL_300M)
		rate = 297 * MHz;
	else if (sel == NCLK_NANDC_SEL_200M)
		rate = 198 * MHz;
	else if (sel == NCLK_NANDC_SEL_150M)
		rate = 148500000;
	else
		rate = 100 * MHz;

	return rate;
}

static ulong rk3538_nandc_set_clk(struct rk3538_clk_priv *priv, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	int sel;

	if (rate >= 297 * MHz)
		sel = NCLK_NANDC_SEL_300M;
	else if (rate >= 198 * MHz)
		sel = NCLK_NANDC_SEL_200M;
	else if (rate >= 148500000)
		sel = NCLK_NANDC_SEL_150M;
	else
		sel = NCLK_NANDC_SEL_100M;

	rk_clrsetreg(&cru->clksel_con[33],
		     NCLK_NANDC_SEL_MASK,
		     sel << NCLK_NANDC_SEL_SHIFT);

	return rk3538_nandc_get_clk(priv);
}

static ulong rk3538_dclk_vop_get_clk(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 con, sel, div;
	ulong rate;

	switch (clk_id) {
	case DCLK_VP0_SRC:
		con = readl(&cru->clksel_con[21]);
		sel = (con & DCLK_VP0_SRC_SEL_MASK) >> DCLK_VP0_SRC_SEL_SHIFT;
		div = (con & DCLK_VP0_SRC_DIV_MASK) >> DCLK_VP0_SRC_DIV_SHIFT;
		if (sel == DCLK_VP0_SRC_SEL_GPLL)
			rate = DIV_TO_RATE(priv->gpll_hz, div);
		else
			rate = DIV_TO_RATE(priv->cpll_hz, div);
		break;

	case DCLK_VP0:
		con = readl(&cru->voclksel_con[0]);
		sel = (con & DCLK_VP0_SEL_MASK) >> DCLK_VP0_SEL_SHIFT;
		if (sel == DCLK_VP0_SEL_SRC)
			rate = rk3538_dclk_vop_get_clk(priv, DCLK_VP0_SRC);
		else
			rate = 0;
		break;

	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rk3538_dclk_vop_set_clk(struct rk3538_clk_priv *priv,
				     ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, div, con;
	ulong prate;

	switch (clk_id) {
	case DCLK_VP0_SRC:
		if ((priv->gpll_hz % rate) == 0) {
			prate = priv->gpll_hz;
			sel = DCLK_VP0_SRC_SEL_GPLL;
		} else {
			prate = priv->cpll_hz;
			sel = DCLK_VP0_SRC_SEL_CPLL;
		}
		div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->clksel_con[21],
			     DCLK_VP0_SRC_SEL_MASK | DCLK_VP0_SRC_DIV_MASK,
			     (sel << DCLK_VP0_SRC_SEL_SHIFT) |
			     ((div - 1) << DCLK_VP0_SRC_DIV_SHIFT));
		break;

	case DCLK_VP0:
		con = readl(&cru->voclksel_con[0]);
		sel = (con & DCLK_VP0_SEL_MASK) >> DCLK_VP0_SEL_SHIFT;
		if (sel == DCLK_VP0_SEL_SRC)
			rk3538_dclk_vop_set_clk(priv, DCLK_VP0_SRC, rate);
		break;

	default:
		return -ENOENT;
	}

	return rk3538_dclk_vop_get_clk(priv, clk_id);
}

static ulong rk3538_uart_frac_get_rate(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 reg, con, fracdiv, p_src;
	unsigned long m, n, p_rate;

	switch (clk_id) {
	case CLK_UART_FRAC_0:
		con = readl(&cru->clksel_con[2]);
		p_src = (con & CLK_UART_FRAC_0_SRC_SEL_MASK) >> CLK_UART_FRAC_0_SRC_SEL_SHIFT;
		reg = 15;
		break;
	case CLK_UART_FRAC_1:
		con = readl(&cru->clksel_con[2]);
		p_src = (con & CLK_UART_FRAC_1_SRC_SEL_MASK) >> CLK_UART_FRAC_1_SRC_SEL_SHIFT;
		reg = 16;
		break;
	case CLK_CM_FRAC_0:
		con = readl(&cru->clksel_con[0]);
		p_src = (con & CLK_CM_FRAC_0_SRC_SEL_MASK) >> CLK_CM_FRAC_0_SRC_SEL_SHIFT;
		reg = 13;
		break;
	case CLK_CM_FRAC_1:
		con = readl(&cru->clksel_con[0]);
		p_src = (con & CLK_CM_FRAC_1_SRC_SEL_MASK) >> CLK_CM_FRAC_1_SRC_SEL_SHIFT;
		reg = 14;
		break;
	case CLK_AUDIO_FRAC_0:
		con = readl(&cru->clksel_con[3]);
		p_src = (con & CLK_AUDIO_FRAC_0_SRC_SEL_MASK) >> CLK_AUDIO_FRAC_0_SRC_SEL_SHIFT;
		reg = 17;
		break;
	case CLK_AUDIO_FRAC_1:
		con = readl(&cru->clksel_con[3]);
		p_src = (con & CLK_AUDIO_FRAC_1_SRC_SEL_MASK) >> CLK_AUDIO_FRAC_1_SRC_SEL_SHIFT;
		reg = 18;
		break;
	default:
		return -ENOENT;
	}

	if (p_src == CLK_CM_FRAC_0_SRC_GPLL)
		p_rate = priv->gpll_hz;
	else if (p_src == CLK_CM_FRAC_0_SRC_CPLL)
		p_rate = priv->cpll_hz;
	else
		p_rate = OSC_HZ;

	fracdiv = readl(&cru->clksel_con[reg]);
	n = fracdiv & CLK_CM_FRAC_NUMERATOR_MASK;
	n >>= CLK_CM_FRAC_NUMERATOR_SHIFT;
	m = fracdiv & CLK_CM_FRAC_DENOMINATOR_MASK;
	m >>= CLK_CM_FRAC_DENOMINATOR_SHIFT;

	return p_rate * n / m;
}

static ulong rk3538_uart_frac_set_rate(struct rk3538_clk_priv *priv,
				       ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 reg, clk_src, p_rate;
	unsigned long m = 0, n = 0, val;

	if (priv->cpll_hz % rate == 0) {
		clk_src = CLK_CM_FRAC_0_SRC_CPLL;
		p_rate = priv->cpll_hz;
	} else if (rate == OSC_HZ) {
		clk_src = CLK_CM_FRAC_0_SRC_SEL_24M;
		p_rate = OSC_HZ;
	} else {
		clk_src = CLK_CM_FRAC_0_SRC_GPLL;
		p_rate = priv->gpll_hz;
	}

	rational_best_approximation(rate, p_rate, GENMASK(16 - 1, 0),
				    GENMASK(16 - 1, 0), &m, &n);

	if (m < 4 && m != 0) {
		if (n % 2 == 0)
			val = 1;
		else
			val = DIV_ROUND_UP(4, m);

		n *= val;
		m *= val;
		if (n > 0xffff)
			n = 0xffff;
	}

	switch (clk_id) {
	case CLK_UART_FRAC_0:
		rk_clrsetreg(&cru->clksel_con[2],
			     CLK_UART_FRAC_0_SRC_SEL_MASK,
			     (clk_src << CLK_UART_FRAC_0_SRC_SEL_SHIFT));
		reg = 15;
		break;
	case CLK_UART_FRAC_1:
		rk_clrsetreg(&cru->clksel_con[2],
			     CLK_UART_FRAC_1_SRC_SEL_MASK,
			     (clk_src << CLK_UART_FRAC_1_SRC_SEL_SHIFT));
		reg = 16;
		break;
	case CLK_CM_FRAC_0:
		rk_clrsetreg(&cru->clksel_con[0],
			     CLK_CM_FRAC_0_SRC_SEL_MASK,
			     (clk_src << CLK_CM_FRAC_0_SRC_SEL_SHIFT));
		reg = 13;
		break;
	case CLK_CM_FRAC_1:
		rk_clrsetreg(&cru->clksel_con[0],
			     CLK_CM_FRAC_1_SRC_SEL_MASK,
			     (clk_src << CLK_CM_FRAC_1_SRC_SEL_SHIFT));
		reg = 14;
		break;
	case CLK_AUDIO_FRAC_0:
		rk_clrsetreg(&cru->clksel_con[3],
			     CLK_AUDIO_FRAC_0_SRC_SEL_MASK,
			     (clk_src << CLK_AUDIO_FRAC_0_SRC_SEL_SHIFT));
		reg = 17;
		break;
	case CLK_AUDIO_FRAC_1:
		rk_clrsetreg(&cru->clksel_con[3],
			     CLK_AUDIO_FRAC_1_SRC_SEL_MASK,
			     (clk_src << CLK_AUDIO_FRAC_1_SRC_SEL_SHIFT));
		reg = 18;
		break;
	default:
		return -ENOENT;
	}

	if (m && n) {
		val = m << CLK_CM_FRAC_NUMERATOR_SHIFT | n;
		writel(val, &cru->clksel_con[reg]);
	}

	return rk3538_uart_frac_get_rate(priv, clk_id);
}

static ulong rk3538_uart_get_rate(struct rk3538_clk_priv *priv, ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, con, div;
	ulong rate;

	switch (clk_id) {
	case SCLK_UART0:
		con = readl(&cru->pmuclksel_con[1]);
		sel = (con & SCLK_UART0_SEL_MASK) >> SCLK_UART0_SEL_SHIFT;
		if (sel == SCLK_UART0_SEL_SRC)
			return rk3538_uart_get_rate(priv, SCLK_UART0_SRC);
		else
			return OSC_HZ;
	case SCLK_UART0_SRC:
		con = readl(&cru->clksel_con[3]);
		sel = (con & SCLK_UART0_SRC_SEL_MASK) >> SCLK_UART0_SRC_SEL_SHIFT;
		div = (con & SCLK_UART0_SRC_DIV_MASK) >> SCLK_UART0_SRC_DIV_SHIFT;
		break;
	case SCLK_UART1:
		con = readl(&cru->clksel_con[3]);
		sel = (con & SCLK_UART1_SEL_MASK) >> SCLK_UART1_SEL_SHIFT;
		con = readl(&cru->clksel_con[4]);
		div = (con & SCLK_UART1_DIV_MASK) >> SCLK_UART1_DIV_SHIFT;
		break;
	case SCLK_UART2:
		con = readl(&cru->clksel_con[4]);
		sel = (con & SCLK_UART2_SEL_MASK) >> SCLK_UART2_SEL_SHIFT;
		div = (con & SCLK_UART2_DIV_MASK) >> SCLK_UART2_DIV_SHIFT;
		break;
	case SCLK_UART3:
		con = readl(&cru->clksel_con[5]);
		sel = (con & SCLK_UART3_SEL_MASK) >> SCLK_UART3_SEL_SHIFT;
		div = (con & SCLK_UART3_DIV_MASK) >> SCLK_UART3_DIV_SHIFT;
		break;
	case SCLK_UART4:
		con = readl(&cru->clksel_con[5]);
		sel = (con & SCLK_UART4_SEL_MASK) >> SCLK_UART4_SEL_SHIFT;
		div = (con & SCLK_UART4_DIV_MASK) >> SCLK_UART4_DIV_SHIFT;
		break;
	case SCLK_UART5:
		con = readl(&cru->clksel_con[6]);
		sel = (con & SCLK_UART5_SEL_MASK) >> SCLK_UART5_SEL_SHIFT;
		div = (con & SCLK_UART5_DIV_MASK) >> SCLK_UART5_DIV_SHIFT;
		break;

	default:
		return -ENOENT;
	}

	if (sel == SCLK_UART0_SRC_SEL_CM_FRAC0)
		rate = DIV_TO_RATE(rk3538_uart_frac_get_rate(priv, CLK_CM_FRAC_0), div);
	else if (sel == SCLK_UART0_SRC_SEL_CM_FRAC1)
		rate = DIV_TO_RATE(rk3538_uart_frac_get_rate(priv, CLK_CM_FRAC_1), div);
	else if (sel == SCLK_UART0_SRC_SEL_UART_FRAC0)
		rate = DIV_TO_RATE(rk3538_uart_frac_get_rate(priv, CLK_UART_FRAC_0), div);
	else if (sel == SCLK_UART0_SRC_SEL_UART_FRAC1)
		rate = DIV_TO_RATE(rk3538_uart_frac_get_rate(priv, CLK_UART_FRAC_1), div);
	else
		rate = DIV_TO_RATE(OSC_HZ, div);

	return rate;
}

static ulong rk3538_uart_set_rate(struct rk3538_clk_priv *priv,
				  ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, div;

	if (rk3538_uart_frac_get_rate(priv, CLK_CM_FRAC_0) % rate == 0) {
		sel = SCLK_UART0_SRC_SEL_CM_FRAC0;
		div = DIV_ROUND_UP(rk3538_uart_frac_get_rate(priv, CLK_CM_FRAC_0), rate);
	} else if (rk3538_uart_frac_get_rate(priv, CLK_CM_FRAC_1) % rate == 0) {
		sel = SCLK_UART0_SRC_SEL_CM_FRAC1;
		div = DIV_ROUND_UP(rk3538_uart_frac_get_rate(priv, CLK_CM_FRAC_1), rate);
	} else if (rk3538_uart_frac_get_rate(priv, CLK_UART_FRAC_0) % rate == 0) {
		sel = SCLK_UART0_SRC_SEL_UART_FRAC0;
		div = DIV_ROUND_UP(rk3538_uart_frac_get_rate(priv, CLK_UART_FRAC_0), rate);
	} else if (rk3538_uart_frac_get_rate(priv, CLK_UART_FRAC_1) % rate == 0) {
		sel = SCLK_UART0_SRC_SEL_UART_FRAC1;
		div = DIV_ROUND_UP(rk3538_uart_frac_get_rate(priv, CLK_UART_FRAC_1), rate);
	} else {
		sel = SCLK_UART0_SRC_SEL_24M;
		div = DIV_ROUND_UP(OSC_HZ, rate);
	}

	switch (clk_id) {
	case SCLK_UART0:
		rk3538_uart_set_rate(priv, SCLK_UART0_SRC, rate);
		rk_clrsetreg(&cru->pmuclksel_con[1],
			     SCLK_UART0_SEL_MASK,
			     SCLK_UART0_SEL_SRC << SCLK_UART0_SEL_SHIFT);
		break;
	case SCLK_UART0_SRC:
		rk_clrsetreg(&cru->clksel_con[3],
			     SCLK_UART0_SRC_SEL_MASK | SCLK_UART0_SRC_DIV_MASK,
			     (sel << SCLK_UART0_SRC_SEL_SHIFT) |
			     ((div - 1) << SCLK_UART0_SRC_DIV_SHIFT));
		break;
	case SCLK_UART1:
		rk_clrsetreg(&cru->clksel_con[3],
			     SCLK_UART1_SEL_MASK,
			     sel << SCLK_UART1_SEL_SHIFT);
		rk_clrsetreg(&cru->clksel_con[4],
			     SCLK_UART1_DIV_MASK,
			     ((div - 1) << SCLK_UART1_DIV_SHIFT));
		break;
	case SCLK_UART2:
		rk_clrsetreg(&cru->clksel_con[4],
			     SCLK_UART2_SEL_MASK | SCLK_UART2_DIV_MASK,
			     (sel << SCLK_UART2_SEL_SHIFT) |
			     ((div - 1) << SCLK_UART2_DIV_SHIFT));
		break;
	case SCLK_UART3:
		rk_clrsetreg(&cru->clksel_con[5],
			     SCLK_UART3_SEL_MASK | SCLK_UART3_DIV_MASK,
			     (sel << SCLK_UART3_SEL_SHIFT) |
			     ((div - 1) << SCLK_UART3_DIV_SHIFT));
		break;
	case SCLK_UART4:
		rk_clrsetreg(&cru->clksel_con[5],
			     SCLK_UART4_SEL_MASK | SCLK_UART4_DIV_MASK,
			     (sel << SCLK_UART4_SEL_SHIFT) |
			     ((div - 1) << SCLK_UART4_DIV_SHIFT));
		break;
	case SCLK_UART5:
		rk_clrsetreg(&cru->clksel_con[6],
			     SCLK_UART5_SEL_MASK | SCLK_UART5_DIV_MASK,
			     (sel << SCLK_UART5_SEL_SHIFT) |
			     ((div - 1) << SCLK_UART5_DIV_SHIFT));
		break;

	default:
		return -ENOENT;
	}

	return rk3538_uart_get_rate(priv, clk_id);
}

static ulong rk3538_mac_get_rate(struct rk3538_clk_priv *priv,
				 ulong clk_id)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, div, con;
	ulong rate = 0;

	switch (clk_id) {
	case CLK_MAC_PTP_REF_SRC:
	case CLK_MAC_PTP_REF:
		con = readl(&cru->clksel_con[22]);
		sel = (con & CLK_MAC_PTP_REF_SRC_MASK) >> CLK_MAC_PTP_REF_SRC_SEL_SHIFT;
		div = (con & CLK_MAC_PTP_REF_SRC_DIV_MASK) >> CLK_MAC_PTP_REF_SRC_DIV_SHIFT;
		if (sel == CLK_MAC_PTP_REF_SRC_CPLL)
			rate = DIV_TO_RATE(priv->cpll_hz, div);
		else
			rate = DIV_TO_RATE(OSC_HZ, div);
		break;
	case ETH0_CLK_25M_OUT:
		con = readl(&cru->pmuclksel_con[0]);
		div = (con & ETH0_CLK_25M_OUT_DIV_MASK) >> ETH0_CLK_25M_OUT_DIV_SHIFT;
		rate = DIV_TO_RATE(priv->cpll_hz, div);
		break;
	default:
		return -ENOENT;
	}
	return rate;
}

static ulong rk3538_mac_set_rate(struct rk3538_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rk3538_cru *cru = priv->cru;
	u32 sel, div, prate;

	switch (clk_id) {
	case CLK_MAC_PTP_REF_SRC:
	case CLK_MAC_PTP_REF:
		if ((OSC_HZ % rate) == 0) {
			prate = OSC_HZ;
			sel = CLK_MAC_PTP_REF_SRC_24M;
		} else {
			prate = priv->cpll_hz;
			sel = CLK_MAC_PTP_REF_SRC_CPLL;
		}
		div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->clksel_con[22],
			     CLK_MAC_PTP_REF_SRC_MASK | CLK_MAC_PTP_REF_SRC_DIV_MASK,
			     (sel << CLK_MAC_PTP_REF_SRC_SEL_SHIFT) |
			     ((div - 1) << CLK_MAC_PTP_REF_SRC_DIV_SHIFT));
		break;
	case ETH0_CLK_25M_OUT:
		div = DIV_ROUND_UP(priv->cpll_hz, rate);
		rk_clrsetreg(&cru->pmuclksel_con[0],
			     ETH0_CLK_25M_OUT_DIV_MASK,
			     ((div - 1) << ETH0_CLK_25M_OUT_DIV_SHIFT));
		break;
	default:
		return -ENOENT;
	}

	return rk3538_mac_get_rate(priv, clk_id);
}

static ulong rk3538_clk_get_rate(struct clk *clk)
{
	struct rk3538_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	if (!priv->gpll_hz || !priv->cpll_hz) {
		printf("%s: gpll=%lu, cpll=%ld\n",
		       __func__, priv->gpll_hz, priv->cpll_hz);
		return -ENOENT;
	}

	switch (clk->id) {
	case PLL_CPLL:
		rate = rockchip_pll_get_rate(&rk3538_pll_clks[CPLL], priv->cru,
					     CPLL);
		break;
	case PLL_GPLL:
		rate = rockchip_pll_get_rate(&rk3538_pll_clks[GPLL], priv->cru,
					     GPLL);
		break;

	case TCLK_WDT:
		rate = OSC_HZ;
		break;
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C2:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
		rate = rk3538_i2c_get_clk(priv, clk->id);
		break;
	case CLK_SPI0:
	case CLK_SPI1:
		rate = rk3538_spi_get_clk(priv, clk->id);
		break;
	case CLK_PWM0:
	case CLK_PWM1:
		rate = rk3538_pwm_get_clk(priv, clk->id);
		break;
	case CLK_SARADC:
	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		rate = rk3538_adc_get_clk(priv, clk->id);
		break;
	case NCLK_NANDC:
		rate = rk3538_nandc_get_clk(priv);
		break;
	case HCLK_SDMMC0:
	case CCLK_SDMMC0:
	case HCLK_SDMMC1:
	case CCLK_SDMMC1:
	case HCLK_SDIO:
	case CCLK_SDIO:
	case HCLK_EMMC:
	case CCLK_EMMC:
	case HCLK_FSPI:
	case SCLK_2X_FSPI:
		rate = rk3538_mmc_get_clk(priv, clk->id);
		break;
	case DCLK_VP0:
	case DCLK_VP0_SRC:
		rate = rk3538_dclk_vop_get_clk(priv, clk->id);
		break;
	case SCLK_UART0:
	case SCLK_UART0_SRC:
	case SCLK_UART1:
	case SCLK_UART2:
	case SCLK_UART3:
	case SCLK_UART4:
	case SCLK_UART5:
		rate = rk3538_uart_get_rate(priv, clk->id);
		break;
	case PCLK_BUS_ROOT:
	case PCLK_TOP_ROOT:
	case HCLK_BUS_ROOT:
	case ACLK_BUS_ROOT:
		rate = rk3538_bus_get_rate(priv, clk->id);
		break;
	case CLK_UART_FRAC_0:
	case CLK_UART_FRAC_1:
	case CLK_CM_FRAC_0:
	case CLK_CM_FRAC_1:
	case CLK_AUDIO_FRAC_0:
	case CLK_AUDIO_FRAC_1:
		rate = rk3538_uart_frac_get_rate(priv, clk->id);
		break;
	case CLK_MAC_PTP_REF_SRC:
	case CLK_MAC_PTP_REF:
	case ETH0_CLK_25M_OUT:
		rate = rk3538_mac_get_rate(priv, clk->id);
		break;
	default:
		return -ENOENT;
	}

	return rate;
};

static ulong rk3538_clk_set_rate(struct clk *clk, ulong rate)
{
	struct rk3538_clk_priv *priv = dev_get_priv(clk->dev);
	ulong ret = 0;

	if (!priv->gpll_hz) {
		printf("%s gpll=%lu\n", __func__, priv->gpll_hz);
		return -ENOENT;
	}

	switch (clk->id) {
	case PLL_CPLL:
		ret = rockchip_pll_set_rate(&rk3538_pll_clks[CPLL], priv->cru,
					    CPLL, rate);
		priv->cpll_hz = rockchip_pll_get_rate(&rk3538_pll_clks[CPLL],
						      priv->cru, CPLL);
		break;
	case PLL_GPLL:
		ret = rockchip_pll_set_rate(&rk3538_pll_clks[GPLL], priv->cru,
					    GPLL, rate);
		priv->gpll_hz = rockchip_pll_get_rate(&rk3538_pll_clks[GPLL],
						      priv->cru, GPLL);
		break;
	case TCLK_WDT:
		return (rate == OSC_HZ) ? 0 : -EINVAL;
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C2:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
		ret = rk3538_i2c_set_clk(priv, clk->id, rate);
		break;
	case CLK_SPI0:
	case CLK_SPI1:
		ret = rk3538_spi_set_clk(priv, clk->id, rate);
		break;
	case CLK_PWM0:
	case CLK_PWM1:
		ret = rk3538_pwm_set_clk(priv, clk->id, rate);
		break;
	case CLK_SARADC:
	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		ret = rk3538_adc_set_clk(priv, clk->id, rate);
		break;
	case NCLK_NANDC:
		ret = rk3538_nandc_set_clk(priv, rate);
		break;
	case HCLK_SDMMC0:
	case CCLK_SDMMC0:
	case HCLK_SDMMC1:
	case CCLK_SDMMC1:
	case HCLK_SDIO:
	case CCLK_SDIO:
	case HCLK_EMMC:
	case CCLK_EMMC:
	case HCLK_FSPI:
	case SCLK_2X_FSPI:
		ret = rk3538_mmc_set_clk(priv, clk->id, rate);
		break;
	case DCLK_VP0:
	case DCLK_VP0_SRC:
		ret = rk3538_dclk_vop_set_clk(priv, clk->id, rate);
		break;
	case SCLK_UART0:
	case SCLK_UART0_SRC:
	case SCLK_UART1:
	case SCLK_UART2:
	case SCLK_UART3:
	case SCLK_UART4:
	case SCLK_UART5:
		ret = rk3538_uart_set_rate(priv, clk->id, rate);
		break;
	case PCLK_BUS_ROOT:
	case PCLK_TOP_ROOT:
	case HCLK_BUS_ROOT:
	case ACLK_BUS_ROOT:
		ret = rk3538_bus_set_rate(priv, clk->id, rate);
		break;
	case CLK_UART_FRAC_0:
	case CLK_UART_FRAC_1:
	case CLK_CM_FRAC_0:
	case CLK_CM_FRAC_1:
	case CLK_AUDIO_FRAC_0:
	case CLK_AUDIO_FRAC_1:
		rate = rk3538_uart_frac_set_rate(priv, clk->id, rate);
		break;
	case CLK_MAC_PTP_REF_SRC:
	case CLK_MAC_PTP_REF:
	case ETH0_CLK_25M_OUT:
		rate = rk3538_mac_set_rate(priv, clk->id, rate);
		break;
	default:
		return -ENOENT;
	}

	return ret;
};

#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
static int rk3538_clk_set_parent(struct clk *clk, struct clk *parent)
{
	struct rk3538_clk_priv *priv = dev_get_priv(clk->dev);
	const char *clock_dev_name = parent->dev->name;

	switch (clk->id) {
	case DCLK_VP0:
		if (!strcmp(clock_dev_name, "innophyo_prepclk"))
			rk_clrsetreg(&priv->cru->voclksel_con[0],
				     DCLK_VP0_SEL_MASK,
				     DCLK_VP0_SEL_INNOPHY << DCLK_VP0_SEL_SHIFT);
		else if (!strcmp(clock_dev_name, "inno_hdmi_pll_clk"))
			rk_clrsetreg(&priv->cru->voclksel_con[0],
				     DCLK_VP0_SEL_MASK,
				     DCLK_VP0_SEL_HDMIPHY << DCLK_VP0_SEL_SHIFT);
		else
			rk_clrsetreg(&priv->cru->voclksel_con[0],
				     DCLK_VP0_SEL_MASK,
				     DCLK_VP0_SEL_SRC << DCLK_VP0_SEL_SHIFT);
		break;

	default:
		return -ENOENT;
	}

	return 0;
}
#endif

static struct clk_ops rk3538_clk_ops = {
	.get_rate = rk3538_clk_get_rate,
	.set_rate = rk3538_clk_set_rate,
#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
	.set_parent = rk3538_clk_set_parent,
#endif
};

static int rk3538_clk_init(struct rk3538_clk_priv *priv)
{
	int ret;

	priv->sync_kernel = false;

	rk_clrsetreg(&priv->cru->clksel_con[19],
		     CLK_CORE_PLL_SEL_MASK | CLK_CORE_PLL_DIV_MASK,
		     (CLK_CORE_PLL_SEL_GPLL << CLK_CORE_PLL_SEL_SHIFT) |
		     (1 << CLK_CORE_PLL_DIV_SHIFT));

	if (priv->cpll_hz != CPLL_HZ) {
		ret = rockchip_pll_set_rate(&rk3538_pll_clks[CPLL], priv->cru,
					    CPLL, CPLL_HZ);
		if (!ret)
			priv->cpll_hz = CPLL_HZ;
	}

	if (priv->gpll_hz != GPLL_HZ) {
		ret = rockchip_pll_set_rate(&rk3538_pll_clks[GPLL], priv->cru,
					    GPLL, GPLL_HZ);
		if (!ret)
			priv->gpll_hz = GPLL_HZ;
	}

	if (!priv->armclk_enter_hz) {
		priv->armclk_enter_hz = DIV_TO_RATE(priv->gpll_hz,
						    (readl(&priv->cru->clksel_con[19]) &
						     CLK_CORE_PLL_DIV_MASK) >>
						    CLK_CORE_PLL_DIV_SHIFT);
		priv->armclk_init_hz = priv->armclk_enter_hz;
	}

	/* enable ETH0_CLK_25M_OUT by default */
	rk_clrsetreg(&priv->cru->pmugate_con[0], 1 << 2, 0 << 2);

	return 0;
}

static int rk3538_clk_probe(struct udevice *dev)
{
	struct rk3538_clk_priv *priv = dev_get_priv(dev);
	int ret;
#if CONFIG_IS_ENABLED(CLK_SCMI)
	struct clk clk;
#endif

#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
	/* set spll to 900M */
	writel(BITS_WITH_WMASK(0U, 0x3U, 0),
	       RK3538_SPMU_CRU_BASE + RK3538_SPMUCRU_MODE_CON00);
	writel(BITS_WITH_WMASK(0x204b, 0x7fffU, 0),
	       RK3538_SPMU_CRU_BASE + RK3538_SPLL_CON(24));
	writel(BITS_WITH_WMASK(0x41, 0x1ffU, 0),
	       RK3538_SPMU_CRU_BASE + RK3538_SPLL_CON(25));
	writel(BITS_WITH_WMASK(1U, 0x3U, 0),
	       RK3538_SPMU_CRU_BASE + RK3538_SPMUCRU_MODE_CON00);
#endif
	ret = rk3538_clk_init(priv);
	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(CLK_SCMI)
#ifndef CONFIG_SPL_BUILD
	ret = rockchip_get_scmi_clk(&clk.dev);
	if (ret) {
		printf("Failed to get scmi clk dev, ret=%d\n", ret);
		return ret;
	}
	if (priv->armclk_enter_hz != CPU_PVTPLL_HZ) {
		clk.id = ARMCLK01;
		ret = clk_set_rate(&clk, CPU_PVTPLL_HZ);
		if (ret < 0) {
			printf("Failed to set cpu01, ret=%d\n", ret);
		} else {
			priv->armclk_enter_hz = CPU_PVTPLL_HZ;
			priv->armclk_init_hz = CPU_PVTPLL_HZ;
		}
		clk.id = ARMCLK23;
		ret = clk_set_rate(&clk, CPU_PVTPLL_HZ);
		if (ret < 0)
			printf("Failed to set cpu23, ret=%d\n", ret);
	}
#endif
#endif

	/* Process 'assigned-{clocks/clock-parents/clock-rates}' properties */
	ret = clk_set_defaults(dev, 1);
	if (ret)
		debug("%s clk_set_defaults failed %d\n", __func__, ret);
	else
		priv->sync_kernel = true;

	return 0;
}

static int rk3538_clk_ofdata_to_platdata(struct udevice *dev)
{
	struct rk3538_clk_priv *priv = dev_get_priv(dev);

	priv->cru = dev_read_addr_ptr(dev);

	return 0;
}

static int rk3538_clk_bind(struct udevice *dev)
{
	struct udevice *sys_child;
	struct sysreset_reg *priv;
	int ret;

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(dev, "rockchip_sysreset", "sysreset",
				 &sys_child);
	if (ret) {
		debug("Warning: No sysreset driver: ret=%d\n", ret);
	} else {
		priv = malloc(sizeof(struct sysreset_reg));
		priv->glb_srst_fst_value = offsetof(struct rk3538_cru,
						    glb_srst_fst);
		priv->glb_srst_snd_value = offsetof(struct rk3538_cru,
						    glb_srst_snd);
		dev_set_priv(sys_child, priv);
	}

#if CONFIG_IS_ENABLED(RESET_ROCKCHIP)
	ret = offsetof(struct rk3538_cru, softrst_con[0]);
	ret = rk3538_reset_bind_lut(dev, ret, 147457);
	if (ret)
		debug("Warning: software reset driver bind failed\n");
#endif

	return 0;
}

static const struct udevice_id rk3538_clk_ids[] = {
	{ .compatible = "rockchip,rk3538-cru" },
	{ }
};

U_BOOT_DRIVER(rockchip_rk3538_cru) = {
	.name		= "rockchip_rk3538_cru",
	.id		= UCLASS_CLK,
	.of_match	= rk3538_clk_ids,
	.priv_auto	= sizeof(struct rk3538_clk_priv),
	.of_to_plat	= rk3538_clk_ofdata_to_platdata,
	.ops		= &rk3538_clk_ops,
	.bind		= rk3538_clk_bind,
	.probe		= rk3538_clk_probe,
};

#ifndef CONFIG_SPL_BUILD
/**
 * soc_clk_dump() - Print clock frequencies
 * Returns zero on success
 *
 * Implementation for the clk dump command.
 */
int soc_clk_dump(void)
{
	const struct rk3538_clk_info *clk_dump;
	struct rk3538_clk_priv *priv;
	struct udevice *cru_dev;
	struct clk clk;
	ulong clk_count = ARRAY_SIZE(clks_dump);
	ulong rate;
	int i, ret;

	ret = uclass_get_device_by_driver(UCLASS_CLK,
					  DM_DRIVER_GET(rockchip_rk3538_cru),
					  &cru_dev);
	if (ret) {
		printf("%s failed to get cru device\n", __func__);
		return ret;
	}

	priv = dev_get_priv(cru_dev);
	printf("CLK: (%s. arm: enter %lu KHz, init %lu KHz, kernel %lu%s)\n",
	       priv->sync_kernel ? "sync kernel" : "uboot",
	       priv->armclk_enter_hz / 1000,
	       priv->armclk_init_hz / 1000,
	       priv->set_armclk_rate ? priv->armclk_hz / 1000 : 0,
	       priv->set_armclk_rate ? " KHz" : "N/A");
	for (i = 0; i < clk_count; i++) {
		clk_dump = &clks_dump[i];
		if (clk_dump->name) {
			clk.id = clk_dump->id;
			ret = clk_request(cru_dev, &clk);
			if (ret < 0)
				return ret;

			rate = clk_get_rate(&clk);
			if (i == 0) {
				if (rate < 0)
					printf("  %s %s\n", clk_dump->name,
					       "unknown");
				else
					printf("  %s %lu KHz\n", clk_dump->name,
					       rate / 1000);
			} else {
				if (rate < 0)
					printf("  %s %s\n", clk_dump->name,
					       "unknown");
				else
					printf("  %s %lu KHz\n", clk_dump->name,
					       rate / 1000);
			}
		}
	}

	return 0;
}
#endif
