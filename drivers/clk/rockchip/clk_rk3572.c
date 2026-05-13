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
#include <asm/arch-rockchip/cru_rk3572.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/hardware.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dt-bindings/clock/rockchip,rk3572-cru.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

#define DIV_TO_RATE(input_rate, div)	((input_rate) / ((div) + 1))

static struct rockchip_pll_rate_table rk3572_24m_pll_rates[] = {
	/* _mhz, _p, _m, _s, _k */
	RK3588_PLL_RATE(1500000000, 2, 250, 1, 0),
	RK3588_PLL_RATE(1200000000, 1, 100, 1, 0),
	RK3588_PLL_RATE(1188000000, 2, 198, 1, 0),
	RK3588_PLL_RATE(1150000000, 3, 575, 2, 0),
	RK3588_PLL_RATE(1100000000, 3, 550, 2, 0),
	RK3588_PLL_RATE(1008000000, 2, 336, 2, 0),
	RK3588_PLL_RATE(1000000000, 3, 500, 2, 0),
	RK3588_PLL_RATE(900000000, 2, 300, 2, 0),
	RK3588_PLL_RATE(850000000, 3, 425, 2, 0),
	RK3588_PLL_RATE(816000000, 2, 272, 2, 0),
	RK3588_PLL_RATE(786432000, 2, 262, 2, 9437),
	RK3588_PLL_RATE(786000000, 1, 131, 2, 0),
	RK3588_PLL_RATE(742500000, 4, 495, 2, 0),
	RK3588_PLL_RATE(722534400, 8, 963, 2, 24850),
	RK3588_PLL_RATE(610400000, 3, 305, 2, 13107),
	RK3588_PLL_RATE(600000000, 2, 200, 2, 0),
	RK3588_PLL_RATE(594000000, 2, 198, 2, 0),
	RK3588_PLL_RATE(200000000, 3, 400, 4, 0),
	RK3588_PLL_RATE(100000000, 3, 400, 5, 0),
	{ /* sentinel */ },
};

static struct rockchip_pll_clock rk3572_pll_clks[] = {
	[BPLL] = PLL(pll_rk3588, PLL_BPLL, RK3572_PLL_CON(0),
		      RK3572_MODE_CON1, 0, 15, 0, rk3572_24m_pll_rates),
	[LPLL] = PLL(pll_rk3588, PLL_LPLL, RK3572_LPLL_CON(16),
		     RK3572_LPLL_MODE_CON0, 0, 15, 0, rk3572_24m_pll_rates),
	[VPLL] = PLL(pll_rk3588, PLL_VPLL, RK3572_PLL_CON(88),
		      RK3572_MODE_CON0, 4, 15, 0, rk3572_24m_pll_rates),
	[AUPLL] = PLL(pll_rk3588, PLL_AUPLL, RK3572_PLL_CON(96),
		      RK3572_MODE_CON0, 6, 15, 0, rk3572_24m_pll_rates),
	[CPLL] = PLL(pll_rk3588, PLL_CPLL, RK3572_PLL_CON(104),
		     RK3572_MODE_CON0, 8, 15, 0, rk3572_24m_pll_rates),
	[GPLL] = PLL(pll_rk3588, PLL_GPLL, RK3572_PLL_CON(112),
		     RK3572_MODE_CON0, 2, 15, 0, rk3572_24m_pll_rates),
	[PPLL] = PLL(pll_rk3588, PLL_PPLL, RK3572_PPLL_CON(128),
		     RK3572_MODE_CON0, 2, 15, 0, rk3572_24m_pll_rates),
};

#ifndef CONFIG_SPL_BUILD
#define RK3572_CLK_DUMP(_id, _name, _iscru)	\
{						\
	.id = _id,				\
	.name = _name,				\
	.is_cru = _iscru,			\
}

static const struct rk3572_clk_info clks_dump[] = {
	RK3572_CLK_DUMP(PLL_BPLL, "bpll", true),
	RK3572_CLK_DUMP(PLL_LPLL, "lpll", true),
	RK3572_CLK_DUMP(PLL_VPLL, "vpll", true),
	RK3572_CLK_DUMP(PLL_AUPLL, "aupll", true),
	RK3572_CLK_DUMP(PLL_CPLL, "cpll", true),
	RK3572_CLK_DUMP(PLL_GPLL, "gpll", true),
	RK3572_CLK_DUMP(PLL_PPLL, "ppll", true),
	RK3572_CLK_DUMP(ACLK_BUS_ROOT, "aclk_bus_root", true),
	RK3572_CLK_DUMP(PCLK_BUS_ROOT, "pclk_bus_root", true),
	RK3572_CLK_DUMP(HCLK_BUS_ROOT, "hclk_bus_root", true),
	RK3572_CLK_DUMP(ACLK_TOP, "aclk_top", true),
	RK3572_CLK_DUMP(ACLK_TOP_MID, "aclk_top_mid", true),
	RK3572_CLK_DUMP(PCLK_TOP_ROOT, "pclk_top", true),
	RK3572_CLK_DUMP(HCLK_TOP, "hclk_top", true),
};
#endif

#ifdef CONFIG_SPL_BUILD
#ifndef BITS_WITH_WMASK
#define BITS_WITH_WMASK(bits, msk, shift) \
	((bits) << (shift)) | ((msk) << ((shift) + 16))
#endif
#endif

#ifndef CONFIG_SPL_BUILD
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
#endif

