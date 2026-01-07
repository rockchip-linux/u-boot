// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 * Author: Elaine Zhang <zhangqing@rock-chips.com>
 */

#include <common.h>
#include <bitfield.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <syscon.h>
#include <asm/arch/clock.h>
#include <asm/arch/cru_rv1126b.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <dm/lists.h>
#include <dt-bindings/clock/rockchip,rv1126b-cru.h>

DECLARE_GLOBAL_DATA_PTR;

#define DIV_TO_RATE(input_rate, div)	((input_rate) / ((div) + 1))

#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
#ifndef BITS_WITH_WMASK
#define BITS_WITH_WMASK(bits, msk, shift) \
	((bits) << (shift)) | ((msk) << ((shift) + 16))
#endif
#endif

static struct rockchip_pll_rate_table rv1126b_pll_rates[] = {
	/* _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	RK3036_PLL_RATE(1200000000, 1, 100, 2, 1, 1, 0),
	RK3036_PLL_RATE(1188000000, 1, 99, 2, 1, 1, 0),
	RK3036_PLL_RATE(1179648000, 1, 49, 1, 1, 0, 2550137),
	RK3036_PLL_RATE(1000000000, 3, 250, 2, 1, 1, 0),
	RK3036_PLL_RATE(993484800, 1, 41, 1, 1, 0, 6630355),
	RK3036_PLL_RATE(983040000, 1, 40, 1, 1, 0, 16106127),
	RK3036_PLL_RATE(903168000, 1, 75, 2, 1, 0, 4429185),
	{ /* sentinel */ },
};

static struct rockchip_pll_clock rv1126b_pll_clks[] = {
	[GPLL] = PLL(pll_rk3328, PLL_GPLL, RV1126B_PLL_CON(8),
		     RV1126B_MODE_CON, 2, 10, 0, rv1126b_pll_rates),
	[AUPLL] = PLL(pll_rk3328, PLL_AUPLL, RV1126B_PLL_CON(0),
		     RV1126B_MODE_CON, 0, 10, 0, rv1126b_pll_rates),
	[CPLL] = PLL(pll_rk3328, PLL_CPLL, RV1126B_PERIPLL_CON(0),
		     RV1126B_MODE_CON, 4, 10, 0, rv1126b_pll_rates),
};

#ifndef CONFIG_SPL_BUILD
#define RV1126B_CLK_DUMP(_id, _name, _iscru)	\
{						\
	.id = _id,				\
	.name = _name,				\
	.is_cru = _iscru,			\
}

static const struct rv1126b_clk_info clks_dump[] = {
	RV1126B_CLK_DUMP(PLL_GPLL, "gpll", true),
	RV1126B_CLK_DUMP(PLL_AUPLL, "aupll", true),
	RV1126B_CLK_DUMP(PLL_CPLL, "cpll", true),
	RV1126B_CLK_DUMP(ACLK_PERI_ROOT, "aclk_peri_root", true),
	RV1126B_CLK_DUMP(PCLK_PERI_ROOT, "pclk_peri_root", true),
	RV1126B_CLK_DUMP(ACLK_TOP_ROOT, "aclk_top_root", true),
	RV1126B_CLK_DUMP(PCLK_TOP_ROOT, "pclk_top_root", true),
	RV1126B_CLK_DUMP(ACLK_BUS_ROOT, "aclk_bus_root", true),
	RV1126B_CLK_DUMP(HCLK_BUS_ROOT, "hclk_bus_root", true),
	RV1126B_CLK_DUMP(PCLK_BUS_ROOT, "pclk_bus_root", true),
	RV1126B_CLK_DUMP(BUSCLK_PMU_SRC, "busclk_pmu_src", true),
};
#endif