static ulong rk3572_bus_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 con, sel, div, rate;

	switch (clk_id) {
	case ACLK_BUS_ROOT:
		con = readl(&cru->clksel_con[64]);
		sel = (con & ACLK_BUS_ROOT_SEL_MASK) >>
		      ACLK_BUS_ROOT_SEL_SHIFT;
		div = (con & ACLK_BUS_ROOT_DIV_MASK) >>
		      ACLK_BUS_ROOT_DIV_SHIFT;
		if (sel == ACLK_BUS_ROOT_SEL_CPLL)
			rate = DIV_TO_RATE(priv->cpll_hz, div);
		else
			rate = DIV_TO_RATE(priv->gpll_hz, div);
		break;
	case HCLK_BUS_ROOT:
		con = readl(&cru->clksel_con[64]);
		sel = (con & HCLK_BUS_ROOT_SEL_MASK) >>
		      HCLK_BUS_ROOT_SEL_SHIFT;
		if (sel == HCLK_BUS_ROOT_SEL_200M)
			rate = 198 * MHz;
		else if (sel == HCLK_BUS_ROOT_SEL_100M)
			rate = 100 * MHz;
		else if (sel == HCLK_BUS_ROOT_SEL_50M)
			rate = 50 * MHz;
		else
			rate = OSC_HZ;
		break;
	case PCLK_BUS_ROOT:
		con = readl(&cru->clksel_con[64]);
		sel = (con & PCLK_BUS_ROOT_SEL_MASK) >>
		      PCLK_BUS_ROOT_SEL_SHIFT;
		if (sel == PCLK_BUS_ROOT_SEL_100M)
			rate = 100 * MHz;
		else if (sel == PCLK_BUS_ROOT_SEL_50M)
			rate = 50 * MHz;
		else
			rate = OSC_HZ;
		break;
	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rk3572_bus_set_clk(struct rk3572_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk, src_clk_div;

	switch (clk_id) {
	case ACLK_BUS_ROOT:
		if (!(priv->cpll_hz % rate)) {
			src_clk = ACLK_BUS_ROOT_SEL_CPLL;
			src_clk_div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else {
			src_clk = ACLK_BUS_ROOT_SEL_GPLL;
			src_clk_div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		rk_clrsetreg(&cru->clksel_con[64],
			     ACLK_BUS_ROOT_SEL_MASK,
			     src_clk << ACLK_BUS_ROOT_SEL_SHIFT);
		assert(src_clk_div - 1 <= 31);
		rk_clrsetreg(&cru->clksel_con[64],
			     ACLK_BUS_ROOT_DIV_MASK |
			     ACLK_BUS_ROOT_SEL_MASK,
			     (src_clk <<
			      ACLK_BUS_ROOT_SEL_SHIFT) |
			     (src_clk_div - 1) << ACLK_BUS_ROOT_DIV_SHIFT);
		break;
	case HCLK_BUS_ROOT:
		if (rate >= 198 * MHz)
			src_clk = HCLK_BUS_ROOT_SEL_200M;
		else if (rate >= 99 * MHz)
			src_clk = HCLK_BUS_ROOT_SEL_100M;
		else if (rate >= 50 * MHz)
			src_clk = HCLK_BUS_ROOT_SEL_50M;
		else
			src_clk = HCLK_BUS_ROOT_SEL_OSC;
		rk_clrsetreg(&cru->clksel_con[64],
			     HCLK_BUS_ROOT_SEL_MASK,
			     src_clk << HCLK_BUS_ROOT_SEL_SHIFT);
		break;
	case PCLK_BUS_ROOT:
		if (rate >= 99 * MHz)
			src_clk = PCLK_BUS_ROOT_SEL_100M;
		else if (rate >= 50 * MHz)
			src_clk = PCLK_BUS_ROOT_SEL_50M;
		else
			src_clk = PCLK_BUS_ROOT_SEL_OSC;
		rk_clrsetreg(&cru->clksel_con[64],
			     PCLK_BUS_ROOT_SEL_MASK,
			     src_clk << PCLK_BUS_ROOT_SEL_SHIFT);
		break;
	default:
		printf("do not support this center freq\n");
		return -EINVAL;
	}

	return rk3572_bus_get_clk(priv, clk_id);
}

static ulong rk3572_top_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 con, sel, div, rate, prate;

	switch (clk_id) {
	case ACLK_TOP:
		con = readl(&cru->clksel_con[9]);
		div = (con & ACLK_TOP_DIV_MASK) >>
		      ACLK_TOP_DIV_SHIFT;
		sel = (con & ACLK_TOP_SEL_MASK) >>
		      ACLK_TOP_SEL_SHIFT;
		if (sel == ACLK_TOP_SEL_CPLL)
			prate = priv->cpll_hz;
		else if (sel == ACLK_TOP_SEL_AUPLL)
			prate = priv->aupll_hz;
		else
			prate = priv->gpll_hz;
		return DIV_TO_RATE(prate, div);
	case ACLK_TOP_MID:
		con = readl(&cru->clksel_con[10]);
		div = (con & ACLK_TOP_MID_DIV_MASK) >>
		      ACLK_TOP_MID_DIV_SHIFT;
		sel = (con & ACLK_TOP_MID_SEL_MASK) >>
		      ACLK_TOP_MID_SEL_SHIFT;
		if (sel == ACLK_TOP_MID_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = priv->gpll_hz;
		return DIV_TO_RATE(prate, div);
	case PCLK_TOP_ROOT:
		con = readl(&cru->clksel_con[8]);
		sel = (con & PCLK_TOP_SEL_MASK) >> PCLK_TOP_SEL_SHIFT;
		if (sel == PCLK_TOP_SEL_100M)
			rate = 100 * MHz;
		else if (sel == PCLK_TOP_SEL_50M)
			rate = 50 * MHz;
		else
			rate = OSC_HZ;
		break;
	case HCLK_TOP:
		con = readl(&cru->clksel_con[19]);
		sel = (con & HCLK_TOP_SEL_MASK) >> HCLK_TOP_SEL_SHIFT;
		if (sel == HCLK_TOP_SEL_200M)
			rate = 200 * MHz;
		else if (sel == HCLK_TOP_SEL_100M)
			rate = 100 * MHz;
		else if (sel == HCLK_TOP_SEL_50M)
			rate = 50 * MHz;
		else
			rate = OSC_HZ;
		break;
	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rk3572_top_set_clk(struct rk3572_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk, src_clk_div;

	switch (clk_id) {
	case ACLK_TOP:
		if (!(priv->cpll_hz % rate)) {
			src_clk = ACLK_TOP_SEL_CPLL;
			src_clk_div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else {
			src_clk = ACLK_TOP_SEL_GPLL;
			src_clk_div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		assert(src_clk_div - 1 <= 31);
		rk_clrsetreg(&cru->clksel_con[9],
			     ACLK_TOP_DIV_MASK |
			     ACLK_TOP_SEL_MASK,
			     (src_clk <<
			      ACLK_TOP_SEL_SHIFT) |
			     (src_clk_div - 1) << ACLK_TOP_SEL_SHIFT);
		break;
	case ACLK_TOP_MID:
		if (!(priv->cpll_hz % rate)) {
			src_clk = ACLK_TOP_MID_SEL_CPLL;
			src_clk_div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else {
			src_clk = ACLK_TOP_MID_SEL_GPLL;
			src_clk_div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		rk_clrsetreg(&cru->clksel_con[10],
			     ACLK_TOP_MID_DIV_MASK |
			     ACLK_TOP_MID_SEL_MASK,
			     (ACLK_TOP_MID_SEL_GPLL <<
			      ACLK_TOP_MID_SEL_SHIFT) |
			     (src_clk_div - 1) << ACLK_TOP_MID_DIV_SHIFT);
		break;
	case PCLK_TOP_ROOT:
		if (rate >= 99 * MHz)
			src_clk = PCLK_TOP_SEL_100M;
		else if (rate >= 50 * MHz)
			src_clk = PCLK_TOP_SEL_50M;
		else
			src_clk = PCLK_TOP_SEL_OSC;
		rk_clrsetreg(&cru->clksel_con[8],
			     PCLK_TOP_SEL_MASK,
			     src_clk << PCLK_TOP_SEL_SHIFT);
		break;
	case HCLK_TOP:
		if (rate >= 198 * MHz)
			src_clk = HCLK_TOP_SEL_200M;
		else if (rate >= 99 * MHz)
			src_clk = HCLK_TOP_SEL_100M;
		else if (rate >= 50 * MHz)
			src_clk = HCLK_TOP_SEL_50M;
		else
			src_clk = HCLK_TOP_SEL_OSC;
		rk_clrsetreg(&cru->clksel_con[19],
			     HCLK_TOP_SEL_MASK,
			     src_clk << HCLK_TOP_SEL_SHIFT);
		break;
	default:
		printf("do not support this top freq\n");
		return -EINVAL;
	}

	return rk3572_top_get_clk(priv, clk_id);
}

static ulong rk3572_i2c_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 sel, con;
	ulong rate;

	switch (clk_id) {
	case CLK_I2C0:
		con = readl(&cru->pmuclksel_con[6]);
		sel = (con & CLK_I2C0_SEL_MASK) >> CLK_I2C0_SEL_SHIFT;
		break;
	case CLK_I2C1:
		con = readl(&cru->clksel_con[67]);
		sel = (con & CLK_I2C1_SEL_MASK) >> CLK_I2C1_SEL_SHIFT;
		break;
	case CLK_I2C2:
		con = readl(&cru->clksel_con[67]);
		sel = (con & CLK_I2C2_SEL_MASK) >> CLK_I2C2_SEL_SHIFT;
		break;
	case CLK_I2C3:
		con = readl(&cru->clksel_con[67]);
		sel = (con & CLK_I2C3_SEL_MASK) >> CLK_I2C3_SEL_SHIFT;
		break;
	case CLK_I2C4:
		con = readl(&cru->clksel_con[67]);
		sel = (con & CLK_I2C4_SEL_MASK) >> CLK_I2C4_SEL_SHIFT;
		break;
	case CLK_I2C5:
		con = readl(&cru->clksel_con[68]);
		sel = (con & CLK_I2C5_SEL_MASK) >> CLK_I2C5_SEL_SHIFT;
		break;
	case CLK_I2C6:
		con = readl(&cru->clksel_con[68]);
		sel = (con & CLK_I2C6_SEL_MASK) >> CLK_I2C6_SEL_SHIFT;
		break;
	case CLK_I2C7:
		con = readl(&cru->clksel_con[68]);
		sel = (con & CLK_I2C7_SEL_MASK) >> CLK_I2C7_SEL_SHIFT;
		break;
	case CLK_I2C8:
		con = readl(&cru->clksel_con[68]);
		sel = (con & CLK_I2C8_SEL_MASK) >> CLK_I2C8_SEL_SHIFT;
		break;
	case CLK_I2C9:
		con = readl(&cru->clksel_con[68]);
		sel = (con & CLK_I2C9_SEL_MASK) >> CLK_I2C9_SEL_SHIFT;
		break;

	default:
		return -ENOENT;
	}
	if (sel == CLK_I2C_SEL_200M)
		rate = 198 * MHz;
	else if (sel == CLK_I2C_SEL_100M)
		rate = 100 * MHz;
	else if (sel == CLK_I2C_SEL_50M)
		rate = 50 * MHz;
	else
		rate = OSC_HZ;

	return rate;
}

static ulong rk3572_i2c_set_clk(struct rk3572_clk_priv *priv, ulong clk_id,
				ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk;

	if (rate >= 198 * MHz)
		src_clk = CLK_I2C_SEL_200M;
	else if (rate >= 99 * MHz)
		src_clk = CLK_I2C_SEL_100M;
	if (rate >= 50 * MHz)
		src_clk = CLK_I2C_SEL_50M;
	else
		src_clk = CLK_I2C_SEL_OSC;

	switch (clk_id) {
	case CLK_I2C0:
		rk_clrsetreg(&cru->pmuclksel_con[6], CLK_I2C0_SEL_MASK,
			     src_clk << CLK_I2C0_SEL_SHIFT);
		break;
	case CLK_I2C1:
		rk_clrsetreg(&cru->clksel_con[67], CLK_I2C1_SEL_MASK,
			     src_clk << CLK_I2C1_SEL_SHIFT);
		break;
	case CLK_I2C2:
		rk_clrsetreg(&cru->clksel_con[67], CLK_I2C2_SEL_MASK,
			     src_clk << CLK_I2C2_SEL_SHIFT);
		break;
	case CLK_I2C3:
		rk_clrsetreg(&cru->clksel_con[67], CLK_I2C3_SEL_MASK,
			     src_clk << CLK_I2C3_SEL_SHIFT);
		break;
	case CLK_I2C4:
		rk_clrsetreg(&cru->clksel_con[67], CLK_I2C4_SEL_MASK,
			     src_clk << CLK_I2C4_SEL_SHIFT);
		break;
	case CLK_I2C5:
		rk_clrsetreg(&cru->clksel_con[68], CLK_I2C5_SEL_MASK,
			     src_clk << CLK_I2C5_SEL_SHIFT);
		break;
	case CLK_I2C6:
		rk_clrsetreg(&cru->clksel_con[68], CLK_I2C6_SEL_MASK,
			     src_clk << CLK_I2C6_SEL_SHIFT);
		break;
	case CLK_I2C7:
		rk_clrsetreg(&cru->clksel_con[68], CLK_I2C7_SEL_MASK,
			     src_clk << CLK_I2C7_SEL_SHIFT);
		break;
	case CLK_I2C8:
		rk_clrsetreg(&cru->clksel_con[68], CLK_I2C8_SEL_MASK,
			     src_clk << CLK_I2C8_SEL_SHIFT);
	case CLK_I2C9:
		rk_clrsetreg(&cru->clksel_con[68], CLK_I2C9_SEL_MASK,
			     src_clk << CLK_I2C9_SEL_SHIFT);
		break;
	default:
		return -ENOENT;
	}

	return rk3572_i2c_get_clk(priv, clk_id);
}

static ulong rk3572_spi_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 sel, con;

	switch (clk_id) {
	case CLK_SPI0:
		con = readl(&cru->clksel_con[81]);
		sel = (con & CLK_SPI0_SEL_MASK) >> CLK_SPI0_SEL_SHIFT;
		break;
	case CLK_SPI1:
		con = readl(&cru->clksel_con[82]);
		sel = (con & CLK_SPI1_SEL_MASK) >> CLK_SPI1_SEL_SHIFT;
		break;
	case CLK_SPI2:
		con = readl(&cru->clksel_con[82]);
		sel = (con & CLK_SPI2_SEL_MASK) >> CLK_SPI2_SEL_SHIFT;
		break;
	case CLK_SPI3:
		con = readl(&cru->clksel_con[82]);
		sel = (con & CLK_SPI3_SEL_MASK) >> CLK_SPI3_SEL_SHIFT;
		break;
	case CLK_SPI4:
		con = readl(&cru->clksel_con[82]);
		sel = (con & CLK_SPI4_SEL_MASK) >> CLK_SPI4_SEL_SHIFT;
		break;
	default:
		return -ENOENT;
	}

	switch (sel) {
	case CLK_SPI_SEL_300M:
		return 297 * MHz;
	case CLK_SPI_SEL_200M:
		return 198 * MHz;
	case CLK_SPI_SEL_100M:
		return 100 * MHz;
	case CLK_SPI_SEL_OSC:
		return OSC_HZ;
	default:
		return -ENOENT;
	}
}

static ulong rk3572_spi_set_clk(struct rk3572_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk;

	if (rate >= 297 * MHz)
		src_clk = CLK_SPI_SEL_300M;
	else if (rate >= 198 * MHz)
		src_clk = CLK_SPI_SEL_200M;
	else if (rate >= 99 * MHz)
		src_clk = CLK_SPI_SEL_100M;
	else
		src_clk = CLK_SPI_SEL_OSC;

	switch (clk_id) {
	case CLK_SPI0:
		rk_clrsetreg(&cru->clksel_con[81],
			     CLK_SPI0_SEL_MASK,
			     src_clk << CLK_SPI0_SEL_SHIFT);
		break;
	case CLK_SPI1:
		rk_clrsetreg(&cru->clksel_con[82],
			     CLK_SPI1_SEL_MASK,
			     src_clk << CLK_SPI1_SEL_SHIFT);
		break;
	case CLK_SPI2:
		rk_clrsetreg(&cru->clksel_con[82],
			     CLK_SPI2_SEL_MASK,
			     src_clk << CLK_SPI2_SEL_SHIFT);
		break;
	case CLK_SPI3:
		rk_clrsetreg(&cru->clksel_con[82],
			     CLK_SPI3_SEL_MASK,
			     src_clk << CLK_SPI3_SEL_SHIFT);
		break;
	case CLK_SPI4:
		rk_clrsetreg(&cru->clksel_con[82],
			     CLK_SPI4_SEL_MASK,
			     src_clk << CLK_SPI4_SEL_SHIFT);
		break;
	default:
		return -ENOENT;
	}

	return rk3572_spi_get_clk(priv, clk_id);
}

static ulong rk3572_pwm_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 sel, con;

	switch (clk_id) {
	case CLK_PWM1:
		con = readl(&cru->clksel_con[82]);
		sel = (con & CLK_PWM1_SEL_MASK) >> CLK_PWM1_SEL_SHIFT;
		break;
	case CLK_PWM2:
		con = readl(&cru->clksel_con[83]);
		sel = (con & CLK_PWM2_SEL_MASK) >> CLK_PWM2_SEL_SHIFT;
		break;
	case CLK_PWM0:
		con = readl(&cru->pmuclksel_con[5]);
		sel = (con & CLK_PWM0_SEL_MASK) >> CLK_PWM0_SEL_SHIFT;
		break;
	default:
		return -ENOENT;
	}

	switch (sel) {
	case CLK_PWM_SEL_100M:
		return 100 * MHz;
	case CLK_PWM_SEL_50M:
		return 50 * MHz;
	case CLK_PWM_SEL_OSC:
		return OSC_HZ;
	default:
		return -ENOENT;
	}
}

static ulong rk3572_pwm_set_clk(struct rk3572_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk;

	if (rate >= 99 * MHz)
		src_clk = CLK_PWM_SEL_100M;
	else if (rate >= 50 * MHz)
		src_clk = CLK_PWM_SEL_50M;
	else
		src_clk = CLK_PWM_SEL_OSC;

	switch (clk_id) {
	case CLK_PWM1:
		rk_clrsetreg(&cru->clksel_con[82],
			     CLK_PWM1_SEL_MASK,
			     src_clk << CLK_PWM1_SEL_SHIFT);
		break;
	case CLK_PWM2:
		rk_clrsetreg(&cru->clksel_con[83],
			     CLK_PWM2_SEL_MASK,
			     src_clk << CLK_PWM2_SEL_SHIFT);
		break;
	case CLK_PWM0:
		rk_clrsetreg(&cru->pmuclksel_con[5],
			     CLK_PWM0_SEL_MASK,
			     src_clk << CLK_PWM0_SEL_SHIFT);
		break;
	default:
		return -ENOENT;
	}

	return rk3572_pwm_get_clk(priv, clk_id);
}

static ulong rk3572_adc_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 div, sel, con, prate;

	switch (clk_id) {
	case CLK_SARADC:
		con = readl(&cru->clksel_con[69]);
		div = (con & CLK_SARADC_DIV_MASK) >> CLK_SARADC_DIV_SHIFT;
		sel = (con & CLK_SARADC_SEL_MASK) >>
		      CLK_SARADC_SEL_SHIFT;
		if (sel == CLK_SARADC_SEL_OSC)
			prate = OSC_HZ;
		else
			prate = priv->gpll_hz;
		return DIV_TO_RATE(prate, div);
	case CLK_TSADC:
		con = readl(&cru->clksel_con[70]);
		div = (con & CLK_TSADC_DIV_MASK) >>
		      CLK_TSADC_DIV_SHIFT;
		prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	default:
		return -ENOENT;
	}
}

static ulong rk3572_adc_set_clk(struct rk3572_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk_div;

	switch (clk_id) {
	case CLK_SARADC:
		if (!(OSC_HZ % rate)) {
			src_clk_div = DIV_ROUND_UP(OSC_HZ, rate);
			assert(src_clk_div - 1 <= 255);
			rk_clrsetreg(&cru->clksel_con[69],
				     CLK_SARADC_SEL_MASK |
				     CLK_SARADC_DIV_MASK,
				     (CLK_SARADC_SEL_OSC <<
				      CLK_SARADC_SEL_SHIFT) |
				     (src_clk_div - 1) <<
				     CLK_SARADC_DIV_SHIFT);
		} else {
			src_clk_div = DIV_ROUND_UP(priv->gpll_hz, rate);
			assert(src_clk_div - 1 <= 255);
			rk_clrsetreg(&cru->clksel_con[69],
				     CLK_SARADC_SEL_MASK |
				     CLK_SARADC_DIV_MASK,
				     (CLK_SARADC_SEL_GPLL <<
				      CLK_SARADC_SEL_SHIFT) |
				     (src_clk_div - 1) <<
				     CLK_SARADC_DIV_SHIFT);
		}
		break;
	case CLK_TSADC:
		src_clk_div = DIV_ROUND_UP(OSC_HZ, rate);
		assert(src_clk_div - 1 <= 255);
		rk_clrsetreg(&cru->clksel_con[70],
			     CLK_TSADC_DIV_MASK,
			     (src_clk_div - 1) <<
			     CLK_TSADC_DIV_SHIFT);
		break;
	default:
		return -ENOENT;
	}
	return rk3572_adc_get_clk(priv, clk_id);
}

static ulong rk3572_mmc_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 sel, con, prate, div = 0;

	switch (clk_id) {
	case CCLK_SRC_SDMMC0:
	case HCLK_SDMMC0:
		con = readl(&cru->clksel_con[121]);
		div = (con & CCLK_SDMMC0_SRC_DIV_MASK) >> CCLK_SDMMC0_SRC_DIV_SHIFT;
		sel = (con & CCLK_SDMMC0_SRC_SEL_MASK) >>
		      CCLK_SDMMC0_SRC_SEL_SHIFT;
		if (sel == CCLK_SDMMC0_SRC_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == CCLK_SDMMC0_SRC_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case CCLK_SRC_SDMMC1:
	case HCLK_SDMMC1:
		con = readl(&cru->clksel_con[116]);
		div = (con & CCLK_SDMMC1_SRC_DIV_MASK) >> CCLK_SDMMC1_SRC_DIV_SHIFT;
		sel = (con & CCLK_SDMMC1_SRC_SEL_MASK) >>
		      CCLK_SDMMC1_SRC_SEL_SHIFT;
		if (sel == CCLK_SDMMC1_SRC_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == CCLK_SDMMC1_SRC_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case CCLK_SRC_EMMC:
	case HCLK_EMMC:
		con = readl(&cru->clksel_con[114]);
		div = (con & CCLK_EMMC_DIV_MASK) >> CCLK_EMMC_DIV_SHIFT;
		sel = (con & CCLK_EMMC_SEL_MASK) >>
		      CCLK_EMMC_SEL_SHIFT;
		if (sel == CCLK_EMMC_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == CCLK_EMMC_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case BCLK_EMMC:
		con = readl(&cru->clksel_con[114]);
		sel = (con & BCLK_EMMC_SEL_MASK) >>
		      BCLK_EMMC_SEL_SHIFT;
		if (sel == BCLK_EMMC_SEL_200M)
			prate = 200 * MHz;
		else if (sel == BCLK_EMMC_SEL_100M)
			prate = 100 * MHz;
		else if (sel == BCLK_EMMC_SEL_50M)
			prate = 50 * MHz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case SCLK_FSPI0_X2:
		con = readl(&cru->clksel_con[115]);
		div = (con & SCLK_FSPI0_DIV_MASK) >> SCLK_FSPI0_DIV_SHIFT;
		sel = (con & SCLK_FSPI0_SEL_MASK) >>
		      SCLK_FSPI0_SEL_SHIFT;
		if (sel == SCLK_FSPI0_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == SCLK_FSPI0_SEL_CPLL)
			prate = priv->cpll_hz;
		else if (sel == SCLK_FSPI0_SEL_SPLL)
			prate = priv->spll_hz;
		else if (sel == SCLK_FSPI0_SEL_AUPLL)
			prate = priv->aupll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case SCLK_FSPI1_X2:
		con = readl(&cru->clksel_con[122]);
		div = (con & SCLK_FSPI1_DIV_MASK) >> SCLK_FSPI1_DIV_SHIFT;
		sel = (con & SCLK_FSPI1_SEL_MASK) >>
		      SCLK_FSPI1_SEL_SHIFT;
		if (sel == SCLK_FSPI1_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == SCLK_FSPI1_SEL_CPLL)
			prate = priv->cpll_hz;
		else if (sel == SCLK_FSPI1_SEL_SPLL)
			prate = priv->spll_hz;
		else if (sel == SCLK_FSPI1_SEL_AUPLL)
			prate = priv->aupll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case DCLK_DECOM:
		con = readl(&cru->clksel_con[84]);
		div = (con & DCLK_DECOM_DIV_MASK) >> DCLK_DECOM_DIV_SHIFT;
		sel = (con & DCLK_DECOM_SEL_MASK) >> DCLK_DECOM_SEL_SHIFT;
		if (sel == DCLK_DECOM_SEL_SPLL)
			prate = priv->spll_hz;
		else if (sel == DCLK_DECOM_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = priv->gpll_hz;
		return DIV_TO_RATE(prate, div);

	default:
		return -ENOENT;
	}
}

static ulong rk3572_mmc_set_clk(struct rk3572_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk, div = 0;

	switch (clk_id) {
	case CCLK_SRC_SDMMC0:
	case CCLK_SRC_SDMMC1:
	case CCLK_SRC_EMMC:
	case HCLK_SDMMC0:
	case HCLK_SDMMC1:
	case HCLK_EMMC:
		if (!(OSC_HZ % rate)) {
			src_clk = CCLK_SDMMC0_SRC_SEL_OSC;
			div = DIV_ROUND_UP(OSC_HZ, rate);
		} else if (!(priv->cpll_hz % rate)) {
			src_clk = CCLK_SDMMC0_SRC_SEL_CPLL;
			div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else {
			src_clk = CCLK_SDMMC0_SRC_SEL_GPLL;
			div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		break;
	case SCLK_FSPI0_X2:
	case SCLK_FSPI1_X2:
		if (!(OSC_HZ % rate)) {
			src_clk = SCLK_FSPI0_SEL_OSC;
			div = DIV_ROUND_UP(OSC_HZ, rate);
		} else if (!(priv->cpll_hz % rate)) {
			src_clk = SCLK_FSPI0_SEL_CPLL;
			div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else if (!(priv->spll_hz % rate)) {
			src_clk = SCLK_FSPI0_SEL_SPLL;
			div = DIV_ROUND_UP(priv->spll_hz, rate);
		} else if (!(priv->aupll_hz % rate)) {
			src_clk = SCLK_FSPI0_SEL_AUPLL;
			div = DIV_ROUND_UP(priv->aupll_hz, rate);
		} else {
			src_clk = SCLK_FSPI0_SEL_GPLL;
			div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		break;
	case BCLK_EMMC:
		if (rate >= 198 * MHz)
			src_clk = BCLK_EMMC_SEL_200M;
		else if (rate >= 99 * MHz)
			src_clk = BCLK_EMMC_SEL_100M;
		else if (rate >= 50 * MHz)
			src_clk = BCLK_EMMC_SEL_50M;
		else
			src_clk = BCLK_EMMC_SEL_OSC;
		break;
	case DCLK_DECOM:
		if (!(priv->spll_hz % rate)) {
			src_clk = DCLK_DECOM_SEL_SPLL;
			div = DIV_ROUND_UP(priv->spll_hz, rate);
		} else if (!(priv->cpll_hz % rate)) {
			src_clk = DCLK_DECOM_SEL_CPLL;
			div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else {
			src_clk = DCLK_DECOM_SEL_GPLL;
			div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		break;
	default:
		return -ENOENT;
	}

	switch (clk_id) {
	case CCLK_SRC_SDMMC0:
	case HCLK_SDMMC0:
		rk_clrsetreg(&cru->clksel_con[121],
			     CCLK_SDMMC0_SRC_SEL_MASK |
			     CCLK_SDMMC0_SRC_DIV_MASK,
			     (src_clk << CCLK_SDMMC0_SRC_SEL_SHIFT) |
			     (div - 1) << CCLK_SDMMC0_SRC_DIV_SHIFT);
		break;
	case CCLK_SRC_SDMMC1:
	case HCLK_SDMMC1:
		rk_clrsetreg(&cru->clksel_con[116],
			     CCLK_SDMMC1_SRC_SEL_MASK |
			     CCLK_SDMMC1_SRC_DIV_MASK,
			     (src_clk << CCLK_SDMMC1_SRC_SEL_SHIFT) |
			     (div - 1) << CCLK_SDMMC1_SRC_DIV_SHIFT);
		break;
	case CCLK_SRC_EMMC:
	case HCLK_EMMC:
		rk_clrsetreg(&cru->clksel_con[114],
			     CCLK_EMMC_DIV_MASK |
			     CCLK_EMMC_SEL_MASK,
			     (src_clk << CCLK_EMMC_SEL_SHIFT) |
			     (div - 1) << CCLK_EMMC_DIV_SHIFT);
		break;
	case SCLK_FSPI0_X2:
		rk_clrsetreg(&cru->clksel_con[115],
			     SCLK_FSPI0_DIV_MASK |
			     SCLK_FSPI0_SEL_MASK,
			     (src_clk << SCLK_FSPI0_SEL_SHIFT) |
			     (div - 1) << SCLK_FSPI0_DIV_SHIFT);
		break;
	case SCLK_FSPI1_X2:
		rk_clrsetreg(&cru->clksel_con[122],
			     SCLK_FSPI1_DIV_MASK |
			     SCLK_FSPI1_SEL_MASK,
			     (src_clk << SCLK_FSPI1_SEL_SHIFT) |
			     (div - 1) << SCLK_FSPI1_DIV_SHIFT);
		break;
	case BCLK_EMMC:
		rk_clrsetreg(&cru->clksel_con[114],
			     BCLK_EMMC_SEL_MASK,
			     src_clk << BCLK_EMMC_SEL_SHIFT);
		break;
	case DCLK_DECOM:
		rk_clrsetreg(&cru->clksel_con[84],
			     DCLK_DECOM_DIV_MASK |
			     DCLK_DECOM_SEL_MASK,
			     (src_clk << DCLK_DECOM_SEL_SHIFT) |
			     (div - 1) << DCLK_DECOM_DIV_SHIFT);
		break;

	default:
		return -ENOENT;
	}

	return rk3572_mmc_get_clk(priv, clk_id);
}

#ifndef CONFIG_SPL_BUILD

static ulong rk3572_aclk_vop_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 div, sel, con, parent = 0;

	switch (clk_id) {
	case ACLK_VOP_ROOT:
	case ACLK_VOP:
		con = readl(&cru->clksel_con[174]);
		div = (con & ACLK_VOP_ROOT_DIV_MASK) >> ACLK_VOP_ROOT_DIV_SHIFT;
		sel = (con & ACLK_VOP_ROOT_SEL_MASK) >> ACLK_VOP_ROOT_SEL_SHIFT;
		if (sel == ACLK_VOP_ROOT_SEL_CPLL)
			parent = priv->cpll_hz;
		else if (sel == ACLK_VOP_ROOT_SEL_AUPLL)
			parent = priv->aupll_hz;
		else if (sel == ACLK_VOP_ROOT_SEL_SPLL)
			parent = priv->spll_hz;
		else
			parent = priv->gpll_hz;
		return DIV_TO_RATE(parent, div);
	case ACLK_VO_ROOT:
		con = readl(&cru->clksel_con[185]);
		div = (con & ACLK_VO_ROOT_DIV_MASK) >> ACLK_VO_ROOT_DIV_SHIFT;
		sel = (con & ACLK_VO_ROOT_SEL_MASK) >> ACLK_VO_ROOT_SEL_SHIFT;
		if (sel == ACLK_VO_ROOT_SEL_GPLL)
			parent = priv->gpll_hz;
		else
			parent = priv->cpll_hz;
		return DIV_TO_RATE(parent, div);
	case HCLK_VOP_ROOT:
		con = readl(&cru->clksel_con[174]);
		sel = (con & HCLK_VOP_ROOT_SEL_MASK) >> HCLK_VOP_ROOT_SEL_SHIFT;
		if (sel == HCLK_VOP_ROOT_SEL_200M)
			return 200 * MHz;
		else if (sel == HCLK_VOP_ROOT_SEL_100M)
			return 100 * MHz;
		else if (sel == HCLK_VOP_ROOT_SEL_50M)
			return 50 * MHz;
		else
			return OSC_HZ;
	case PCLK_VOP_ROOT:
		con = readl(&cru->clksel_con[174]);
		sel = (con & PCLK_VOP_ROOT_SEL_MASK) >> PCLK_VOP_ROOT_SEL_SHIFT;
		if (sel == PCLK_VOP_ROOT_SEL_100M)
			return 100 * MHz;
		else if (sel == PCLK_VOP_ROOT_SEL_50M)
			return 50 * MHz;
		else
			return OSC_HZ;

	default:
		return -ENOENT;
	}
}

static ulong rk3572_aclk_vop_set_clk(struct rk3572_clk_priv *priv,
				     ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int src_clk, div;

	switch (clk_id) {
	case ACLK_VOP_ROOT:
	case ACLK_VOP:
		if (rate >= 700 * MHz) {
			src_clk = ACLK_VOP_ROOT_SEL_SPLL;
			div = 1;
		} else if (!(priv->cpll_hz % rate)) {
			src_clk = ACLK_VOP_ROOT_SEL_CPLL;
			div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else {
			src_clk = ACLK_VOP_ROOT_SEL_GPLL;
			div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		rk_clrsetreg(&cru->clksel_con[174],
			     ACLK_VOP_ROOT_DIV_MASK |
			     ACLK_VOP_ROOT_SEL_MASK,
			     (src_clk << ACLK_VOP_ROOT_SEL_SHIFT) |
			     (div - 1) << ACLK_VOP_ROOT_DIV_SHIFT);
		break;
	case ACLK_VO_ROOT:
		if (!(priv->cpll_hz % rate)) {
			src_clk = ACLK_VO_ROOT_SEL_CPLL;
			div = DIV_ROUND_UP(priv->cpll_hz, rate);
		} else {
			src_clk = ACLK_VO_ROOT_SEL_GPLL;
			div = DIV_ROUND_UP(priv->gpll_hz, rate);
		}
		rk_clrsetreg(&cru->clksel_con[185],
			     ACLK_VO_ROOT_DIV_MASK |
			     ACLK_VO_ROOT_SEL_MASK,
			     (src_clk << ACLK_VO_ROOT_SEL_SHIFT) |
			     (div - 1) << ACLK_VO_ROOT_DIV_SHIFT);
		break;
	case HCLK_VOP_ROOT:
		if (rate >= 198 * MHz)
			src_clk = HCLK_VOP_ROOT_SEL_200M;
		else if (rate >= 99 * MHz)
			src_clk = HCLK_VOP_ROOT_SEL_100M;
		else if (rate >= 50 * MHz)
			src_clk = HCLK_VOP_ROOT_SEL_50M;
		else
			src_clk = HCLK_VOP_ROOT_SEL_OSC;
		rk_clrsetreg(&cru->clksel_con[174],
			     HCLK_VOP_ROOT_SEL_MASK,
			     src_clk << HCLK_VOP_ROOT_SEL_SHIFT);
		break;
	case PCLK_VOP_ROOT:
		if (rate >= 99 * MHz)
			src_clk = PCLK_VOP_ROOT_SEL_100M;
		else if (rate >= 50 * MHz)
			src_clk = PCLK_VOP_ROOT_SEL_50M;
		else
			src_clk = PCLK_VOP_ROOT_SEL_OSC;
		rk_clrsetreg(&cru->clksel_con[174],
			     PCLK_VOP_ROOT_SEL_MASK,
			     src_clk << PCLK_VOP_ROOT_SEL_SHIFT);
		break;

	default:
		return -ENOENT;
	}

	return rk3572_aclk_vop_get_clk(priv, clk_id);
}

static ulong rk3572_dclk_vop_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 div, sel, con, parent;

	switch (clk_id) {
	case DCLK_VP0:
	case DCLK_VP0_SRC:
		con = readl(&cru->clksel_con[175]);
		div = (con & DCLK0_VOP_SRC_DIV_MASK) >> DCLK0_VOP_SRC_DIV_SHIFT;
		sel = (con & DCLK0_VOP_SRC_SEL_MASK) >> DCLK0_VOP_SRC_SEL_SHIFT;
		break;
	case DCLK_VP1:
	case DCLK_VP1_SRC:
		con = readl(&cru->clksel_con[176]);
		div = (con & DCLK0_VOP_SRC_DIV_MASK) >> DCLK0_VOP_SRC_DIV_SHIFT;
		sel = (con & DCLK0_VOP_SRC_SEL_MASK) >> DCLK0_VOP_SRC_SEL_SHIFT;
		break;
	default:
		return -ENOENT;
	}

	if (sel == DCLK_VOP_SRC_SEL_VPLL)
		parent = priv->vpll_hz;
	else if (sel == DCLK_VOP_SRC_SEL_BPLL)
		parent = priv->bpll_hz / 4;
	else if (sel == DCLK_VOP_SRC_SEL_GPLL)
		parent = priv->gpll_hz;
	else
		parent = priv->cpll_hz;

	return DIV_TO_RATE(parent, div);
}

#define RK3572_VOP_PLL_LIMIT_FREQ 594000000

static ulong rk3572_dclk_vop_set_clk(struct rk3572_clk_priv *priv,
				     ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	ulong pll_rate, now, best_rate = 0;
	u32 i, conid, con, sel, div, best_div = 0, best_sel = 0;
	u32 mask, div_shift, sel_shift;

	switch (clk_id) {
	case DCLK_VP0:
	case DCLK_VP0_SRC:
		conid = 175;
		con = readl(&cru->clksel_con[conid]);
		sel = (con & DCLK0_VOP_SRC_SEL_MASK) >> DCLK0_VOP_SRC_SEL_SHIFT;
		mask = DCLK0_VOP_SRC_SEL_MASK | DCLK0_VOP_SRC_DIV_MASK;
		div_shift = DCLK0_VOP_SRC_DIV_SHIFT;
		sel_shift = DCLK0_VOP_SRC_SEL_SHIFT;
		break;
	case DCLK_VP1:
	case DCLK_VP1_SRC:
		conid = 176;
		con = readl(&cru->clksel_con[conid]);
		sel = (con & DCLK0_VOP_SRC_SEL_MASK) >> DCLK0_VOP_SRC_SEL_SHIFT;
		mask = DCLK0_VOP_SRC_SEL_MASK | DCLK0_VOP_SRC_DIV_MASK;
		div_shift = DCLK0_VOP_SRC_DIV_SHIFT;
		sel_shift = DCLK0_VOP_SRC_SEL_SHIFT;
		break;
	default:
		return -ENOENT;
	}

	if (sel == DCLK_VOP_SRC_SEL_VPLL) {
		pll_rate = rockchip_pll_get_rate(&rk3572_pll_clks[VPLL],
						 priv->cru, VPLL);
		if (pll_rate >= RK3572_VOP_PLL_LIMIT_FREQ && pll_rate % rate == 0) {
			div = DIV_ROUND_UP(pll_rate, rate);
			rk_clrsetreg(&cru->clksel_con[conid],
				     mask,
				     DCLK_VOP_SRC_SEL_VPLL << sel_shift |
				     ((div - 1) << div_shift));
		} else {
			div = DIV_ROUND_UP(RK3572_VOP_PLL_LIMIT_FREQ, rate);
			if (div % 2)
				div = div + 1;
			rk_clrsetreg(&cru->clksel_con[conid],
				     mask,
				     DCLK_VOP_SRC_SEL_VPLL << sel_shift |
				     ((div - 1) << div_shift));
			rockchip_pll_set_rate(&rk3572_pll_clks[VPLL],
					      priv->cru, VPLL, div * rate);
			priv->vpll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[VPLL],
							      priv->cru, VPLL);
		}
	} else {
		for (i = 0; i <= DCLK_VOP_SRC_SEL_BPLL; i++) {
			switch (i) {
			case DCLK_VOP_SRC_SEL_GPLL:
				pll_rate = priv->gpll_hz;
				break;
			case DCLK_VOP_SRC_SEL_CPLL:
				pll_rate = priv->cpll_hz;
				break;
			case DCLK_VOP_SRC_SEL_BPLL:
				pll_rate = 0;
				break;
			case DCLK_VOP_SRC_SEL_VPLL:
				pll_rate = 0;
				break;
			default:
				printf("do not support this vop pll sel\n");
				return -EINVAL;
			}

			div = DIV_ROUND_UP(pll_rate, rate);
			if (div > 255 || div == 0)
				continue;
			now = pll_rate / div;
			if (abs(rate - now) < abs(rate - best_rate)) {
				best_rate = now;
				best_div = div;
				best_sel = i;
			}
			debug("p_rate=%lu, best_rate=%lu, div=%u, sel=%u\n",
			      pll_rate, best_rate, best_div, best_sel);
		}

		if (best_rate) {
			rk_clrsetreg(&cru->clksel_con[conid],
				     mask,
				     best_sel << sel_shift |
				     (best_div - 1) << div_shift);
		} else {
			printf("do not support this vop freq %lu\n", rate);
			return -EINVAL;
		}
	}

	return rk3572_dclk_vop_get_clk(priv, clk_id);
}

static ulong rk3572_dclk_ebc_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 div, sel, con, parent;
	unsigned long m = 0, n = 0;

	switch (clk_id) {
	case DCLK_EBC_INT_SRC:
		con = readl(&cru->clksel_con[144]);
		div = (con & DCLK_EBC_SRC_DIV_MASK) >> DCLK_EBC_SRC_DIV_SHIFT;
		con = readl(&cru->clksel_con[143]);
		sel = (con & DCLK_EBC_SRC_SEL_MASK) >> DCLK_EBC_SRC_SEL_SHIFT;
		if (sel == DCLK_EBC_SRC_SEL_CPLL)
			parent = priv->cpll_hz;
		else if (sel == DCLK_EBC_SRC_SEL_VPLL)
			parent = priv->vpll_hz;
		else if (sel == DCLK_EBC_SRC_SEL_AUPLL)
			parent = priv->aupll_hz;
		else if (sel == DCLK_EBC_SRC_SEL_GPLL)
			parent = priv->gpll_hz;
		else
			parent = OSC_HZ;
		return DIV_TO_RATE(parent, div);
	case DCLK_EBC_FRAC_SRC:
		con = readl(&cru->clksel_con[143]);
		div = readl(&cru->clksel_con[145]);
		sel = (con & DCLK_EBC_SRC_SEL_MASK) >> DCLK_EBC_SRC_SEL_SHIFT;
		if (sel == DCLK_EBC_SRC_SEL_GPLL)
			parent = priv->gpll_hz;
		else if (sel == DCLK_EBC_SRC_SEL_CPLL)
			parent = priv->cpll_hz;
		else if (sel == DCLK_EBC_SRC_SEL_VPLL)
			parent = priv->vpll_hz;
		else if (sel == DCLK_EBC_SRC_SEL_AUPLL)
			parent = priv->aupll_hz;
		else
			parent = OSC_HZ;

		n = div & CLK_UART_FRAC_NUMERATOR_MASK;
		n >>= CLK_UART_FRAC_NUMERATOR_SHIFT;
		m = div & CLK_UART_FRAC_DENOMINATOR_MASK;
		m >>= CLK_UART_FRAC_DENOMINATOR_SHIFT;
		return parent * n / m;
	case DCLK_EBC:
		con = readl(&cru->clksel_con[146]);
		sel = (con & DCLK_EBC_SEL_MASK) >> DCLK_EBC_SEL_SHIFT;
		if (sel == DCLK_EBC_SEL_INT)
			parent = rk3572_dclk_ebc_get_clk(priv, DCLK_EBC_INT_SRC);
		else
			parent = rk3572_dclk_ebc_get_clk(priv, DCLK_EBC_FRAC_SRC);
		return parent;
	default:
		return -ENOENT;
	}
}

static ulong rk3572_dclk_ebc_set_clk(struct rk3572_clk_priv *priv,
				     ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	ulong pll_rate, now, best_rate = 0;
	u32 i, con, sel, div, best_div = 0, best_sel = 0;
	unsigned long m = 0, n = 0, val;

	switch (clk_id) {
	case DCLK_EBC:
		con = readl(&cru->clksel_con[146]);
		sel = (con & DCLK_EBC_SEL_MASK) >> DCLK_EBC_SEL_SHIFT;
		if (sel == DCLK_EBC_SEL_INT) {
			con = readl(&cru->clksel_con[143]);
			sel = (con & DCLK_EBC_SRC_SEL_MASK) >> DCLK_EBC_SRC_SEL_SHIFT;
			if (sel == DCLK_EBC_SRC_SEL_VPLL) {
				pll_rate = rockchip_pll_get_rate(&rk3572_pll_clks[VPLL],
								 priv->cru, VPLL);
				if (pll_rate >= RK3572_VOP_PLL_LIMIT_FREQ &&
				    pll_rate % rate == 0) {
					div = DIV_ROUND_UP(pll_rate, rate);
					rk_clrsetreg(&cru->clksel_con[144],
						     DCLK_EBC_SRC_DIV_MASK,
						     (div - 1) << DCLK_EBC_SRC_DIV_SHIFT);
				} else {
					div = DIV_ROUND_UP(RK3572_VOP_PLL_LIMIT_FREQ,
							   rate);
					if (div % 2)
						div = div + 1;
					rk_clrsetreg(&cru->clksel_con[144],
						     DCLK_EBC_SRC_DIV_MASK,
						     (div - 1) << DCLK_EBC_SRC_DIV_SHIFT);
					rockchip_pll_set_rate(&rk3572_pll_clks[VPLL],
							      priv->cru,
							      VPLL, div * rate);
					priv->vpll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[VPLL],
									      priv->cru,
									      VPLL);
				}
			} else {
				for (i = 0; i < DCLK_EBC_SRC_SEL_BPLL; i++) {
					switch (i) {
					case DCLK_EBC_SRC_SEL_GPLL:
						pll_rate = priv->gpll_hz;
						break;
					case DCLK_EBC_SRC_SEL_CPLL:
						pll_rate = priv->cpll_hz;
						break;
					case DCLK_EBC_SRC_SEL_VPLL:
						pll_rate = 0;
						break;
					case DCLK_EBC_SRC_SEL_AUPLL:
						pll_rate = priv->aupll_hz;
						break;
					default:
						printf("not support ebc pll sel\n");
						return -EINVAL;
					}

					div = DIV_ROUND_UP(pll_rate, rate);
					if (div > 255 || div == 0)
						continue;
					now = pll_rate / div;
					if (abs(rate - now) < abs(rate - best_rate)) {
						best_rate = now;
						best_div = div;
						best_sel = i;
					}
				}

				if (best_rate) {
					rk_clrsetreg(&cru->clksel_con[143],
						     DCLK_EBC_SRC_SEL_MASK,
						     best_sel <<
						     DCLK_EBC_SRC_SEL_SHIFT);
					rk_clrsetreg(&cru->clksel_con[144],
						     DCLK_EBC_SRC_DIV_MASK,
						     (best_div - 1) <<
						     DCLK_EBC_SRC_DIV_SHIFT);
				} else {
					printf("do not support this vop freq %lu\n",
					       rate);
					return -EINVAL;
				}
			}
		} else if (sel == DCLK_EBC_SEL_FRAC) {
			rk3572_dclk_ebc_set_clk(priv, DCLK_EBC_FRAC_SRC, rate);
			div = rk3572_dclk_ebc_get_clk(priv, DCLK_EBC_FRAC_SRC) / rate;
		}
		break;
	case DCLK_EBC_FRAC_SRC:
		sel = DCLK_EBC_SRC_SEL_GPLL;
		div = 1;
		rational_best_approximation(rate, priv->gpll_hz,
					    GENMASK(16 - 1, 0),
					    GENMASK(16 - 1, 0),
					    &m, &n);

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
		if (m && n) {
			val = m << CLK_UART_FRAC_NUMERATOR_SHIFT | n;
			writel(val, &cru->clksel_con[145]);
		}
		rk_clrsetreg(&cru->clksel_con[143],
			     DCLK_EBC_SRC_SEL_MASK,
			     (sel << DCLK_EBC_SRC_SEL_SHIFT));
		rk_clrsetreg(&cru->clksel_con[144],
			     DCLK_EBC_SRC_DIV_MASK,
			     (div - 1) <<
					     DCLK_EBC_SRC_DIV_SHIFT);
		break;
	default:
		return -ENOENT;
	}
	return rk3572_dclk_ebc_get_clk(priv, clk_id);
}

static ulong rk3572_gmac_get_clk(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 con, div, src, p_rate;

	switch (clk_id) {
	case CLK_GMAC0_PTP_REF_PRE:
		con = readl(&cru->clksel_con[98]);
		div = (con & CLK_GMAC0_PTP_PRE_DIV_MASK) >> CLK_GMAC0_PTP_PRE_DIV_SHIFT;
		src = (con & CLK_GMAC0_PTP_PRE_SEL_MASK) >> CLK_GMAC0_PTP_PRE_SEL_SHIFT;
		if (src == CLK_GMAC0_PTP_PRE_SEL_GPLL)
			p_rate = priv->gpll_hz;
		else
			p_rate = priv->cpll_hz;
		return DIV_TO_RATE(p_rate, div);
	case CLK_GMAC0_PTP_REF_SRC:
		con = readl(&cru->clksel_con[98]);
		src = (con & CLK_GMAC0_PTP_SEL_MASK) >> CLK_GMAC0_PTP_SEL_SHIFT;
		if (src == CLK_GMAC0_PTP_SEL_PRE)
			p_rate = rk3572_gmac_get_clk(priv, CLK_GMAC0_PTP_REF_PRE);
		else
			p_rate = GMAC0_PTP_REFCLK_IN;
		return p_rate;
	case CLK_GMAC1_PTP_REF_SRC:
		con = readl(&cru->clksel_con[114]);
		div = (con & CLK_GMAC1_PTP_SRC_DIV_MASK) >> CLK_GMAC1_PTP_SRC_DIV_SHIFT;
		src = (con & CLK_GMAC1_PTP_SRC_SEL_MASK) >> CLK_GMAC1_PTP_SRC_SEL_SHIFT;
		if (src == CLK_GMAC1_PTP_SRC_SEL_GPLL)
			p_rate = priv->gpll_hz;
		else if (src == CLK_GMAC1_PTP_SRC_SEL_CPLL)
			p_rate = priv->cpll_hz;
		return DIV_TO_RATE(p_rate, div);
	case CLK_GMAC1_PTP_REF:
		con = readl(&cru->clksel_con[115]);
		src = (con & CLK_GMAC1_PTP_SEL_MASK) >> CLK_GMAC1_PTP_SEL_SHIFT;
		if (src == CLK_GMAC1_PTP_SEL_SRC)
			p_rate = rk3572_gmac_get_clk(priv, CLK_GMAC1_PTP_REF_SRC);
		else
			p_rate = GMAC1_PTP_REFCLK_IN;
		return p_rate;
	case CLK_GMAC0_125M_SRC:
		con = readl(&cru->clksel_con[33]);
		div = (con & CLK_GMAC0_125M_DIV_MASK) >> CLK_GMAC0_125M_DIV_SHIFT;
		return DIV_TO_RATE(priv->cpll_hz, div);
	case CLK_GMAC1_125M_SRC:
		con = readl(&cru->clksel_con[34]);
		div = (con & CLK_GMAC1_125M_DIV_MASK) >> CLK_GMAC1_125M_DIV_SHIFT;
		return DIV_TO_RATE(priv->cpll_hz, div);
	default:
		return -ENOENT;
	}
}

static ulong rk3572_gmac_set_clk(struct rk3572_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	int div, src;

	div = DIV_ROUND_UP(priv->cpll_hz, rate);

	switch (clk_id) {
	case CLK_GMAC0_PTP_REF_PRE:
		if (!(priv->gpll_hz % rate)) {
			src = CLK_GMAC0_PTP_PRE_SEL_GPLL;
			div = priv->gpll_hz / rate;
		} else {
			src = CLK_GMAC0_PTP_PRE_SEL_CPLL;
			div = priv->cpll_hz / rate;
		}
		rk_clrsetreg(&cru->clksel_con[98],
			     CLK_GMAC0_PTP_PRE_DIV_MASK | CLK_GMAC0_PTP_PRE_SEL_MASK,
			     src << CLK_GMAC0_PTP_PRE_SEL_SHIFT |
			     (div - 1) << CLK_GMAC0_PTP_PRE_DIV_SHIFT);
	case CLK_GMAC0_PTP_REF_SRC:
		if (rate == GMAC0_PTP_REFCLK_IN) {
			src = CLK_GMAC0_PTP_SEL_IOIN;
		} else {
			rk3572_gmac_set_clk(priv, CLK_GMAC0_PTP_REF_PRE, rate);
			src = CLK_GMAC0_PTP_SEL_PRE;
		}
		rk_clrsetreg(&cru->clksel_con[98],
			     CLK_GMAC0_PTP_SEL_MASK,
			     src << CLK_GMAC0_PTP_SEL_SHIFT);
		break;
	case CLK_GMAC1_PTP_REF_SRC:
		if (!(priv->gpll_hz % rate)) {
			src = CLK_GMAC1_PTP_SRC_SEL_GPLL;
			div = priv->gpll_hz / rate;
		} else {
			src = CLK_GMAC1_PTP_SRC_SEL_CPLL;
			div = priv->cpll_hz / rate;
		}
		rk_clrsetreg(&cru->clksel_con[114],
			     CLK_GMAC1_PTP_SRC_DIV_MASK | CLK_GMAC1_PTP_SRC_SEL_MASK,
			     src << CLK_GMAC1_PTP_SRC_SEL_SHIFT |
			     (div - 1) << CLK_GMAC1_PTP_SRC_DIV_SHIFT);
		break;
	case CLK_GMAC1_PTP_REF:
		if (rate == GMAC1_PTP_REFCLK_IN) {
			src = CLK_GMAC1_PTP_SEL_IOIN;
		} else {
			rk3572_gmac_set_clk(priv, CLK_GMAC1_PTP_REF_SRC, rate);
			src = CLK_GMAC1_PTP_SEL_SRC;
		}
		rk_clrsetreg(&cru->clksel_con[115],
			     CLK_GMAC1_PTP_SEL_MASK,
			     src << CLK_GMAC1_PTP_SEL_SHIFT);
		break;

	case CLK_GMAC0_125M_SRC:
		rk_clrsetreg(&cru->clksel_con[33],
			     CLK_GMAC0_125M_DIV_MASK,
			     (div - 1) << CLK_GMAC0_125M_DIV_SHIFT);
		break;
	case CLK_GMAC1_125M_SRC:
		rk_clrsetreg(&cru->clksel_con[34],
			     CLK_GMAC1_125M_DIV_MASK,
			     (div - 1) << CLK_GMAC1_125M_DIV_SHIFT);
		break;
	default:
		return -ENOENT;
	}

	return rk3572_gmac_get_clk(priv, clk_id);
}

static ulong rk3572_uart_frac_get_rate(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 reg, con, fracdiv, p_src, p_rate;
	unsigned long m, n;

	switch (clk_id) {
	case CLK_UART_FRAC_0:
		reg = 22;
		break;
	case CLK_UART_FRAC_1:
		reg = 24;
		break;
	case CLK_UART_FRAC_2:
		reg = 26;
		break;
	default:
		return -ENOENT;
	}
	con = readl(&cru->clksel_con[reg + 1]);
	p_src = (con & CLK_UART_SRC_SEL_MASK) >> CLK_UART_SRC_SEL_SHIFT;
	if (p_src == CLK_UART_SRC_SEL_GPLL)
		p_rate = priv->gpll_hz;
	else if (p_src == CLK_UART_SRC_SEL_CPLL)
		p_rate = priv->cpll_hz;
	else if (p_src == CLK_UART_SRC_SEL_AUPLL)
		p_rate = priv->aupll_hz;
	else
		p_rate = OSC_HZ;

	fracdiv = readl(&cru->clksel_con[reg]);
	n = fracdiv & CLK_UART_FRAC_NUMERATOR_MASK;
	n >>= CLK_UART_FRAC_NUMERATOR_SHIFT;
	m = fracdiv & CLK_UART_FRAC_DENOMINATOR_MASK;
	m >>= CLK_UART_FRAC_DENOMINATOR_SHIFT;
	return p_rate * n / m;
}

static ulong rk3572_uart_frac_set_rate(struct rk3572_clk_priv *priv,
				       ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	u32 reg, clk_src, p_rate;
	unsigned long m = 0, n = 0, val;

	if (priv->cpll_hz % rate == 0) {
		clk_src = CLK_UART_SRC_SEL_CPLL;
		p_rate = priv->cpll_hz;
	} else if (rate == OSC_HZ) {
		clk_src = CLK_UART_SRC_SEL_OSC;
		p_rate = OSC_HZ;
	} else {
		clk_src = CLK_UART_SRC_SEL_GPLL;
		p_rate = priv->cpll_hz;
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
		reg = 22;
		break;
	case CLK_UART_FRAC_1:
		reg = 24;
		break;
	case CLK_UART_FRAC_2:
		reg = 26;
		break;
	default:
		return -ENOENT;
	}

	rk_clrsetreg(&cru->clksel_con[reg + 1],
		     CLK_UART_SRC_SEL_MASK,
		     (clk_src << CLK_UART_SRC_SEL_SHIFT));
	if (m && n) {
		val = m << CLK_UART_FRAC_NUMERATOR_SHIFT | n;
		writel(val, &cru->clksel_con[reg]);
	}

	return rk3572_uart_frac_get_rate(priv, clk_id);
}

static ulong rk3572_uart_get_rate(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 con, div, src, p_rate;

	switch (clk_id) {
	case SCLK_UART0:
		con = readl(&cru->clksel_con[71]);
		break;
	case SCLK_UART1:
		con = readl(&cru->pmuclksel_con[8]);
		src = (con & CLK_UART1_SEL_MASK) >> CLK_UART1_SEL_SHIFT;
		if (src == CLK_UART1_SEL_OSC)
			return OSC_HZ;
		con = readl(&cru->clksel_con[29]);
		break;
	case SCLK_UART2:
		con = readl(&cru->clksel_con[72]);
		break;
	case SCLK_UART3:
		con = readl(&cru->clksel_con[73]);
		break;
	case SCLK_UART4:
		con = readl(&cru->clksel_con[74]);
		break;
	case SCLK_UART5:
		con = readl(&cru->clksel_con[75]);
		break;
	case SCLK_UART6:
		con = readl(&cru->clksel_con[76]);
		break;
	case SCLK_UART7:
		con = readl(&cru->clksel_con[77]);
		break;
	case SCLK_UART8:
		con = readl(&cru->clksel_con[78]);
		break;
	case SCLK_UART9:
		con = readl(&cru->clksel_con[79]);
		break;
	case SCLK_UART10:
		con = readl(&cru->clksel_con[80]);
		break;
	case SCLK_UART11:
		con = readl(&cru->clksel_con[81]);
		break;
	default:
		return -ENOENT;
	}

	src = (con & CLK_UART_SEL_MASK) >> CLK_UART_SEL_SHIFT;
	div = (con & CLK_UART_DIV_MASK) >> CLK_UART_DIV_SHIFT;
	if (src == CLK_UART_SEL_GPLL)
		p_rate = priv->gpll_hz;
	else  if (src == CLK_UART_SEL_CPLL)
		p_rate = priv->cpll_hz;
	else  if (src == CLK_UART_SEL_AUPLL)
		p_rate = priv->aupll_hz;
	else  if (src == CLK_UART_SEL_FRAC0)
		p_rate = rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_0);
	else  if (src == CLK_UART_SEL_FRAC1)
		p_rate = rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_1);
	else  if (src == CLK_UART_SEL_FRAC2)
		p_rate = rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_2);
	else
		p_rate = OSC_HZ;

	return DIV_TO_RATE(p_rate, div);
}

static ulong rk3572_uart_set_rate(struct rk3572_clk_priv *priv,
				  ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	u32 reg, clk_src = 0, div = 0;

	if (!(priv->gpll_hz % rate)) {
		clk_src = CLK_UART_SEL_GPLL;
		div = DIV_ROUND_UP(priv->gpll_hz, rate);
	} else if (!(priv->cpll_hz % rate)) {
		clk_src = CLK_UART_SEL_CPLL;
		div = DIV_ROUND_UP(priv->gpll_hz, rate);
	} else if (!(rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_0) % rate)) {
		clk_src = CLK_UART_SEL_FRAC0;
		div = DIV_ROUND_UP(rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_0), rate);
	} else if (!(rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_1) % rate)) {
		clk_src = CLK_UART_SEL_FRAC1;
		div = DIV_ROUND_UP(rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_1), rate);
	} else if (!(rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_2) % rate)) {
		clk_src = CLK_UART_SEL_FRAC2;
		div = DIV_ROUND_UP(rk3572_uart_frac_get_rate(priv, CLK_UART_FRAC_2), rate);
	} else if (!(OSC_HZ % rate)) {
		clk_src = CLK_UART_SEL_OSC;
		div = DIV_ROUND_UP(OSC_HZ, rate);
	}

	switch (clk_id) {
	case SCLK_UART0:
		reg = 71;
		break;
	case SCLK_UART1:
		if (rate == OSC_HZ) {
			rk_clrsetreg(&cru->pmuclksel_con[8],
				     CLK_UART1_SEL_MASK,
				     CLK_UART1_SEL_OSC << CLK_UART1_SEL_SHIFT);
			return 0;
		}

		reg = 29;
		rk_clrsetreg(&cru->pmuclksel_con[8],
			     CLK_UART1_SEL_MASK,
			     CLK_UART1_SEL_TOP << CLK_UART1_SEL_SHIFT);
		return 0;
	case SCLK_UART2:
		reg = 72;
		break;
	case SCLK_UART3:
		reg = 73;
		break;
	case SCLK_UART4:
		reg = 74;
		break;
	case SCLK_UART5:
		reg = 75;
		break;
	case SCLK_UART6:
		reg = 76;
		break;
	case SCLK_UART7:
		reg = 77;
		break;
	case SCLK_UART8:
		reg = 78;
		break;
	case SCLK_UART9:
		reg = 79;
		break;
	case SCLK_UART10:
		reg = 80;
		break;
	case SCLK_UART11:
		reg = 81;
		break;
	default:
		return -ENOENT;
	}

	rk_clrsetreg(&cru->clksel_con[reg],
		     CLK_UART_SEL_MASK |
		     CLK_UART_DIV_MASK,
		     (clk_src << CLK_UART_SEL_SHIFT) |
		     ((div - 1) << CLK_UART_DIV_SHIFT));

	return rk3572_uart_get_rate(priv, clk_id);
}

static ulong rk3572_ref_clkout_get_clk(struct rk3572_clk_priv *priv,
				       ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 reg, con, div, src, p_rate;

	switch (clk_id) {
	case REF_CLK0_OUT_PLL:
		reg = 38;
		break;
	case REF_CLK1_OUT_PLL:
		reg = 39;
		break;
	case REF_CLK2_OUT_PLL:
		reg = 40;
		break;
	default:
		return -ENOENT;
	}
	con = readl(&cru->clksel_con[reg]);
	div = (con & REF_CLK0_OUT_PLL_DIV_MASK) >> REF_CLK0_OUT_PLL_DIV_SHIFT;
	src = (con & REF_CLK0_OUT_PLL_SEL_MASK) >> REF_CLK0_OUT_PLL_SEL_SHIFT;
	if (src == REF_CLK0_OUT_PLL_SEL_GPLL)
		p_rate = priv->gpll_hz;
	else if (src == REF_CLK0_OUT_PLL_SEL_CPLL)
		p_rate = priv->cpll_hz;
	else if (src == REF_CLK0_OUT_PLL_SEL_SPLL)
		p_rate = priv->spll_hz;
	else if (src == REF_CLK0_OUT_PLL_SEL_AUPLL)
		p_rate = priv->aupll_hz;
	else
		p_rate = OSC_HZ;
	return DIV_TO_RATE(p_rate, div);
}

static ulong rk3572_ref_clkout_set_clk(struct rk3572_clk_priv *priv,
				       ulong clk_id, ulong rate)
{
	struct rk3572_cru *cru = priv->cru;
	ulong p_rate, now, best_rate = 0;
	u32 i, con, div, best_div = 0, best_sel = 0;

	switch (clk_id) {
	case REF_CLK0_OUT_PLL:
		con = 38;
		break;
	case REF_CLK1_OUT_PLL:
		con = 39;
		break;
	case REF_CLK2_OUT_PLL:
		con = 40;
		break;
	default:
		return -ENOENT;
	}

	for (i = 0; i <= REF_CLK0_OUT_PLL_SEL_OSC; i++) {
		switch (i) {
		case REF_CLK0_OUT_PLL_SEL_GPLL:
			p_rate = priv->gpll_hz;
			break;
		case REF_CLK0_OUT_PLL_SEL_CPLL:
			p_rate = priv->cpll_hz;
			break;
		case REF_CLK0_OUT_PLL_SEL_SPLL:
			p_rate = priv->spll_hz;
			break;
		case REF_CLK0_OUT_PLL_SEL_AUPLL:
			p_rate = priv->aupll_hz;
			break;
		case REF_CLK0_OUT_PLL_SEL_BPLL:
			p_rate = 0;
			break;
		case REF_CLK0_OUT_PLL_SEL_OSC:
			p_rate = OSC_HZ;
			break;
		default:
			printf("do not support this vop pll sel\n");
			return -EINVAL;
		}

		div = DIV_ROUND_UP(p_rate, rate);
		if (div > 255)
			continue;
		now = p_rate / div;
		if (abs(rate - now) < abs(rate - best_rate)) {
			best_rate = now;
			best_div = div;
			best_sel = i;
		}
		debug("p_rate=%lu, best_rate=%lu, div=%u, sel=%u\n",
		      p_rate, best_rate, best_div, best_sel);
	}
	if (best_rate) {
		rk_clrsetreg(&cru->clksel_con[con],
			     REF_CLK0_OUT_PLL_DIV_MASK |
			     REF_CLK0_OUT_PLL_SEL_MASK,
			     best_sel << REF_CLK0_OUT_PLL_SEL_SHIFT |
			     (best_div - 1) << REF_CLK0_OUT_PLL_DIV_SHIFT);
	} else {
		printf("do not support this vop freq %lu\n", rate);
		return -EINVAL;
	}

	return rk3572_ref_clkout_get_clk(priv, clk_id);
}

#endif

static ulong rk3572_ufs_ref_get_rate(struct rk3572_clk_priv *priv, ulong clk_id)
{
	struct rk3572_cru *cru = priv->cru;
	u32 src, div;

	src = readl(&cru->pmuclksel_con[3]) & 0x1;
	div = readl(&cru->phpclksel_con[1]) & 0xff;
	if (src == 1)
		return priv->ppll_hz / (div + 1);
	else
		return OSC_HZ;
}

static ulong rk3572_clk_get_rate(struct clk *clk)
{
	struct rk3572_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	if (!priv->gpll_hz) {
		printf("%s gpll=%lu\n", __func__, priv->gpll_hz);
		return -ENOENT;
	}

	if (!priv->ppll_hz) {
		priv->ppll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[PPLL],
						      priv->cru, PPLL);
	}

	switch (clk->id) {
	case PLL_LPLL:
		rate = rockchip_pll_get_rate(&rk3572_pll_clks[LPLL], priv->cru,
					     LPLL);
		priv->lpll_hz = rate;
		break;
	case PLL_BPLL:
		rate = rockchip_pll_get_rate(&rk3572_pll_clks[BPLL], priv->cru,
					     BPLL);
		priv->bpll_hz = rate;
		break;
	case PLL_GPLL:
		rate = rockchip_pll_get_rate(&rk3572_pll_clks[GPLL], priv->cru,
					     GPLL);
		break;
	case PLL_CPLL:
		rate = rockchip_pll_get_rate(&rk3572_pll_clks[CPLL], priv->cru,
					     CPLL);
		break;
	case PLL_VPLL:
		rate = rockchip_pll_get_rate(&rk3572_pll_clks[VPLL], priv->cru,
					     VPLL);
		break;
	case PLL_AUPLL:
		rate = rockchip_pll_get_rate(&rk3572_pll_clks[AUPLL], priv->cru,
					     AUPLL);
		break;
	case PLL_PPLL:
		rate = rockchip_pll_get_rate(&rk3572_pll_clks[PPLL], priv->cru,
					     PPLL) * 2;
		break;
	case ACLK_BUS_ROOT:
	case HCLK_BUS_ROOT:
	case PCLK_BUS_ROOT:
		rate = rk3572_bus_get_clk(priv, clk->id);
		break;
	case ACLK_TOP:
	case HCLK_TOP:
	case PCLK_TOP_ROOT:
	case ACLK_TOP_MID:
		rate = rk3572_top_get_clk(priv, clk->id);
		break;
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C2:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
	case CLK_I2C6:
	case CLK_I2C7:
	case CLK_I2C8:
	case CLK_I2C9:
		rate = rk3572_i2c_get_clk(priv, clk->id);
		break;
	case CLK_SPI0:
	case CLK_SPI1:
	case CLK_SPI2:
	case CLK_SPI3:
	case CLK_SPI4:
		rate = rk3572_spi_get_clk(priv, clk->id);
		break;
	case CLK_PWM1:
	case CLK_PWM2:
	case CLK_PWM0:
		rate = rk3572_pwm_get_clk(priv, clk->id);
		break;
	case CLK_SARADC:
	case CLK_TSADC:
		rate = rk3572_adc_get_clk(priv, clk->id);
		break;
	case CCLK_SRC_SDMMC0:
	case CCLK_SRC_SDMMC1:
	case CCLK_SRC_EMMC:
	case HCLK_SDMMC0:
	case HCLK_SDMMC1:
	case HCLK_EMMC:
	case BCLK_EMMC:
	case SCLK_FSPI0_X2:
	case SCLK_FSPI1_X2:
	case DCLK_DECOM:
		rate = rk3572_mmc_get_clk(priv, clk->id);
		break;
	case TCLK_EMMC:
	case TCLK_WDT0:
		rate = OSC_HZ;
		break;
#ifndef CONFIG_SPL_BUILD
	case ACLK_VOP_ROOT:
	case ACLK_VOP:
	case ACLK_VO_ROOT:
	case HCLK_VOP_ROOT:
	case PCLK_VOP_ROOT:
		rate = rk3572_aclk_vop_get_clk(priv, clk->id);
		break;
	case DCLK_VP0:
	case DCLK_VP0_SRC:
	case DCLK_VP1:
	case DCLK_VP1_SRC:
		rate = rk3572_dclk_vop_get_clk(priv, clk->id);
		break;
	case CLK_GMAC0_PTP_REF_PRE:
	case CLK_GMAC0_PTP_REF_SRC:
	case CLK_GMAC1_PTP_REF_SRC:
	case CLK_GMAC1_PTP_REF:
	case CLK_GMAC0_125M_SRC:
	case CLK_GMAC1_125M_SRC:
		rate = rk3572_gmac_get_clk(priv, clk->id);
		break;
	case CLK_UART_FRAC_0:
	case CLK_UART_FRAC_1:
	case CLK_UART_FRAC_2:
		rate = rk3572_uart_frac_get_rate(priv, clk->id);
		break;
	case SCLK_UART0:
	case SCLK_UART1:
	case SCLK_UART2:
	case SCLK_UART3:
	case SCLK_UART4:
	case SCLK_UART5:
	case SCLK_UART6:
	case SCLK_UART7:
	case SCLK_UART8:
	case SCLK_UART9:
	case SCLK_UART10:
	case SCLK_UART11:
		rate = rk3572_uart_get_rate(priv, clk->id);
		break;
	case DCLK_EBC:
	case DCLK_EBC_FRAC_SRC:
		rate = rk3572_dclk_ebc_get_clk(priv, clk->id);
		break;
	case REF_CLK0_OUT_PLL:
	case REF_CLK1_OUT_PLL:
	case REF_CLK2_OUT_PLL:
		rate = rk3572_ref_clkout_get_clk(priv, clk->id);
		break;
#endif
	case CLK_REF_UFS_CLKOUT:
	case CLK_REF_OSC_MPHY:
		rate = rk3572_ufs_ref_get_rate(priv, clk->id);
		break;

	default:
		return -ENOENT;
	}

	return rate;
};

static ulong rk3572_clk_set_rate(struct clk *clk, ulong rate)
{
	struct rk3572_clk_priv *priv = dev_get_priv(clk->dev);
	ulong ret = 0;

	if (!priv->ppll_hz) {
		priv->ppll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[PPLL],
						      priv->cru, PPLL);
	}
	if (!priv->aupll_hz) {
		priv->aupll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[AUPLL],
						       priv->cru, AUPLL);
	}

	switch (clk->id) {
	case PLL_CPLL:
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[CPLL], priv->cru,
					    CPLL, rate);
		priv->cpll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[CPLL],
						      priv->cru, CPLL);
		break;
	case PLL_GPLL:
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[GPLL], priv->cru,
					    GPLL, rate);
		priv->gpll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[GPLL],
						      priv->cru, GPLL);
		break;
	case PLL_VPLL:
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[VPLL], priv->cru,
					    VPLL, rate);
		priv->vpll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[VPLL],
						      priv->cru, VPLL);
		break;
	case PLL_AUPLL:
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[AUPLL], priv->cru,
					    AUPLL, rate);
		priv->aupll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[AUPLL],
						       priv->cru, AUPLL);
		break;
	case PLL_PPLL:
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[PPLL], priv->cru,
					    PPLL, rate);
		priv->ppll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[PPLL],
						      priv->cru, PPLL) * 2;
		break;
	case ACLK_BUS_ROOT:
	case HCLK_BUS_ROOT:
	case PCLK_BUS_ROOT:
		ret = rk3572_bus_set_clk(priv, clk->id, rate);
		break;
	case ACLK_TOP:
	case HCLK_TOP:
	case PCLK_TOP_ROOT:
	case ACLK_TOP_MID:
		ret = rk3572_top_set_clk(priv, clk->id, rate);
		break;
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C2:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
	case CLK_I2C6:
	case CLK_I2C7:
	case CLK_I2C8:
	case CLK_I2C9:
		ret = rk3572_i2c_set_clk(priv, clk->id, rate);
		break;
	case CLK_SPI0:
	case CLK_SPI1:
	case CLK_SPI2:
	case CLK_SPI3:
	case CLK_SPI4:
		ret = rk3572_spi_set_clk(priv, clk->id, rate);
		break;
	case CLK_PWM1:
	case CLK_PWM2:
	case CLK_PWM0:
		ret = rk3572_pwm_set_clk(priv, clk->id, rate);
		break;
	case CLK_SARADC:
	case CLK_TSADC:
		ret = rk3572_adc_set_clk(priv, clk->id, rate);
		break;
	case CCLK_SRC_SDMMC0:
	case CCLK_SRC_SDMMC1:
	case CCLK_SRC_EMMC:
	case HCLK_SDMMC0:
	case HCLK_SDMMC1:
	case HCLK_EMMC:
	case BCLK_EMMC:
	case SCLK_FSPI0_X2:
	case SCLK_FSPI1_X2:
	case DCLK_DECOM:
		ret = rk3572_mmc_set_clk(priv, clk->id, rate);
		break;
	case TCLK_EMMC:
	case TCLK_WDT0:
		ret = OSC_HZ;
		break;

	/* Might occur in cru assigned-clocks, can be ignored here */
	case CLK_AUDIO_FRAC_0:
	case CLK_AUDIO_FRAC_1:
	case CLK_AUDIO_FRAC_0_SRC:
	case CLK_AUDIO_FRAC_1_SRC:
	case CLK_CPLL_DIV2:
	case CLK_CPLL_DIV4:
	case CLK_CPLL_DIV10:
		ret = 0;
		break;