static ulong rv1126b_peri_get_clk(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 con, sel, rate;

	switch (clk_id) {
	case ACLK_PERI_ROOT:
		con = readl(&cru->clksel_con[47]);
		sel = (con & ACLK_PERI_SEL_MASK) >> ACLK_PERI_SEL_SHIFT;
		if (sel == ACLK_PERI_SEL_200M)
			rate = 200 * MHz;
		else
			rate = OSC_HZ;
		break;
	case PCLK_PERI_ROOT:
		con = readl(&cru->clksel_con[47]);
		sel = (con & PCLK_PERI_SEL_MASK) >> PCLK_PERI_SEL_SHIFT;
		if (sel == PCLK_PERI_SEL_100M)
			rate = 100 * MHz;
		else
			rate = OSC_HZ;
		break;
	case ACLK_TOP_ROOT:
		con = readl(&cru->clksel_con[44]);
		sel = (con & ACLK_TOP_SEL_MASK) >> ACLK_TOP_SEL_SHIFT;
		if (sel == ACLK_TOP_SEL_600M)
			rate = 600 * MHz;
		else if (sel == ACLK_TOP_SEL_400M)
			rate = 400 * MHz;
		else
			rate = 200 * MHz;
		break;
	case PCLK_TOP_ROOT:
	case PCLK_BUS_ROOT:
	case BUSCLK_PMU_SRC:
		rate = 100 * MHz;
		break;
	case ACLK_BUS_ROOT:
		con = readl(&cru->clksel_con[44]);
		sel = (con & ACLK_BUS_SEL_MASK) >> ACLK_BUS_SEL_SHIFT;
		if (sel == ACLK_BUS_SEL_400M)
			rate = 400 * MHz;
		else if (sel == ACLK_BUS_SEL_300M)
			rate = 300 * MHz;
		else
			rate = 200 * MHz;
		break;
	case HCLK_BUS_ROOT:
		con = readl(&cru->clksel_con[44]);
		sel = (con & HCLK_BUS_SEL_MASK) >> HCLK_BUS_SEL_SHIFT;
		if (sel == HCLK_BUS_SEL_200M)
			rate = 200 * MHz;
		else
			rate = 100 * MHz;
		break;
	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rv1126b_peri_set_clk(struct rv1126b_clk_priv *priv,
				  ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk;

	switch (clk_id) {
	case ACLK_PERI_ROOT:
		if (rate >= 198 * MHz)
			src_clk = ACLK_PERI_SEL_200M;
		else
			src_clk = ACLK_PERI_SEL_24M;
		rk_clrsetreg(&cru->clksel_con[47],
			     ACLK_PERI_SEL_MASK,
			     src_clk << ACLK_PERI_SEL_SHIFT);
		break;
	case PCLK_PERI_ROOT:
		if (rate >= 99 * MHz)
			src_clk = PCLK_PERI_SEL_100M;
		else
			src_clk = PCLK_PERI_SEL_24M;
		rk_clrsetreg(&cru->clksel_con[47],
			     PCLK_PERI_SEL_MASK,
			     src_clk << PCLK_PERI_SEL_SHIFT);
		break;
	case ACLK_TOP_ROOT:
		if (rate >= 594 * MHz)
			src_clk = ACLK_TOP_SEL_600M;
		else if (rate >= 396 * MHz)
			src_clk = ACLK_TOP_SEL_400M;
		else
			src_clk = ACLK_TOP_SEL_200M;
		rk_clrsetreg(&cru->clksel_con[44],
			     ACLK_TOP_SEL_MASK,
			     src_clk << ACLK_TOP_SEL_SHIFT);
		break;
	case PCLK_TOP_ROOT:
	case PCLK_BUS_ROOT:
	case BUSCLK_PMU_SRC:
		break;
	case ACLK_BUS_ROOT:
		if (rate >= 396 * MHz)
			src_clk = ACLK_BUS_SEL_400M;
		else if (rate >= 297 * MHz)
			src_clk = ACLK_BUS_SEL_300M;
		else
			src_clk = ACLK_BUS_SEL_200M;
		rk_clrsetreg(&cru->clksel_con[44],
			     ACLK_BUS_SEL_MASK,
			     src_clk << ACLK_BUS_SEL_SHIFT);
		break;
	case HCLK_BUS_ROOT:
		if (rate >= 198 * MHz)
			src_clk = HCLK_BUS_SEL_200M;
		else
			src_clk = HCLK_BUS_SEL_100M;
		rk_clrsetreg(&cru->clksel_con[44],
			     HCLK_BUS_SEL_MASK,
			     src_clk << HCLK_BUS_SEL_SHIFT);
		break;
	default:
		printf("do not support this permid freq\n");
		return -EINVAL;
	}

	return rv1126b_peri_get_clk(priv, clk_id);
}

static ulong rv1126b_i2c_get_clk(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, con;
	ulong rate;

	switch (clk_id) {
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
	case CLK_I2C_BUS_SRC:
		con = readl(&cru->clksel_con[50]);
		sel = (con & CLK_I2C_SEL_MASK) >> CLK_I2C_SEL_SHIFT;
		if (sel == CLK_I2C_SEL_200M)
			rate = 200 * MHz;
		else
			rate = OSC_HZ;
		break;
	case CLK_I2C2:
		con = readl(&cru->pmu_clksel_con[2]);
		sel = (con & CLK_I2C2_SEL_MASK) >> CLK_I2C2_SEL_SHIFT;
		if (sel == CLK_I2C2_SEL_100M)
			rate = 100 * MHz;
		else if (sel == CLK_I2C2_SEL_RCOSC)
			rate = RC_OSC_HZ;
		else
			rate = OSC_HZ;
		break;
	default:
		return -ENOENT;
	}

	return rate;
}

static ulong rv1126b_i2c_set_clk(struct rv1126b_clk_priv *priv, ulong clk_id,
				 ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk;

	switch (clk_id) {
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
	case CLK_I2C_BUS_SRC:
		if (rate == OSC_HZ)
			src_clk = CLK_I2C_SEL_24M;
		else
			src_clk = CLK_I2C_SEL_200M;
		rk_clrsetreg(&cru->clksel_con[50], CLK_I2C_SEL_MASK,
			     src_clk << CLK_I2C_SEL_SHIFT);
		break;
	case CLK_I2C2:
		if (rate == OSC_HZ)
			src_clk = CLK_I2C2_SEL_24M;
		else if (rate == RC_OSC_HZ)
			src_clk = CLK_I2C2_SEL_RCOSC;
		else
			src_clk = CLK_I2C2_SEL_100M;
		rk_clrsetreg(&cru->pmu_clksel_con[2], CLK_I2C2_SEL_MASK,
			     src_clk << CLK_I2C2_SEL_SHIFT);
		break;
	default:
		return -ENOENT;
	}
	return rv1126b_i2c_get_clk(priv, clk_id);
}

static ulong rv1126b_crypto_get_clk(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, con, rate;

	switch (clk_id) {
	case PCLK_RKCE:
		return rv1126b_peri_get_clk(priv, PCLK_BUS_ROOT);
	case HCLK_NS_RKCE:
		return rv1126b_peri_get_clk(priv, HCLK_BUS_ROOT);
	case ACLK_RKCE_SRC:
	case ACLK_NSRKCE:
		con = readl(&cru->clksel_con[50]);
		sel = (con & ACLK_RKCE_SEL_MASK) >>
		      ACLK_RKCE_SEL_SHIFT;
		if (sel == ACLK_RKCE_SEL_200M)
			rate = 200 * MHz;
		else
			rate = OSC_HZ;
		break;
	case CLK_PKA_RKCE_SRC:
	case CLK_PKA_NSRKCE:
		con = readl(&cru->clksel_con[50]);
		sel = (con & CLK_PKA_RKCE_SEL_MASK) >>
		      CLK_PKA_RKCE_SEL_SHIFT;
		if (sel == CLK_PKA_RKCE_SEL_300M)
			rate = 300 * MHz;
		else
			rate = 200 * MHz;
		break;
	default:
		return -ENOENT;
	}
	return rate;
}

static ulong rv1126b_crypto_set_clk(struct rv1126b_clk_priv *priv,
				    ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel;

	switch (clk_id) {
	case PCLK_RKCE:
		break;
	case HCLK_NS_RKCE:
		rv1126b_peri_set_clk(priv, HCLK_BUS_ROOT, rate);
	case ACLK_RKCE_SRC:
	case ACLK_NSRKCE:
		if (rate >= 198 * MHz)
			sel = ACLK_RKCE_SEL_200M;
		else
			sel = ACLK_RKCE_SEL_24M;
		rk_clrsetreg(&cru->clksel_con[50],
			     ACLK_RKCE_SEL_MASK,
			     (sel << ACLK_RKCE_SEL_SHIFT));
		break;
	case CLK_PKA_RKCE_SRC:
	case CLK_PKA_NSRKCE:
		if (rate >= 297 * MHz)
			sel = CLK_PKA_RKCE_SEL_300M;
		else
			sel = CLK_PKA_RKCE_SEL_200M;
		rk_clrsetreg(&cru->clksel_con[50],
			     CLK_PKA_RKCE_SEL_MASK,
			     (sel << CLK_PKA_RKCE_SEL_SHIFT));
		break;
	default:
		return -ENOENT;
	}
	return rv1126b_crypto_get_clk(priv, clk_id);
}

static ulong rv1126b_mmc_get_clk(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 div, sel, con, prate;

	switch (clk_id) {
	case CCLK_SDMMC0:
	case HCLK_SDMMC0:
		con = readl(&cru->clksel_con[45]);
		sel = (con & CLK_SDMMC_SEL_MASK) >>
		      CLK_SDMMC_SEL_SHIFT;
		div = (con & CLK_SDMMC_DIV_MASK) >>
		      CLK_SDMMC_DIV_SHIFT;
		if (sel == CLK_SDMMC_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == CLK_SDMMC_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case CCLK_SDMMC1:
	case HCLK_SDMMC1:
		con = readl(&cru->clksel_con[46]);
		sel = (con & CLK_SDMMC_SEL_MASK) >>
		      CLK_SDMMC_SEL_SHIFT;
		div = (con & CLK_SDMMC_DIV_MASK) >>
		      CLK_SDMMC_DIV_SHIFT;
		if (sel == CLK_SDMMC_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == CLK_SDMMC_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case CCLK_EMMC:
	case HCLK_EMMC:
		con = readl(&cru->clksel_con[47]);
		sel = (con & CLK_SDMMC_SEL_MASK) >>
		      CLK_SDMMC_SEL_SHIFT;
		div = (con & CLK_SDMMC_DIV_MASK) >>
		      CLK_SDMMC_DIV_SHIFT;
		if (sel == CLK_SDMMC_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == CLK_SDMMC_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case SCLK_2X_FSPI0:
	case HCLK_FSPI0:
	case HCLK_XIP_FSPI0:
		con = readl(&cru->clksel_con[48]);
		sel = (con & CLK_SDMMC_SEL_MASK) >>
		      CLK_SDMMC_SEL_SHIFT;
		div = (con & CLK_SDMMC_DIV_MASK) >>
		      CLK_SDMMC_DIV_SHIFT;
		if (sel == CLK_SDMMC_SEL_GPLL)
			prate = priv->gpll_hz;
		else if (sel == CLK_SDMMC_SEL_CPLL)
			prate = priv->cpll_hz;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	case SCLK_1X_FSPI1:
	case HCLK_FSPI1:
	case HCLK_XIP_FSPI1:
		con = readl(&cru->pmu1_clksel_con[0]);
		sel = (con & SCLK_1X_FSPI1_SEL_MASK) >>
		     SCLK_1X_FSPI1_SEL_SHIFT;
		div = (con & SCLK_1X_FSPI1_DIV_MASK) >>
		      SCLK_1X_FSPI1_DIV_SHIFT;
		if (sel == SCLK_1X_FSPI1_SEL_100M)
			prate = 100 * MHz;
		else if (sel == SCLK_1X_FSPI1_SEL_RCOSC)
			prate = RC_OSC_HZ;
		else
			prate = OSC_HZ;
		return DIV_TO_RATE(prate, div);
	default:
		return -ENOENT;
	}
}

static ulong rv1126b_mmc_set_clk(struct rv1126b_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, src_clk_div;
	ulong prate = 0;

	if ((OSC_HZ % rate) == 0) {
		sel = CLK_SDMMC_SEL_24M;
		prate = OSC_HZ;
	} else if ((priv->cpll_hz % rate) == 0) {
		sel = CLK_SDMMC_SEL_CPLL;
		prate = priv->cpll_hz;
	} else {
		sel = CLK_SDMMC_SEL_GPLL;
		prate = priv->gpll_hz;
	}
	src_clk_div = DIV_ROUND_UP(prate, rate);

	switch (clk_id) {
	case CCLK_SDMMC0:
	case HCLK_SDMMC0:
		src_clk_div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->clksel_con[45],
			     CLK_SDMMC_SEL_MASK |
			     CLK_SDMMC_DIV_MASK,
			     (sel << CLK_SDMMC_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_SDMMC_DIV_SHIFT));
		break;
	case CCLK_SDMMC1:
	case HCLK_SDMMC1:
		src_clk_div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->clksel_con[46],
			     CLK_SDMMC_SEL_MASK |
			     CLK_SDMMC_DIV_MASK,
			     (sel << CLK_SDMMC_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_SDMMC_DIV_SHIFT));
		break;
	case CCLK_EMMC:
	case HCLK_EMMC:
		src_clk_div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->clksel_con[47],
			     CLK_SDMMC_SEL_MASK |
			     CLK_SDMMC_DIV_MASK,
			     (sel << CLK_SDMMC_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_SDMMC_DIV_SHIFT));
		break;
	case SCLK_2X_FSPI0:
	case HCLK_FSPI0:
	case HCLK_XIP_FSPI0:
		src_clk_div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->clksel_con[48],
			     CLK_SDMMC_SEL_MASK |
			     CLK_SDMMC_DIV_MASK,
			     (sel << CLK_SDMMC_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_SDMMC_DIV_SHIFT));
		break;
	case SCLK_1X_FSPI1:
	case HCLK_FSPI1:
	case HCLK_XIP_FSPI1:
		if ((OSC_HZ % rate) == 0) {
			sel = SCLK_1X_FSPI1_SEL_24M;
			prate = OSC_HZ;
		} else if ((100 * MHz % rate) == 0) {
			sel = SCLK_1X_FSPI1_SEL_100M;
			prate = priv->cpll_hz;
		} else {
			sel = SCLK_1X_FSPI1_SEL_RCOSC;
			prate = RC_OSC_HZ;
		}
		src_clk_div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->pmu1_clksel_con[0],
			     SCLK_1X_FSPI1_SEL_MASK |
			     SCLK_1X_FSPI1_DIV_MASK,
			     (sel << SCLK_1X_FSPI1_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      SCLK_1X_FSPI1_DIV_SHIFT));
		break;
	default:
		return -ENOENT;
	}
	return rv1126b_mmc_get_clk(priv, clk_id);
}