#ifndef CONFIG_SPL_BUILD
	case ACLK_VOP_ROOT:
	case ACLK_VOP:
	case ACLK_VO_ROOT:
	case HCLK_VOP_ROOT:
	case PCLK_VOP_ROOT:
		ret = rk3572_aclk_vop_set_clk(priv, clk->id, rate);
		break;
	case DCLK_VP0:
	case DCLK_VP0_SRC:
	case DCLK_VP1:
	case DCLK_VP1_SRC:
		ret = rk3572_dclk_vop_set_clk(priv, clk->id, rate);
		break;
	case CLK_GMAC0_PTP_REF_PRE:
	case CLK_GMAC0_PTP_REF_SRC:
	case CLK_GMAC1_PTP_REF_SRC:
	case CLK_GMAC1_PTP_REF:
	case CLK_GMAC0_125M_SRC:
	case CLK_GMAC1_125M_SRC:
		ret = rk3572_gmac_set_clk(priv, clk->id, rate);
		break;
	case CLK_UART_FRAC_0:
	case CLK_UART_FRAC_1:
	case CLK_UART_FRAC_2:
		ret = rk3572_uart_frac_set_rate(priv, clk->id, rate);
		break;
	case SCLK_UART0:
	case SCLK_UART1:
	case SCLK_UART2:
	case SCLK_UART3:
	case SCLK_UART4:
	case SCLK_UART5:
	case SCLK_UART6:
	case SCLK_UART7:
	case SCLK_UART8:
	case SCLK_UART9:
	case SCLK_UART10:
	case SCLK_UART11:
		ret = rk3572_uart_set_rate(priv, clk->id, rate);
		break;
	case DCLK_EBC:
	case DCLK_EBC_FRAC_SRC:
		ret = rk3572_dclk_ebc_set_clk(priv, clk->id, rate);
		break;
	case REF_CLK0_OUT_PLL:
	case REF_CLK1_OUT_PLL:
	case REF_CLK2_OUT_PLL:
		ret = rk3572_ref_clkout_set_clk(priv, clk->id, rate);
		break;