static ulong rv1126b_spi_get_clk(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, con, rate;

	switch (clk_id) {
	case CLK_SPI0:
		con = readl(&cru->clksel_con[50]);
		sel = (con & CLK_SPI0_SEL_MASK) >> CLK_SPI0_SEL_SHIFT;
		break;
	case CLK_SPI1:
		con = readl(&cru->clksel_con[50]);
		sel = (con & CLK_SPI1_SEL_MASK) >> CLK_SPI1_SEL_SHIFT;
		break;
	default:
		return -ENOENT;
	}
	if (sel == CLK_SPI0_SEL_200M)
		rate = 200 * MHz;
	else if (sel == CLK_SPI0_SEL_100M)
		rate = 100 * MHz;
	else if (sel == CLK_SPI0_SEL_50M)
		rate = 50 * MHz;
	else
		rate = OSC_HZ;

	return rate;
}

static ulong rv1126b_spi_set_clk(struct rv1126b_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk;

	if (rate >= 198 * MHz)
		src_clk = CLK_SPI0_SEL_200M;
	else if (rate >= 99 * MHz)
		src_clk = CLK_SPI0_SEL_100M;
	else if (rate >= 48 * MHz)
		src_clk = CLK_SPI0_SEL_50M;
	else
		src_clk = CLK_SPI0_SEL_24M;

	switch (clk_id) {
	case CLK_SPI0:
		rk_clrsetreg(&cru->clksel_con[50], CLK_SPI0_SEL_MASK,
			     src_clk << CLK_SPI0_SEL_SHIFT);
		break;
	case CLK_SPI1:
		rk_clrsetreg(&cru->clksel_con[50], CLK_SPI1_SEL_MASK,
			     src_clk << CLK_SPI1_SEL_SHIFT);
		break;
	default:
		return -ENOENT;
	}

	return rv1126b_spi_get_clk(priv, clk_id);
}

static ulong rv1126b_pwm_get_clk(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, div, con;

	switch (clk_id) {
	case CLK_PWM0:
		con = readl(&cru->clksel_con[50]);
		sel = (con & CLK_PWM0_SEL_MASK) >> CLK_PWM0_SEL_SHIFT;
		break;
	case CLK_PWM2:
		con = readl(&cru->clksel_con[50]);
		sel = (con & CLK_PWM2_SEL_MASK) >> CLK_PWM2_SEL_SHIFT;
		break;
	case CLK_PWM3:
		con = readl(&cru->clksel_con[50]);
		sel = (con & CLK_PWM3_SEL_MASK) >> CLK_PWM3_SEL_SHIFT;
		break;
	case CLK_PWM1:
		con = readl(&cru->pmu_clksel_con[2]);
		sel = (con & CLK_PWM1_SEL_MASK) >> CLK_PWM1_SEL_SHIFT;
		div = (con & CLK_PWM1_DIV_MASK) >>
		      CLK_PWM1_DIV_SHIFT;
		if (sel == CLK_PWM1_SEL_100M)
			return DIV_TO_RATE(100 * MHz, div);
		else if (sel == CLK_PWM1_SEL_RCOSC)
			return DIV_TO_RATE(RC_OSC_HZ, div);
		else
			return DIV_TO_RATE(OSC_HZ, div);
	default:
		return -ENOENT;
	}

	switch (sel) {
	case CLK_PWM_SEL_100M:
		return 100 * MHz;
	case CLK_PWM_SEL_24M:
		return OSC_HZ;
	default:
		return -ENOENT;
	}
}

static ulong rv1126b_pwm_set_clk(struct rv1126b_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk, src_clk_div, prate;

	if (rate >= 99 * MHz)
		src_clk = CLK_PWM_SEL_100M;
	else
		src_clk = CLK_PWM_SEL_24M;

	switch (clk_id) {
	case CLK_PWM0:
		rk_clrsetreg(&cru->clksel_con[50],
			     CLK_PWM0_SEL_MASK,
			     src_clk << CLK_PWM0_SEL_SHIFT);
		break;
	case CLK_PWM2:
		rk_clrsetreg(&cru->clksel_con[50],
			     CLK_PWM2_SEL_MASK,
			     src_clk << CLK_PWM2_SEL_SHIFT);
		break;
	case CLK_PWM3:
		rk_clrsetreg(&cru->clksel_con[50],
			     CLK_PWM3_SEL_MASK,
			     src_clk << CLK_PWM3_SEL_SHIFT);
		break;
	case CLK_PWM1:
		if ((OSC_HZ % rate) == 0) {
			src_clk = CLK_PWM1_SEL_24M;
			prate = OSC_HZ;
		} else if ((100 * MHz % rate) == 0) {
			src_clk = CLK_PWM1_SEL_100M;
			prate = priv->cpll_hz;
		} else {
			src_clk = CLK_PWM1_SEL_RCOSC;
			prate = RC_OSC_HZ;
		}
		src_clk_div = DIV_ROUND_UP(prate, rate);
		rk_clrsetreg(&cru->pmu1_clksel_con[2],
			     CLK_PWM1_SEL_MASK |
			     CLK_PWM1_DIV_MASK,
			     (src_clk << CLK_PWM1_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_PWM1_DIV_SHIFT));
		break;

	default:
		return -ENOENT;
	}

	return rv1126b_pwm_get_clk(priv, clk_id);
}

static ulong rv1126b_adc_get_clk(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, div, con;

	switch (clk_id) {
	case CLK_SARADC0:
	case CLK_SARADC0_SRC:
		con = readl(&cru->clksel_con[63]);
		sel = (con & CLK_SARADC0_SEL_MASK) >> CLK_SARADC0_SEL_SHIFT;
		div = (con & CLK_SARADC0_DIV_MASK) >>
		      CLK_SARADC0_DIV_SHIFT;
		break;
	case CLK_SARADC1:
	case CLK_SARADC1_SRC:
		con = readl(&cru->clksel_con[63]);
		sel = (con & CLK_SARADC1_SEL_MASK) >> CLK_SARADC1_SEL_SHIFT;
		div = (con & CLK_SARADC1_DIV_MASK) >>
		      CLK_SARADC1_DIV_SHIFT;
		break;
	case CLK_SARADC2:
	case CLK_SARADC2_SRC:
		con = readl(&cru->clksel_con[63]);
		sel = (con & CLK_SARADC2_SEL_MASK) >> CLK_SARADC2_SEL_SHIFT;
		div = (con & CLK_SARADC2_DIV_MASK) >>
		      CLK_SARADC2_DIV_SHIFT;
		break;
	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		return OSC_HZ;
	default:
		return -ENOENT;
	}

	if (sel == CLK_SARADC_SEL_200M)
		return DIV_TO_RATE(200 * MHz, div);
	else
		return DIV_TO_RATE(OSC_HZ, div);
}

static ulong rv1126b_adc_set_clk(struct rv1126b_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk_sel, src_clk_div, prate;

	if ((OSC_HZ % rate) == 0) {
		src_clk_sel = CLK_SARADC_SEL_24M;
		prate = OSC_HZ;
	} else {
		src_clk_sel = CLK_SARADC_SEL_200M;
		prate = 200 * MHz;
	}
	src_clk_div = DIV_ROUND_UP(prate, rate);

	switch (clk_id) {
	case CLK_SARADC0:
	case CLK_SARADC0_SRC:
		assert(src_clk_div - 1 <= 7);
		rk_clrsetreg(&cru->clksel_con[63],
			     CLK_SARADC0_SEL_MASK |
			     CLK_SARADC0_DIV_MASK,
			     (src_clk_sel << CLK_SARADC0_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_SARADC0_DIV_SHIFT));
		break;
	case CLK_SARADC1:
	case CLK_SARADC1_SRC:
		assert(src_clk_div - 1 <= 7);
		rk_clrsetreg(&cru->clksel_con[63],
			     CLK_SARADC1_SEL_MASK |
			     CLK_SARADC1_DIV_MASK,
			     (src_clk_sel << CLK_SARADC1_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_SARADC1_DIV_SHIFT));
		break;
	case CLK_SARADC2:
	case CLK_SARADC2_SRC:
		assert(src_clk_div - 1 <= 7);
		rk_clrsetreg(&cru->clksel_con[63],
			     CLK_SARADC2_SEL_MASK |
			     CLK_SARADC2_DIV_MASK,
			     (src_clk_sel << CLK_SARADC2_SEL_SHIFT) |
			     ((src_clk_div - 1) <<
			      CLK_SARADC2_DIV_SHIFT));
		break;
	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		break;
	default:
		return -ENOENT;
	}
	return rv1126b_adc_get_clk(priv, clk_id);
}

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