#endif
	default:
		return -ENOENT;
	}

	return ret;
};

#if (IS_ENABLED(OF_CONTROL)) || (!IS_ENABLED(OF_PLATDATA))
static int __maybe_unused rk3572_dclk_vop_set_parent(struct clk *clk,
						     struct clk *parent)
{
	struct rk3572_clk_priv *priv = dev_get_priv(clk->dev);
	struct rk3572_cru *cru = priv->cru;
	u32 sel;
	const char *clock_dev_name = parent->dev->name;

	if (parent->id == PLL_VPLL)
		sel = 2;
	else if (parent->id == PLL_CPLL)
		sel = 1;
	else if (parent->id == PLL_BPLL)
		sel = 3;
	else
		sel = 0;

	switch (clk->id) {
	case DCLK_VP0_SRC:
		rk_clrsetreg(&cru->clksel_con[175], DCLK0_VOP_SRC_SEL_MASK,
			     sel << DCLK0_VOP_SRC_SEL_SHIFT);
		break;
	case DCLK_VP1_SRC:
		rk_clrsetreg(&cru->clksel_con[176], DCLK0_VOP_SRC_SEL_MASK,
			     sel << DCLK0_VOP_SRC_SEL_SHIFT);
		break;
	case DCLK_VP0:
		if (!strcmp(clock_dev_name, "hdmiphypll_clk0"))
			sel = 1;
		else
			sel = 0;
		rk_clrsetreg(&cru->clksel_con[177], DCLK0_VOP_SEL_MASK,
			     sel << DCLK0_VOP_SEL_SHIFT);
		break;
	case DCLK_VP1:
		if (!strcmp(clock_dev_name, "hdmiphypll_clk0"))
			sel = 1;
		else
			sel = 0;
		rk_clrsetreg(&cru->clksel_con[177], DCLK1_VOP_SEL_MASK,
			     sel << DCLK1_VOP_SEL_SHIFT);
		break;
	case DCLK_EBC_SRC_TOP:
		if (parent->id == PLL_CPLL)
			sel = 1;
		else if (parent->id == PLL_VPLL)
			sel = 2;
		else if (parent->id == PLL_AUPLL)
			sel = 3;
		else if (parent->id == PLL_BPLL)
			sel = 4;
		else
			sel = 0;
		rk_clrsetreg(&cru->clksel_con[143], DCLK_EBC_SRC_SEL_MASK,
			     sel << DCLK_EBC_SRC_SEL_SHIFT);
		break;
	case DCLK_EBC:
		if (parent->id == DCLK_EBC_FRAC_SRC)
			sel = 1;
		else
			sel = 0;
		rk_clrsetreg(&cru->clksel_con[146], DCLK_EBC_SEL_MASK,
			     sel << DCLK_EBC_SEL_SHIFT);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int __maybe_unused rk3572_ufs_ref_set_parent(struct clk *clk,
						    struct clk *parent)
{
	struct rk3572_clk_priv *priv = dev_get_priv(clk->dev);
	struct rk3572_cru *cru = priv->cru;
	u32 sel;

	if (parent->id == CLK_REF_MPHY_26M)
		sel = 1;
	else
		sel = 0;

	rk_clrsetreg(&cru->pmuclksel_con[3], 0x1, sel << 0);
	return 0;
}

static int rk3572_clk_set_parent(struct clk *clk, struct clk *parent)
{
	switch (clk->id) {
	case DCLK_VP0_SRC:
	case DCLK_VP1_SRC:
	case DCLK_VP0:
	case DCLK_VP1:
	case DCLK_EBC_SRC_TOP:
	case DCLK_EBC:
		return rk3572_dclk_vop_set_parent(clk, parent);
	case CLK_REF_OSC_MPHY:
		return rk3572_ufs_ref_set_parent(clk, parent);
	case CLK_AUDIO_FRAC_0_SRC:
	case CLK_AUDIO_FRAC_1_SRC:
		/* Might occur in cru assigned-clocks, can be ignored here */
		return 0;
	default:
		return -ENOENT;
	}

	return 0;
}
#endif

static struct clk_ops rk3572_clk_ops = {
	.get_rate = rk3572_clk_get_rate,
	.set_rate = rk3572_clk_set_rate,
#if (IS_ENABLED(OF_CONTROL)) || (!IS_ENABLED(OF_PLATDATA))
	.set_parent = rk3572_clk_set_parent,
#endif
};

static void rk3572_clk_init(struct rk3572_clk_priv *priv)
{
	int ret;

	priv->spll_hz = 702000000;

	if (priv->cpll_hz != CPLL_HZ) {
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[CPLL], priv->cru,
					    CPLL, CPLL_HZ);
		if (!ret)
			priv->cpll_hz = CPLL_HZ;
	}
	priv->gpll_hz = rockchip_pll_get_rate(&rk3572_pll_clks[GPLL], priv->cru, GPLL);
	if (priv->aupll_hz != AUPLL_HZ) {
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[AUPLL], priv->cru,
					    AUPLL, AUPLL_HZ);
		if (!ret)
			priv->aupll_hz = AUPLL_HZ;
	}
}

static int rk3572_clk_probe(struct udevice *dev)
{
	struct rk3572_clk_priv *priv = dev_get_priv(dev);
	int ret;
#if CONFIG_IS_ENABLED(CLK_SCMI)
	struct clk clk;
#endif

	priv->sync_kernel = false;

#ifdef CONFIG_SPL_BUILD
	/* relase bigcore */
	writel(BITS_WITH_WMASK(0, 0x3U, 0),
	       RK3572_CRU_BASE + RK3572_CCI2LITCORE1_RSTCON);

	/* set noc timeout 27*217*/
	writel(BITS_WITH_WMASK(0x1a, 0xffU, 4),
	       RK3572_CRU_BASE + RK3572_CLKSEL_CON(19));
	writel(BITS_WITH_WMASK(0xd8, 0x3ffU, 6),
	       RK3572_CRU_BASE + RK3572_CLKSEL_CON(20));

	/* set spll to normal mode */
	writel(BITS_WITH_WMASK(0, 0x3U, 0),
	       RK3572_SCRU_BASE + RK3572_MODE_CON0);
	writel(BITS_WITH_WMASK(2, 0x7U, 6),
	       RK3572_SCRU_BASE + RK3572_PLL_CON(137));
	writel(BITS_WITH_WMASK(1, 0x3U, 0),
	       RK3572_SCRU_BASE + RK3572_MODE_CON0);

	/* slow mode */
	writel(BITS_WITH_WMASK(0, 0xffffU, 0),
	       RK3572_CRU_BASE + RK3572_MODE_CON0);

	/* fix ppll\aupll\cpll\gpll */
	writel(BITS_WITH_WMASK(2, 0x7U, 6),
	       RK3572_CRU_BASE + RK3572_PPLL_CON(129));
	writel(BITS_WITH_WMASK(2, 0x7U, 6),
	       RK3572_CRU_BASE + RK3572_PLL_CON(97));
	writel(BITS_WITH_WMASK(2, 0x7U, 6),
	       RK3572_CRU_BASE + RK3572_PLL_CON(105));
	writel(BITS_WITH_WMASK(1, 0x7U, 6),
	       RK3572_CRU_BASE + RK3572_PLL_CON(113));

	/* fix div */
	writel(BITS_WITH_WMASK(9, 0x1fU, 6),
	       RK3572_CRU_BASE + RK3572_CLKSEL_CON(0));
	writel(BITS_WITH_WMASK(3, 0x1fU, 0),
	       RK3572_CRU_BASE + RK3572_CLKSEL_CON(2));
	writel(BITS_WITH_WMASK(1, 0x1fU, 6),
	       RK3572_CRU_BASE + RK3572_CLKSEL_CON(4));

	/* normal mode */
	writel(BITS_WITH_WMASK(0x5555, 0xffffU, 0),
	       RK3572_CRU_BASE + RK3572_MODE_CON0);
	/* init cci gpll/2 */
	writel(BITS_WITH_WMASK(1, 0x3U, 12),
	       RK3572_CRU_BASE + RK3572_CCI_CLKSEL_CON(3));
	/* init litcore1 lpll/1 */
	if (!priv->armclk_enter_hz) {
		ret = rockchip_pll_set_rate(&rk3572_pll_clks[LPLL], priv->cru,
					    LPLL, LPLL_HZ);
		priv->armclk_enter_hz =
			rockchip_pll_get_rate(&rk3572_pll_clks[LPLL],
					      priv->cru, LPLL);
		priv->armclk_init_hz = priv->armclk_enter_hz;
		writel(BITS_WITH_WMASK(0, 0x3U, 8),
		       RK3572_CRU_BASE + RK3572_LITCORE0_CLKSEL_CON(0));
		writel(BITS_WITH_WMASK(0, 0x1fU, 3),
		       RK3572_CRU_BASE + RK3572_LITCORE0_CLKSEL_CON(0));
		writel(BITS_WITH_WMASK(0, 0x3U, 8),
		       RK3572_CRU_BASE + RK3572_LITCORE1_CLKSEL_CON(0));
		writel(BITS_WITH_WMASK(0, 0x1fU, 3),
		       RK3572_CRU_BASE + RK3572_LITCORE1_CLKSEL_CON(0));
	}
	/* init cci lpll/1 */
	writel(BITS_WITH_WMASK(0, 0x3U, 12),
	       RK3572_CRU_BASE + RK3572_CCI_CLKSEL_CON(3));
	writel(BITS_WITH_WMASK(0, 0x1fU, 7),
	       RK3572_CRU_BASE + RK3572_CCI_CLKSEL_CON(3));
	rockchip_pll_set_rate(&rk3572_pll_clks[BPLL], priv->cru,
			      BPLL, LPLL_HZ);
	writel(BITS_WITH_WMASK(0, 0x3U, 5),
	       RK3572_CRU_BASE + RK3572_BIGCORE_CLKSEL_CON(1));
	writel(BITS_WITH_WMASK(0, 0x1fU, 0),
	       RK3572_CRU_BASE + RK3572_BIGCORE_CLKSEL_CON(1));

	/* clk_extref_timeout_src_div = 256 */
	writel(BITS_WITH_WMASK(0xff, 0xffU, 4),
	       RK3572_CRU_BASE + RK3572_CLKSEL_CON(19));
	/* clk_extref_timeout_128div_div = 256 */
	writel(BITS_WITH_WMASK(0xff, 0x3ffU, 6),
	       RK3572_CRU_BASE + RK3572_CLKSEL_CON(20));
#endif

	rk3572_clk_init(priv);

#if CONFIG_IS_ENABLED(CLK_SCMI)
#ifndef CONFIG_SPL_BUILD
	ret = rockchip_get_scmi_clk(&clk.dev);
	if (ret) {
		printf("Failed to get scmi clk dev, ret=%d\n", ret);
		return ret;
	}
	if (!priv->armclk_enter_hz) {
		clk.id = ARMCLK_L0;
		ret = clk_set_rate(&clk, CPU_PVTPLL_HZ);
		if (ret < 0) {
			printf("Failed to set cpul0, ret=%d\n", ret);
		} else {
			priv->armclk_enter_hz = CPU_PVTPLL_HZ;
			priv->armclk_init_hz = CPU_PVTPLL_HZ;
		}
	}
	clk.id = ARMCLK_L1;
	ret = clk_set_rate(&clk, CPU_PVTPLL_HZ);
	if (ret < 0)
		printf("Failed to set cpul1, ret=%d\n", ret);
	clk.id = ARMCLK_B;
	ret = clk_set_rate(&clk, CPU_PVTPLL_HZ);
	if (ret < 0)
		printf("Failed to set cpub, ret=%d\n", ret);
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

static int rk3572_clk_ofdata_to_platdata(struct udevice *dev)
{
	struct rk3572_clk_priv *priv = dev_get_priv(dev);

	priv->cru = dev_read_addr_ptr(dev);

	return 0;
}

static int rk3572_clk_bind(struct udevice *dev)
{
	int ret;
	struct udevice *sys_child;
	struct sysreset_reg *priv;

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(dev, "rockchip_sysreset", "sysreset",
				 &sys_child);
	if (ret) {
		debug("Warning: No sysreset driver: ret=%d\n", ret);
	} else {
		priv = malloc(sizeof(struct sysreset_reg));
		priv->glb_srst_fst_value = offsetof(struct rk3572_cru,
						    glb_srst_fst);
		priv->glb_srst_snd_value = offsetof(struct rk3572_cru,
						    glb_srsr_snd);
		dev_set_priv(sys_child, priv);
	}

#if CONFIG_IS_ENABLED(RESET_ROCKCHIP)
	ret = offsetof(struct rk3572_cru, softrst_con[0]);
	ret = rk3572_reset_bind_lut(dev, ret, 90115);
	if (ret)
		debug("Warning: software reset driver bind failed\n");
#endif

	return 0;
}

static const struct udevice_id rk3572_clk_ids[] = {
	{ .compatible = "rockchip,rk3572-cru" },
	{ }
};

U_BOOT_DRIVER(rockchip_rk3572_cru) = {
	.name		= "rockchip_rk3572_cru",
	.id		= UCLASS_CLK,
	.of_match	= rk3572_clk_ids,
	.priv_auto	= sizeof(struct rk3572_clk_priv),
	.of_to_plat	= rk3572_clk_ofdata_to_platdata,
	.ops		= &rk3572_clk_ops,
	.bind		= rk3572_clk_bind,
	.probe		= rk3572_clk_probe,
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
	struct udevice *cru_dev;
	struct rk3572_clk_priv *priv;
	const struct rk3572_clk_info *clk_dump;
	struct clk clk;
	unsigned long clk_count = ARRAY_SIZE(clks_dump);
	unsigned long rate;
	int i, ret;

	ret = uclass_get_device_by_driver(UCLASS_CLK,
					  DM_DRIVER_GET(rockchip_rk3572_cru),
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
			memset(&clk, 0, sizeof(struct clk));
			clk.id = clk_dump->id;
			if (clk_dump->is_cru)
				ret = clk_request(cru_dev, &clk);
			if (ret < 0)
				return ret;

			rate = clk_get_rate(&clk);
			if (rate < 0)
				printf("  %s %s\n", clk_dump->name,
				       "unknown");
			else
				printf("  %s %lu KHz\n", clk_dump->name,
				       rate / 1000);
		}
	}

	return 0;
}
#endif