static ulong rv1126b_frac_get_rate(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 reg, reg_h, src, con, p_rate;
	unsigned long m, n, m_l, n_l, m_h, n_h;

	switch (clk_id) {
	case CLK_CM_FRAC0:
		con = readl(&cru->clksel_con[10]);
		src = (con & CLK_CM_FRAC0_SRC_SEL_MASK) >>
		      CLK_CM_FRAC0_SRC_SEL_SHIFT;
		reg = readl(&cru->clksel_con[25]);
		reg_h = readl(&cru->clk_cm_frac0_div_h);
		break;
	case CLK_CM_FRAC1:
		con = readl(&cru->clksel_con[10]);
		src = (con & CLK_CM_FRAC1_SRC_SEL_MASK) >>
		      CLK_CM_FRAC1_SRC_SEL_SHIFT;
		reg = readl(&cru->clksel_con[26]);
		reg_h = readl(&cru->clk_cm_frac1_div_h);
		break;
	case CLK_CM_FRAC2:
		con = readl(&cru->clksel_con[10]);
		src = (con & CLK_CM_FRAC2_SRC_SEL_MASK) >>
		      CLK_CM_FRAC2_SRC_SEL_SHIFT;
		reg = readl(&cru->clksel_con[27]);
		reg_h = readl(&cru->clk_cm_frac2_div_h);
		break;
	case CLK_UART_FRAC0:
		con = readl(&cru->clksel_con[10]);
		src = (con & CLK_UART_FRAC0_SRC_SEL_MASK) >>
		      CLK_UART_FRAC0_SRC_SEL_SHIFT;
		reg = readl(&cru->clksel_con[28]);
		reg_h = readl(&cru->clk_uart_frac0_div_h);
		break;
	case CLK_UART_FRAC1:
		con = readl(&cru->clksel_con[10]);
		src = (con & CLK_UART_FRAC1_SRC_SEL_MASK) >>
		      CLK_UART_FRAC1_SRC_SEL_SHIFT;
		reg = readl(&cru->clksel_con[29]);
		reg_h = readl(&cru->clk_uart_frac1_div_h);
		break;
	case CLK_AUDIO_FRAC0:
		con = readl(&cru->clksel_con[10]);
		src = (con & CLK_AUDIO_FRAC0_SRC_SEL_MASK) >>
		      CLK_AUDIO_FRAC0_SRC_SEL_SHIFT;
		reg = readl(&cru->clksel_con[30]);
		reg_h = readl(&cru->clk_audio_frac0_div_h);
		break;
	case CLK_AUDIO_FRAC1:
		con = readl(&cru->clksel_con[10]);
		src = (con & CLK_AUDIO_FRAC1_SRC_SEL_MASK) >>
		      CLK_AUDIO_FRAC1_SRC_SEL_SHIFT;
		reg = readl(&cru->clksel_con[31]);
		reg_h = readl(&cru->clk_audio_frac1_div_h);
		break;

	default:
		return -ENOENT;
	}

	switch (src) {
	case CLK_FRAC_SRC_SEL_24M:
		p_rate = OSC_HZ;
		break;
	case CLK_FRAC_SRC_SEL_GPLL:
		p_rate = priv->gpll_hz;
		break;
	case CLK_FRAC_SRC_SEL_AUPLL:
		p_rate = priv->aupll_hz;
		break;
	case CLK_FRAC_SRC_SEL_CPLL:
		p_rate = priv->cpll_hz;
		break;
	default:
		return -ENOENT;
	}

	n_l = reg & CLK_FRAC_NUMERATOR_MASK;
	n_l >>= CLK_FRAC_NUMERATOR_SHIFT;
	m_l = reg & CLK_FRAC_DENOMINATOR_MASK;
	m_l >>= CLK_FRAC_DENOMINATOR_SHIFT;
	n_h = reg_h & CLK_FRAC_H_NUMERATOR_MASK;
	n_h >>= CLK_FRAC_H_NUMERATOR_SHIFT;
	m_h = reg_h & CLK_FRAC_H_DENOMINATOR_MASK;
	m_h >>= CLK_FRAC_H_DENOMINATOR_SHIFT;
	n = n_l | (n_h << 16);
	m = m_l | (m_h << 16);
	return p_rate * n / m;
}

static ulong rv1126b_frac_set_rate(struct rv1126b_clk_priv *priv,
				   ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 src, p_rate, val;
	unsigned long m, n, m_l, n_l, m_h, n_h;

	if ((OSC_HZ % rate) == 0) {
		src = CLK_FRAC_SRC_SEL_24M;
		p_rate = OSC_HZ;
	} else if ((priv->aupll_hz % rate) == 0) {
		src = CLK_FRAC_SRC_SEL_AUPLL;
		p_rate = priv->aupll_hz;
	} else if ((priv->cpll_hz % rate) == 0) {
		src = CLK_FRAC_SRC_SEL_CPLL;
		p_rate = priv->cpll_hz;
	} else {
		src = CLK_FRAC_SRC_SEL_GPLL;
		p_rate = priv->gpll_hz;
	}

	rational_best_approximation(rate, p_rate,
				    GENMASK(24 - 1, 0),
				    GENMASK(24 - 1, 0),
				    &m, &n);

	if (m < 4 && m != 0) {
		if (n % 2 == 0)
			val = 1;
		else
			val = DIV_ROUND_UP(4, m);

		n *= val;
		m *= val;
		if (n > 0xffffff)
			n = 0xffffff;
	}

	n_l = n & 0xffff;
	m_l = m & 0xffff;
	n_h = (n & 0xff0000) >> 16;
	m_h = (m & 0xff0000) >> 16;

	switch (clk_id) {
	case CLK_CM_FRAC0:
		rk_clrsetreg(&cru->clksel_con[10],
			     CLK_CM_FRAC0_SRC_SEL_MASK,
			     (src << CLK_CM_FRAC0_SRC_SEL_SHIFT));
		val = m_h << CLK_FRAC_H_NUMERATOR_SHIFT | n_h;
		writel(val, &cru->clk_cm_frac0_div_h);
		val = m_l << CLK_FRAC_NUMERATOR_SHIFT | n_l;
		writel(val, &cru->clksel_con[25]);
		break;
	case CLK_CM_FRAC1:
		rk_clrsetreg(&cru->clksel_con[10],
			     CLK_CM_FRAC1_SRC_SEL_MASK,
			     (src << CLK_CM_FRAC1_SRC_SEL_SHIFT));
		val = m_h << CLK_FRAC_H_NUMERATOR_SHIFT | n_h;
		writel(val, &cru->clk_cm_frac1_div_h);
		val = m_l << CLK_FRAC_NUMERATOR_SHIFT | n_l;
		writel(val, &cru->clksel_con[26]);
		break;
	case CLK_CM_FRAC2:
		rk_clrsetreg(&cru->clksel_con[10],
			     CLK_CM_FRAC2_SRC_SEL_MASK,
			     (src << CLK_CM_FRAC2_SRC_SEL_SHIFT));
		val = m_h << CLK_FRAC_H_NUMERATOR_SHIFT | n_h;
		writel(val, &cru->clk_cm_frac2_div_h);
		val = m_l << CLK_FRAC_NUMERATOR_SHIFT | n_l;
		writel(val, &cru->clksel_con[27]);
		break;
	case CLK_UART_FRAC0:
		rk_clrsetreg(&cru->clksel_con[10],
			     CLK_UART_FRAC0_SRC_SEL_MASK,
			     (src << CLK_UART_FRAC0_SRC_SEL_SHIFT));
		val = m_h << CLK_FRAC_H_NUMERATOR_SHIFT | n_h;
		writel(val, &cru->clk_uart_frac0_div_h);
		val = m_l << CLK_FRAC_NUMERATOR_SHIFT | n_l;
		writel(val, &cru->clksel_con[28]);
		break;
	case CLK_UART_FRAC1:
		rk_clrsetreg(&cru->clksel_con[10],
			     CLK_UART_FRAC1_SRC_SEL_MASK,
			     (src << CLK_UART_FRAC1_SRC_SEL_SHIFT));
		val = m_h << CLK_FRAC_H_NUMERATOR_SHIFT | n_h;
		writel(val, &cru->clk_uart_frac1_div_h);
		val = m_l << CLK_FRAC_NUMERATOR_SHIFT | n_l;
		writel(val, &cru->clksel_con[29]);
		break;
	case CLK_AUDIO_FRAC0:
		rk_clrsetreg(&cru->clksel_con[10],
			     CLK_AUDIO_FRAC0_SRC_SEL_MASK,
			     (src << CLK_AUDIO_FRAC0_SRC_SEL_SHIFT));
		val = m_h << CLK_FRAC_H_NUMERATOR_SHIFT | n_h;
		writel(val, &cru->clk_audio_frac0_div_h);
		val = m_l << CLK_FRAC_NUMERATOR_SHIFT | n_l;
		writel(val, &cru->clksel_con[30]);
		break;
	case CLK_AUDIO_FRAC1:
		rk_clrsetreg(&cru->clksel_con[10],
			     CLK_AUDIO_FRAC0_SRC_SEL_MASK,
			     (src << CLK_AUDIO_FRAC0_SRC_SEL_SHIFT));
		val = m_h << CLK_FRAC_H_NUMERATOR_SHIFT | n_h;
		writel(val, &cru->clk_audio_frac1_div_h);
		val = m_l << CLK_FRAC_NUMERATOR_SHIFT | n_l;
		writel(val, &cru->clksel_con[31]);
		break;

	default:
		return -ENOENT;
	}

	return rv1126b_frac_get_rate(priv, clk_id);
}

static ulong rv1126b_uart_get_rate(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 con, div, src, p_rate;

	switch (clk_id) {
	case SCLK_UART0:
		con = readl(&cru->pmu_clksel_con[3]);
		src = (con & SCLK_UART0_SEL_MASK) >> SCLK_UART0_SEL_SHIFT;
		if (src == SCLK_UART0_SEL_UART0_SRC)
			return rv1126b_uart_get_rate(priv, SCLK_UART0_SRC);
		else if (src == SCLK_UART0_SEL_UART0_SRC)
			return RC_OSC_HZ;
		else
			return OSC_HZ;
	case SCLK_UART0_SRC:
		con = readl(&cru->clksel_con[12]);
		src = (con & SCLK_UART0_SRC_SEL_MASK) >>
		      SCLK_UART0_SRC_SEL_SHIFT;
		div = (con & SCLK_UART0_SRC_DIV_MASK) >>
		      SCLK_UART0_SRC_DIV_SHIFT;
		break;
	case SCLK_UART1:
		con = readl(&cru->clksel_con[12]);
		src = (con & SCLK_UART1_SEL_MASK) >> SCLK_UART1_SEL_SHIFT;
		div = (con & SCLK_UART1_DIV_MASK) >> SCLK_UART1_DIV_SHIFT;
		break;
	case SCLK_UART2:
		con = readl(&cru->clksel_con[13]);
		src = (con & SCLK_UART2_SEL_MASK) >> SCLK_UART2_SEL_SHIFT;
		div = (con & SCLK_UART2_DIV_MASK) >> SCLK_UART2_DIV_SHIFT;
		break;
	case SCLK_UART3:
		con = readl(&cru->clksel_con[13]);
		src = (con & SCLK_UART3_SEL_MASK) >> SCLK_UART3_SEL_SHIFT;
		div = (con & SCLK_UART3_DIV_MASK) >> SCLK_UART3_DIV_SHIFT;
		break;
	case SCLK_UART4:
		con = readl(&cru->clksel_con[14]);
		src = (con & SCLK_UART4_SEL_MASK) >> SCLK_UART4_SEL_SHIFT;
		div = (con & SCLK_UART4_DIV_MASK) >> SCLK_UART4_DIV_SHIFT;
		break;
	case SCLK_UART5:
		con = readl(&cru->clksel_con[14]);
		src = (con & SCLK_UART5_SEL_MASK) >> SCLK_UART5_SEL_SHIFT;
		div = (con & SCLK_UART5_DIV_MASK) >> SCLK_UART5_DIV_SHIFT;
		break;
	case SCLK_UART6:
		con = readl(&cru->clksel_con[15]);
		src = (con & SCLK_UART6_SEL_MASK) >> SCLK_UART6_SEL_SHIFT;
		div = (con & SCLK_UART6_DIV_MASK) >> SCLK_UART6_DIV_SHIFT;
		break;
	case SCLK_UART7:
		con = readl(&cru->clksel_con[15]);
		src = (con & SCLK_UART7_SEL_MASK) >> SCLK_UART7_SEL_SHIFT;
		div = (con & SCLK_UART7_DIV_MASK) >> SCLK_UART7_DIV_SHIFT;
		break;

	default:
		return -ENOENT;
	}

	switch (src) {
	case SCLK_UART_SEL_OSC:
		p_rate = OSC_HZ;
		break;
	case SCLK_UART_SEL_CM_FRAC0:
		p_rate = rv1126b_frac_get_rate(priv, CLK_CM_FRAC0);
		break;
	case SCLK_UART_SEL_CM_FRAC1:
		p_rate = rv1126b_frac_get_rate(priv, CLK_CM_FRAC1);
		break;
	case SCLK_UART_SEL_CM_FRAC2:
		p_rate = rv1126b_frac_get_rate(priv, CLK_CM_FRAC2);
		break;
	case SCLK_UART_SEL_UART_FRAC0:
		p_rate = rv1126b_frac_get_rate(priv, CLK_UART_FRAC0);
		break;
	case SCLK_UART_SEL_UART_FRAC1:
		p_rate = rv1126b_frac_get_rate(priv, CLK_UART_FRAC1);
		break;
	default:
		return -ENOENT;
	}

	return DIV_TO_RATE(p_rate, div);
}

static ulong rv1126b_uart_set_rate(struct rv1126b_clk_priv *priv,
				   ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 uart_src, div, p_rate;

	if (rv1126b_frac_get_rate(priv, CLK_CM_FRAC0) % rate == 0) {
		uart_src = SCLK_UART_SEL_CM_FRAC0;
		p_rate = rv1126b_frac_get_rate(priv, CLK_CM_FRAC0);
	} else if (rv1126b_frac_get_rate(priv, CLK_CM_FRAC1) % rate == 0) {
		uart_src = SCLK_UART_SEL_CM_FRAC1;
		p_rate = rv1126b_frac_get_rate(priv, CLK_CM_FRAC1);
	} else if (rv1126b_frac_get_rate(priv, CLK_CM_FRAC2) % rate == 0) {
		uart_src = SCLK_UART_SEL_CM_FRAC2;
		p_rate = rv1126b_frac_get_rate(priv, CLK_CM_FRAC2);
	} else if (rv1126b_frac_get_rate(priv, CLK_UART_FRAC0) % rate == 0) {
		uart_src = SCLK_UART_SEL_UART_FRAC0;
		p_rate = rv1126b_frac_get_rate(priv, CLK_UART_FRAC0);
	} else if (rv1126b_frac_get_rate(priv, CLK_UART_FRAC1) % rate == 0) {
		uart_src = SCLK_UART_SEL_UART_FRAC1;
		p_rate = rv1126b_frac_get_rate(priv, CLK_UART_FRAC1);
	} else {
		uart_src = SCLK_UART_SEL_OSC;
		p_rate = OSC_HZ;
	}

	div = DIV_ROUND_UP(p_rate, rate);

	switch (clk_id) {
	case SCLK_UART0:
		if (rate == OSC_HZ)
			uart_src = SCLK_UART0_SEL_OSC;
		else if (rate == RC_OSC_HZ)
			uart_src = SCLK_UART0_SEL_RCOSC;
		else
			uart_src = SCLK_UART0_SEL_UART0_SRC;
		rk_clrsetreg(&cru->pmu_clksel_con[3],
			     SCLK_UART0_SEL_MASK,
			     uart_src << SCLK_UART0_SEL_SHIFT);
		if (uart_src == SCLK_UART0_SEL_UART0_SRC)
			rv1126b_uart_set_rate(priv, SCLK_UART0_SRC, rate);
		break;
	case SCLK_UART0_SRC:
		rk_clrsetreg(&cru->clksel_con[12],
			     SCLK_UART0_SRC_SEL_MASK |
			     SCLK_UART0_SRC_DIV_MASK,
			     (uart_src << SCLK_UART0_SRC_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART0_SRC_DIV_SHIFT));
		break;
	case SCLK_UART1:
		rk_clrsetreg(&cru->clksel_con[12],
			     SCLK_UART1_SEL_MASK |
			     SCLK_UART1_DIV_MASK,
			     (uart_src << SCLK_UART1_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART1_DIV_SHIFT));
		break;
	case SCLK_UART2:
		rk_clrsetreg(&cru->clksel_con[13],
			     SCLK_UART2_SEL_MASK |
			     SCLK_UART2_DIV_MASK,
			     (uart_src << SCLK_UART2_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART2_DIV_SHIFT));
		break;
	case SCLK_UART3:
		rk_clrsetreg(&cru->clksel_con[13],
			     SCLK_UART3_SEL_MASK |
			     SCLK_UART3_DIV_MASK,
			     (uart_src << SCLK_UART3_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART3_DIV_SHIFT));
		break;
	case SCLK_UART4:
		rk_clrsetreg(&cru->clksel_con[14],
			     SCLK_UART4_SEL_MASK |
			     SCLK_UART4_DIV_MASK,
			     (uart_src << SCLK_UART4_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART4_DIV_SHIFT));
		break;
	case SCLK_UART5:
		rk_clrsetreg(&cru->clksel_con[14],
			     SCLK_UART5_SEL_MASK |
			     SCLK_UART5_DIV_MASK,
			     (uart_src << SCLK_UART5_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART5_DIV_SHIFT));
		break;
	case SCLK_UART6:
		rk_clrsetreg(&cru->clksel_con[15],
			     SCLK_UART6_SEL_MASK |
			     SCLK_UART6_DIV_MASK,
			     (uart_src << SCLK_UART6_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART6_DIV_SHIFT));
		break;
	case SCLK_UART7:
		rk_clrsetreg(&cru->clksel_con[15],
			     SCLK_UART7_SEL_MASK |
			     SCLK_UART7_DIV_MASK,
			     (uart_src << SCLK_UART7_SEL_SHIFT) |
			     ((div - 1) <<
			      SCLK_UART7_DIV_SHIFT));
		break;
	default:
		return -ENOENT;
	}

	return rv1126b_uart_get_rate(priv, clk_id);
}

static ulong rv1126b_wdt_get_rate(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, con;

	switch (clk_id) {
	case TCLK_WDT_NS_SRC:
	case TCLK_WDT_NS:
		con = readl(&cru->clksel_con[46]);
		sel = (con & TCLK_WDT_NS_SEL_MASK) >>
		      TCLK_WDT_NS_SEL_SHIFT;
		break;
	case TCLK_WDT_S:
		con = readl(&cru->clksel_con[46]);
		sel = (con & TCLK_WDT_S_SEL_MASK) >>
		      TCLK_WDT_S_SEL_SHIFT;
		break;
	case TCLK_WDT_HPMCU:
		con = readl(&cru->clksel_con[46]);
		sel = (con & TCLK_WDT_HPMCU_SEL_MASK) >>
		      TCLK_WDT_HPMCU_SEL_SHIFT;
		break;
	case TCLK_WDT_LPMCU:
		con = readl(&cru->pmu_clksel_con[3]);
		sel = (con & TCLK_WDT_LPMCU_SEL_MASK) >>
		      TCLK_WDT_LPMCU_SEL_SHIFT;
		if (sel == TCLK_WDT_LPMCU_SEL_100M)
			return 100 * MHz;
		else if (sel == TCLK_WDT_LPMCU_SEL_RCOSC)
			return RC_OSC_HZ;
		else if (sel == TCLK_WDT_LPMCU_SEL_OSC)
			return OSC_HZ;
		else
			return 32768;
	default:
		return -ENOENT;
	}

	if (sel == TCLK_WDT_SEL_100M)
		return 100 * MHz;
	else
		return OSC_HZ;
}

static ulong rv1126b_wdt_set_rate(struct rv1126b_clk_priv *priv,
				  ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk_sel;

	if (rate == OSC_HZ)
		src_clk_sel = TCLK_WDT_SEL_OSC;
	else
		src_clk_sel = TCLK_WDT_SEL_100M;

	switch (clk_id) {
	case TCLK_WDT_NS_SRC:
		rk_clrsetreg(&cru->clksel_con[46],
			     TCLK_WDT_NS_SEL_MASK,
			     (src_clk_sel << TCLK_WDT_NS_SEL_SHIFT));
		break;
	case TCLK_WDT_S:
		rk_clrsetreg(&cru->clksel_con[46],
			     TCLK_WDT_NS_SEL_MASK,
			     (src_clk_sel << TCLK_WDT_NS_SEL_SHIFT));
		break;
	case TCLK_WDT_HPMCU:
		rk_clrsetreg(&cru->clksel_con[46],
			     TCLK_WDT_HPMCU_SEL_MASK,
			     (src_clk_sel << TCLK_WDT_HPMCU_SEL_SHIFT));
		break;
	case TCLK_WDT_LPMCU:
		if (rate == OSC_HZ)
			src_clk_sel = TCLK_WDT_LPMCU_SEL_OSC;
		else if (rate == RC_OSC_HZ)
			src_clk_sel = TCLK_WDT_LPMCU_SEL_RCOSC;
		else if (rate == 1000000)
			src_clk_sel = TCLK_WDT_LPMCU_SEL_100M;
		else
			src_clk_sel = TCLK_WDT_LPMCU_SEL_32K;
		rk_clrsetreg(&cru->pmu_clksel_con[3],
			     TCLK_WDT_LPMCU_SEL_MASK,
			     (src_clk_sel << TCLK_WDT_LPMCU_SEL_SHIFT));
		break;
	default:
		return -ENOENT;
	}
	return rv1126b_wdt_get_rate(priv, clk_id);
}

static ulong rv1126b_vop_get_rate(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, div, con, p_rate;

	switch (clk_id) {
	case DCLK_VOP:
		con = readl(&cru->clksel_con[43]);
		sel = (con & DCLK_VOP_SEL_MASK) >> DCLK_VOP_SEL_SHIFT;
		div = (con & DCLK_VOP_DIV_MASK) >> DCLK_VOP_DIV_SHIFT;
		break;
	default:
		return -ENOENT;
	}
	if (sel == DCLK_VOP_SEL_CPLL)
		p_rate = priv->cpll_hz;
	else
		p_rate = priv->gpll_hz;

	return DIV_TO_RATE(p_rate, div);
}

static ulong rv1126b_vop_set_rate(struct rv1126b_clk_priv *priv,
				 ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk, div, p_rate;

	if (!(priv->cpll_hz % rate)) {
		src_clk = DCLK_VOP_SEL_CPLL;
		p_rate = priv->cpll_hz;
	} else {
		src_clk = DCLK_VOP_SEL_GPLL;
		p_rate = priv->gpll_hz;
	}

	div = DIV_ROUND_UP(p_rate, rate);
	switch (clk_id) {
	case DCLK_VOP:
		rk_clrsetreg(&cru->clksel_con[43], DCLK_VOP_SEL_MASK | DCLK_VOP_DIV_MASK,
			     (src_clk << DCLK_VOP_SEL_SHIFT) |
			     ((div - 1) << DCLK_VOP_DIV_SHIFT) );
		break;
	default:
		return -ENOENT;
	}

	return rv1126b_vop_get_rate(priv, clk_id);
}

static ulong rv1126b_mac_get_rate(struct rv1126b_clk_priv *priv, ulong clk_id)
{
	struct rv1126b_cru *cru = priv->cru;
	u32 sel, div, con, p_rate;

	switch (clk_id) {
	case CLK_GMAC_PTP_REF_SRC:
	case CLK_GMAC_PTP_REF:
		con = readl(&cru->clksel_con[45]);
		sel = (con & CLK_GMAC_PTP_REF_SRC_SEL_MASK) >>
		      CLK_GMAC_PTP_REF_SRC_SEL_SHIFT;
		div = (con & CLK_GMAC_PTP_REF_SRC_DIV_MASK) >>
		      CLK_GMAC_PTP_REF_SRC_DIV_SHIFT;
		if (sel == CLK_GMAC_PTP_REF_SRC_SEL_CPLL)
			p_rate = priv->cpll_hz;
		else
			p_rate = OSC_HZ;
		break;
	case CLK_MAC_OUT2IO:
		con = readl(&cru->clksel_con[69]);
		sel = (con & CLK_MAC_OUT2IO_SEL_MASK) >>
		      CLK_MAC_OUT2IO_SEL_SHIFT;
		div = (con & CLK_MAC_OUT2IO_DIV_MASK) >>
		      CLK_MAC_OUT2IO_DIV_SHIFT;
		if (sel == CLK_MAC_OUT2IO_SEL_CPLL)
			p_rate = priv->cpll_hz;
		else if (sel == CLK_MAC_OUT2IO_SEL_GPLL)
			p_rate = priv->gpll_hz;
		else
			p_rate = OSC_HZ;
		break;
	case CLK_GMAC_125M:
		return priv->cpll_hz / 8;
	case CLK_50M_GMAC_IOBUF_VI:
		return priv->cpll_hz / 20;
	default:
		return -ENOENT;
	}

	return DIV_TO_RATE(p_rate, div);
}

static ulong rv1126b_mac_set_rate(struct rv1126b_clk_priv *priv,
				  ulong clk_id, ulong rate)
{
	struct rv1126b_cru *cru = priv->cru;
	int src_clk, div, p_rate;

	switch (clk_id) {
	case CLK_GMAC_PTP_REF_SRC:
	case CLK_GMAC_PTP_REF:
		if (!(priv->cpll_hz % rate)) {
			src_clk = CLK_GMAC_PTP_REF_SRC_SEL_CPLL;
			p_rate = priv->cpll_hz;
		} else {
			src_clk = CLK_GMAC_PTP_REF_SRC_SEL_24M;
			p_rate = OSC_HZ;
		}
		div = DIV_ROUND_UP(p_rate, rate);
		rk_clrsetreg(&cru->clksel_con[45],
			     CLK_GMAC_PTP_REF_SRC_DIV_MASK |
			     CLK_GMAC_PTP_REF_SRC_SEL_MASK,
			     (src_clk << CLK_GMAC_PTP_REF_SRC_SEL_SHIFT) |
			     ((div - 1) << CLK_GMAC_PTP_REF_SRC_DIV_SHIFT));
		break;
	case CLK_MAC_OUT2IO:
		if (!(priv->cpll_hz % rate)) {
			src_clk = CLK_MAC_OUT2IO_SEL_CPLL;
			p_rate = priv->cpll_hz;
		} else if (!(priv->gpll_hz % rate)) {
			src_clk = CLK_MAC_OUT2IO_SEL_GPLL;
			p_rate = priv->gpll_hz;
		} else {
			src_clk = CLK_MAC_OUT2IO_SEL_24M;
			p_rate = OSC_HZ;
		}
		div = DIV_ROUND_UP(p_rate, rate);
		rk_clrsetreg(&cru->clksel_con[69],
			     CLK_MAC_OUT2IO_DIV_MASK | CLK_MAC_OUT2IO_SEL_MASK,
			     (src_clk << CLK_MAC_OUT2IO_SEL_SHIFT) |
			     ((div - 1) << CLK_MAC_OUT2IO_DIV_SHIFT));
		writel(0x01000000, &cru->clkgate_con[15]);
		break;
	case CLK_GMAC_125M:
		return rv1126b_mac_get_rate(priv, clk_id);;
	case CLK_50M_GMAC_IOBUF_VI:
		return rv1126b_mac_get_rate(priv, clk_id);;
	default:
		return -ENOENT;
	}

	return rv1126b_mac_get_rate(priv, clk_id);
}

static ulong rv1126b_clk_get_rate(struct clk *clk)
{
	struct rv1126b_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	if (!priv->gpll_hz) {
		printf("%s gpll=%lu\n", __func__, priv->gpll_hz);
		return -ENOENT;
	}

	switch (clk->id) {
	case PLL_GPLL:
		rate = rockchip_pll_get_rate(&rv1126b_pll_clks[GPLL], priv->cru,
					     GPLL);
		break;
	case PLL_AUPLL:
		rate = rockchip_pll_get_rate(&rv1126b_pll_clks[AUPLL],
					     priv->cru, AUPLL);
		break;
	case PLL_CPLL:
		rate = rockchip_pll_get_rate(&rv1126b_pll_clks[CPLL], priv->cru,
					     CPLL);
		break;
	case ACLK_PERI_ROOT:
	case PCLK_PERI_ROOT:
	case ACLK_TOP_ROOT:
	case PCLK_TOP_ROOT:
	case PCLK_BUS_ROOT:
	case BUSCLK_PMU_SRC:
	case ACLK_BUS_ROOT:
	case HCLK_BUS_ROOT:
		rate = rv1126b_peri_get_clk(priv, clk->id);
		break;
	case PCLK_RKCE:
	case HCLK_NS_RKCE:
	case ACLK_RKCE_SRC:
	case ACLK_NSRKCE:
	case CLK_PKA_RKCE_SRC:
	case CLK_PKA_NSRKCE:
		rate = rv1126b_crypto_get_clk(priv, clk->id);
		break;
	case CCLK_SDMMC0:
	case HCLK_SDMMC0:
	case CCLK_SDMMC1:
	case HCLK_SDMMC1:
	case CCLK_EMMC:
	case HCLK_EMMC:
	case SCLK_2X_FSPI0:
	case HCLK_FSPI0:
	case HCLK_XIP_FSPI0:
	case SCLK_1X_FSPI1:
	case HCLK_FSPI1:
	case HCLK_XIP_FSPI1:
		rate = rv1126b_mmc_get_clk(priv, clk->id);
		break;
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
	case CLK_I2C2:
	case CLK_I2C_BUS_SRC:
		rate = rv1126b_i2c_get_clk(priv, clk->id);
		break;
	case CLK_SPI0:
	case CLK_SPI1:
		rate = rv1126b_spi_get_clk(priv, clk->id);
		break;
	case CLK_PWM0:
	case CLK_PWM2:
	case CLK_PWM3:
	case CLK_PWM1:
		rate = rv1126b_pwm_get_clk(priv, clk->id);
		break;
	case CLK_SARADC0:
	case CLK_SARADC0_SRC:
	case CLK_SARADC1:
	case CLK_SARADC1_SRC:
	case CLK_SARADC2:
	case CLK_SARADC2_SRC:
	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		rate = rv1126b_adc_get_clk(priv, clk->id);
		break;
	case CLK_CM_FRAC0:
	case CLK_CM_FRAC1:
	case CLK_CM_FRAC2:
	case CLK_UART_FRAC0:
	case CLK_UART_FRAC1:
	case CLK_AUDIO_FRAC0:
	case CLK_AUDIO_FRAC1:
		rate = rv1126b_frac_get_rate(priv, clk->id);
		break;
	case SCLK_UART0:
	case SCLK_UART0_SRC:
	case SCLK_UART1:
	case SCLK_UART2:
	case SCLK_UART3:
	case SCLK_UART4:
	case SCLK_UART5:
	case SCLK_UART6:
	case SCLK_UART7:
		rate = rv1126b_uart_get_rate(priv, clk->id);
		break;
	case DCLK_DECOM:
		rate = 400 * MHz;
		break;
	case TCLK_WDT_NS_SRC:
	case TCLK_WDT_NS:
	case TCLK_WDT_S:
	case TCLK_WDT_HPMCU:
	case TCLK_WDT_LPMCU:
		rate = rv1126b_wdt_get_rate(priv, clk->id);
		break;
	case DCLK_VOP:
		rate = rv1126b_vop_get_rate(priv, clk->id);
		break;
	case CLK_GMAC_125M:
	case CLK_GMAC_PTP_REF_SRC:
	case CLK_50M_GMAC_IOBUF_VI:
	case CLK_MAC_OUT2IO:
	case CLK_GMAC_PTP_REF:
		rate = rv1126b_mac_get_rate(priv, clk->id);
		break;

	default:
		return -ENOENT;
	}

	return rate;
};

static ulong rv1126b_clk_set_rate(struct clk *clk, ulong rate)
{
	struct rv1126b_clk_priv *priv = dev_get_priv(clk->dev);
	ulong ret = 0;

	if (!priv->gpll_hz) {
		printf("%s gpll=%lu\n", __func__, priv->gpll_hz);
		return -ENOENT;
	}

	switch (clk->id) {
	case PLL_GPLL:
		ret = rockchip_pll_set_rate(&rv1126b_pll_clks[GPLL], priv->cru,
					    GPLL, rate);
		break;
	case PLL_AUPLL:
		ret = rockchip_pll_set_rate(&rv1126b_pll_clks[AUPLL], priv->cru,
					    AUPLL, rate);
		break;
	case PLL_CPLL:
		ret = rockchip_pll_set_rate(&rv1126b_pll_clks[CPLL], priv->cru,
					    CPLL, rate);
		break;
	case ACLK_PERI_ROOT:
	case PCLK_PERI_ROOT:
	case ACLK_TOP_ROOT:
	case PCLK_TOP_ROOT:
	case PCLK_BUS_ROOT:
	case BUSCLK_PMU_SRC:
	case ACLK_BUS_ROOT:
	case HCLK_BUS_ROOT:
		ret = rv1126b_peri_set_clk(priv, clk->id, rate);
		break;
	case PCLK_RKCE:
	case HCLK_NS_RKCE:
	case ACLK_RKCE_SRC:
	case ACLK_NSRKCE:
	case CLK_PKA_RKCE_SRC:
	case CLK_PKA_NSRKCE:
		ret = rv1126b_crypto_set_clk(priv, clk->id, rate);
		break;
	case CCLK_SDMMC0:
	case HCLK_SDMMC0:
	case CCLK_SDMMC1:
	case HCLK_SDMMC1:
	case CCLK_EMMC:
	case HCLK_EMMC:
	case SCLK_2X_FSPI0:
	case HCLK_FSPI0:
	case HCLK_XIP_FSPI0:
	case SCLK_1X_FSPI1:
	case HCLK_FSPI1:
	case HCLK_XIP_FSPI1:
		ret = rv1126b_mmc_set_clk(priv, clk->id, rate);
		break;
	case CLK_I2C0:
	case CLK_I2C1:
	case CLK_I2C3:
	case CLK_I2C4:
	case CLK_I2C5:
	case CLK_I2C2:
	case CLK_I2C_BUS_SRC:
		ret = rv1126b_i2c_set_clk(priv, clk->id, rate);
		break;
	case CLK_SPI0:
	case CLK_SPI1:
		ret = rv1126b_spi_set_clk(priv, clk->id, rate);
		break;
	case CLK_PWM0:
	case CLK_PWM2:
	case CLK_PWM3:
	case CLK_PWM1:
		ret = rv1126b_pwm_set_clk(priv, clk->id, rate);
		break;
	case CLK_SARADC0:
	case CLK_SARADC0_SRC:
	case CLK_SARADC1:
	case CLK_SARADC1_SRC:
	case CLK_SARADC2:
	case CLK_SARADC2_SRC:
	case CLK_TSADC:
	case CLK_TSADC_PHYCTRL:
		ret = rv1126b_adc_set_clk(priv, clk->id, rate);
		break;
	case CLK_CM_FRAC0:
	case CLK_CM_FRAC1:
	case CLK_CM_FRAC2:
	case CLK_UART_FRAC0:
	case CLK_UART_FRAC1:
	case CLK_AUDIO_FRAC0:
	case CLK_AUDIO_FRAC1:
		ret = rv1126b_frac_set_rate(priv, clk->id, rate);
		break;
	case SCLK_UART0:
	case SCLK_UART0_SRC:
	case SCLK_UART1:
	case SCLK_UART2:
	case SCLK_UART3:
	case SCLK_UART4:
	case SCLK_UART5:
	case SCLK_UART6:
	case SCLK_UART7:
		ret = rv1126b_uart_set_rate(priv, clk->id, rate);
		break;
	case DCLK_DECOM:
		break;
	case TCLK_WDT_NS_SRC:
	case TCLK_WDT_NS:
	case TCLK_WDT_S:
	case TCLK_WDT_HPMCU:
	case TCLK_WDT_LPMCU:
		ret = rv1126b_wdt_set_rate(priv, clk->id, rate);
		break;
	case DCLK_VOP:
		ret = rv1126b_vop_set_rate(priv, clk->id, rate);
		break;
	case CLK_GMAC_125M:
	case CLK_GMAC_PTP_REF_SRC:
	case CLK_50M_GMAC_IOBUF_VI:
	case CLK_MAC_OUT2IO:
	case CLK_GMAC_PTP_REF:
		ret = rv1126b_mac_set_rate(priv, clk->id, rate);
		break;

	default:
		return -ENOENT;
	}

	return ret;
};

static int rv1126b_clk_set_parent(struct clk *clk, struct clk *parent)
{
	switch (clk->id) {
	default:
		return -ENOENT;
	}

	return 0;
}

static int rv1126b_clk_enable(struct clk *clk)
{
	ulong ret = 0;

	switch (clk->id) {
#ifdef CONFIG_SPL_BUILD
	case PCLK_KEY_READER_S:
		ret = writel(BITS_WITH_WMASK(0, 0x1U, 13),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(0));
		break;
	case HCLK_KL_RKCE_S:
		ret = writel(BITS_WITH_WMASK(0, 0x1U, 9),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(0));
		break;
	case HCLK_RKCE_S:
		ret = writel(BITS_WITH_WMASK(0, 0x1U, 8),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(0));
		break;
	case HCLK_RKRNG_S:
		ret = writel(BITS_WITH_WMASK(0, 0x1U, 14),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(2));
		break;
	case CLK_PKA_RKCE_S:
		ret = writel(BITS_WITH_WMASK(0, 0x1U, 13),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(2));
		break;
	case ACLK_RKCE_S:
		ret = writel(BITS_WITH_WMASK(0, 0x1U, 12),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(2));
		break;
#endif
	default:
		return ret;
	}
	return ret;
}

static int rv1126b_clk_disable(struct clk *clk)
{
	ulong ret = 0;

	switch (clk->id) {
#ifdef CONFIG_SPL_BUILD
	case PCLK_KEY_READER_S:
		ret = writel(BITS_WITH_WMASK(1, 0x1U, 13),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(0));
		break;
	case HCLK_KL_RKCE_S:
		ret = writel(BITS_WITH_WMASK(1, 0x1U, 9),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(0));
		break;
	case HCLK_RKCE_S:
		ret = writel(BITS_WITH_WMASK(1, 0x1U, 8),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(0));
		break;
	case HCLK_RKRNG_S:
		ret = writel(BITS_WITH_WMASK(1, 0x1U, 14),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(2));
		break;
	case CLK_PKA_RKCE_S:
		ret = writel(BITS_WITH_WMASK(1, 0x1U, 13),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(2));
		break;
	case ACLK_RKCE_S:
		ret = writel(BITS_WITH_WMASK(1, 0x1U, 12),
			     RV1126B_CRU_BASE + RV1126B_SBUSCLKGATE_CON(2));
		break;
#endif
	default:
		return ret;
	}
	return ret;
}

static struct clk_ops rv1126b_clk_ops = {
	.get_rate = rv1126b_clk_get_rate,
	.set_rate = rv1126b_clk_set_rate,
#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
	.set_parent = rv1126b_clk_set_parent,
#endif
	.enable = rv1126b_clk_enable,
	.disable = rv1126b_clk_disable,
};

static void rv1126b_clk_init(struct rv1126b_clk_priv *priv)
{
	int ret;

	priv->sync_kernel = false;
	priv->gpll_hz = rockchip_pll_get_rate(&rv1126b_pll_clks[GPLL],
					      priv->cru, GPLL);
	if (priv->gpll_hz != GPLL_HZ) {
		ret = rockchip_pll_set_rate(&rv1126b_pll_clks[GPLL], priv->cru,
					    GPLL, GPLL_HZ);
		if (!ret)
			priv->gpll_hz = GPLL_HZ;
	}
	priv->aupll_hz = rockchip_pll_get_rate(&rv1126b_pll_clks[AUPLL],
					       priv->cru, AUPLL);
	if (priv->aupll_hz != AUPLL_HZ) {
		ret = rockchip_pll_set_rate(&rv1126b_pll_clks[AUPLL], priv->cru,
					    AUPLL, AUPLL_HZ);
		if (!ret)
			priv->aupll_hz = AUPLL_HZ;
	}
	priv->cpll_hz = rockchip_pll_get_rate(&rv1126b_pll_clks[CPLL],
					      priv->cru, CPLL);
	if (priv->cpll_hz != CPLL_HZ) {
		ret = rockchip_pll_set_rate(&rv1126b_pll_clks[CPLL], priv->cru,
					    CPLL, CPLL_HZ);
		if (!ret)
			priv->cpll_hz = CPLL_HZ;
	}
}

static int rv1126b_clk_probe(struct udevice *dev)
{
	struct rv1126b_clk_priv *priv = dev_get_priv(dev);
	int ret;

#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
	/* fix gpll and some clks modify by maskrom */
	writel(BITS_WITH_WMASK(11, 0x1fU, 5),
	       RV1126B_CRU_BASE + RV1126B_CLKSEL_CON(1));
	writel(BITS_WITH_WMASK(1, 0x1U, 15),
	       RV1126B_CRU_BASE + RV1126B_CLKSEL_CON(1));
	writel(BITS_WITH_WMASK(5, 0x1fU, 5),
	       RV1126B_CRU_BASE + RV1126B_CLKSEL_CON(2));
	writel(BITS_WITH_WMASK(1, 0x7U, 0),
	       RV1126B_CRU_BASE + RV1126B_CLKSEL_CON(60));
	writel(BITS_WITH_WMASK(1, 0x7U, 6),
	       RV1126B_CRU_BASE + RV1126B_PLL_CON(9));
	writel(BITS_WITH_WMASK(1, 0x3U, 4),
	       RV1126B_CRU_BASE + RV1126B_MODE_CON);
	/* Set clk_pka_rkce to 198M */
	writel(BITS_WITH_WMASK(1, 0x1U, 12),
	       RV1126B_CRU_BASE + RV1126B_CLKSEL_CON(50));
#endif

	rv1126b_clk_init(priv);

	/* Process 'assigned-{clocks/clock-parents/clock-rates}' properties */
	ret = clk_set_defaults(dev);
	if (ret)
		debug("%s clk_set_defaults failed %d\n", __func__, ret);
	else
		priv->sync_kernel = true;
	return 0;
}

static int rv1126b_clk_ofdata_to_platdata(struct udevice *dev)
{
	struct rv1126b_clk_priv *priv = dev_get_priv(dev);

	priv->cru = dev_read_addr_ptr(dev);

	return 0;
}

static int rv1126b_clk_bind(struct udevice *dev)
{
	int ret;
	struct udevice *sys_child, *sf_child;
	struct sysreset_reg *priv;
	struct softreset_reg *sf_priv;

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(dev, "rockchip_sysreset", "sysreset",
				 &sys_child);
	if (ret) {
		debug("Warning: No sysreset driver: ret=%d\n", ret);
	} else {
		priv = malloc(sizeof(struct sysreset_reg));
		priv->glb_srst_fst_value = offsetof(struct rv1126b_cru,
						    glb_srst_fst);
		priv->glb_srst_snd_value = offsetof(struct rv1126b_cru,
						    glb_srst_snd);
		sys_child->priv = priv;
	}

	ret = device_bind_driver_to_node(dev, "rockchip_reset", "reset",
					 dev_ofnode(dev), &sf_child);
	if (ret) {
		debug("Warning: No rockchip reset driver: ret=%d\n", ret);
	} else {
		sf_priv = malloc(sizeof(struct softreset_reg));
		sf_priv->sf_reset_offset = offsetof(struct rv1126b_cru,
						    softrst_con[0]);
		sf_priv->sf_reset_num = CLK_NR_SRST;
		sf_child->priv = sf_priv;
	}

	return 0;
}

static const struct udevice_id rv1126b_clk_ids[] = {
	{ .compatible = "rockchip,rv1126b-cru" },
	{ }
};

U_BOOT_DRIVER(rockchip_rv1126b_cru) = {
	.name		= "rockchip_rv1126b_cru",
	.id		= UCLASS_CLK,
	.of_match	= rv1126b_clk_ids,
	.priv_auto_alloc_size = sizeof(struct rv1126b_clk_priv),
	.ofdata_to_platdata = rv1126b_clk_ofdata_to_platdata,
	.ops		= &rv1126b_clk_ops,
	.bind		= rv1126b_clk_bind,
	.probe		= rv1126b_clk_probe,
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
	struct rv1126b_clk_priv *priv;
	const struct rv1126b_clk_info *clk_dump;
	struct clk clk;
	unsigned long clk_count = ARRAY_SIZE(clks_dump);
	unsigned long rate;
	int i, ret;

	ret = uclass_get_device_by_driver(UCLASS_CLK,
					  DM_GET_DRIVER(rockchip_rv1126b_cru),
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
			if (clk_dump->is_cru)
				ret = clk_request(cru_dev, &clk);
			if (ret < 0)
				return ret;

			rate = clk_get_rate(&clk);
			clk_free(&clk);
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
