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
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/hardware.h>
#include <dt-bindings/clock/rockchip,rk182x-cru.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

#define DIV_TO_RATE(input_rate, div)    ((input_rate) / ((div) + 1))

#define MHz		1000000
#define KHz		1000
#define OSC_HZ		(24 * MHz)

#define GPLL_HZ		(1000 * MHz)
#define MPLL_HZ		(800 * MHz)
#define L0PLL_HZ	(1500 * MHz)

/* RK3538 pll id */
enum rk182x_pll_id {
	GPLL,
	MPLL,
	DPLL,
	L0PLL,
	PLL_COUNT,
};

struct rk182x_cru {
	u32 mpll_con[5];                        /* Address Offset: 0x0000 */
	u32 reserved0014[11];                   /* Address Offset: 0x0014 */
	u32 dpll_con[5];                        /* Address Offset: 0x0040 */
	u32 reserved0054[3];                    /* Address Offset: 0x0054 */
	u32 gpll_con[5];                        /* Address Offset: 0x0060 */
	u32 reserved0074[3];                    /* Address Offset: 0x0074 */
	u32 l0pll_con[5];                       /* Address Offset: 0x0080 */
	u32 reserved0094[123];                  /* Address Offset: 0x0094 */
	u32 mode_con[2];                        /* Address Offset: 0x0280 */
	u32 reserved0288[6];                    /* Address Offset: 0x0288 */
	u32 protect_top_con;                    /* Address Offset: 0x02A0 */
	u32 protect_peri_con;                   /* Address Offset: 0x02A4 */
	u32 protect_php_con;                    /* Address Offset: 0x02A8 */
	u32 protect_cpu_con;                    /* Address Offset: 0x02AC */
	u32 reserved02B0[2];                    /* Address Offset: 0x02B0 */
	u32 protect_pmu_con;                    /* Address Offset: 0x02B8 */
	u32 protect_video_con;                  /* Address Offset: 0x02BC */
	u32 reserved02C0[16];                   /* Address Offset: 0x02C0 */
	u32 clksel_con[83];                     /* Address Offset: 0x0300 */
	u32 reserved044C[237];                  /* Address Offset: 0x044C */
	u32 gate_con[36];                       /* Address Offset: 0x0800 */
	u32 reserved0890[92];                   /* Address Offset: 0x0890 */
	u32 softrst_con[51];                    /* Address Offset: 0x0A00 */
	u32 reserved0ACC[77];                   /* Address Offset: 0x0ACC */
	u32 glb_cnt_th;                         /* Address Offset: 0x0C00 */
	u32 glb_rst_st;                         /* Address Offset: 0x0C04 */
	u32 glb_srst_fst;                       /* Address Offset: 0x0C08 */
	u32 glb_srst_snd;                       /* Address Offset: 0x0C0C */
	u32 reserved0C10;                       /* Address Offset: 0x0C10 */
	u32 GLBRST_ST_NCLR;                     /* Address Offset: 0x0C14 */
	u32 glb_rst_con0;                       /* Address Offset: 0x0C18 */
	u32 glb_rst_con1;                       /* Address Offset: 0x0C1C */
	u32 glb_rst_con2;                       /* Address Offset: 0x0C20 */
};

struct rk182x_clk_info {
	unsigned long id;
	char *name;
};

struct rk182x_clk_priv {
	struct rk182x_cru *cru;
	ulong gpll_hz;
	ulong mpll_hz;
	ulong dpll_hz;
	ulong l0pll_hz;
	bool sync_kernel;
};

#define RK182X_CRU_BASE               	0x1030800000
#define RK182X_PLL_CON(x)               ((x) * 0x4)
#define RK182X_MODE_CON0                0x280
#define RK182X_MODE_CON1                0x284
#define RK182X_GLB_SRST_FST             0xc08
#define RK182X_GLB_SRST_SND             0xc0c
#define RK182X_CLKSEL_CON(x)            ((x) * 0x4 + 0x300)
#define RK182X_CLKGATE_CON(x)           ((x) * 0x4 + 0x800)
#define RK182X_SOFTRST_CON(x)           ((x) * 0x4 + 0xa00)

#define CCLK_SRC_SDMMC0_SEL_SHIFT	6
#define CCLK_SRC_SDMMC0_SEL_MASK	0x7 << CCLK_SRC_SDMMC0_SEL_SHIFT
#define CCLK_SRC_SDMMC0_DIV_SHIFT	0
#define CCLK_SRC_SDMMC0_DIV_MASK	0x3f << CCLK_SRC_SDMMC0_DIV_SHIFT
#define SCLK_FSPI_X2_SEL_SHIFT		8
#define SCLK_FSPI_X2_SEL_MASK		0x7 << SCLK_FSPI_X2_SEL_SHIFT
#define SCLK_FSPI_X2_DIV_SHIFT		2
#define SCLK_FSPI_X2_DIV_MASK		0x3f << SCLK_FSPI_X2_DIV_SHIFT

static struct rockchip_pll_rate_table rk182x_pll_rates[] = {
	/* _mhz, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _frac */
	RK3036_PLL_RATE(1500000000, 1, 125, 2, 1, 1, 0),
	RK3036_PLL_RATE(1200000000, 1, 100, 2, 1, 1, 0),
	RK3036_PLL_RATE(1188000000, 1, 99, 2, 1, 1, 0),
	RK3036_PLL_RATE(1000000000, 3, 250, 2, 1, 1, 0),
	RK3036_PLL_RATE(800000000, 3, 200, 2, 1, 1, 0),
	RK3036_PLL_RATE(750000000, 2, 125, 2, 1, 1, 0),
	RK3036_PLL_RATE(700000000, 3, 175, 2, 1, 1, 0),
	{ /* sentinel */ },
};

static struct rockchip_pll_clock rk182x_pll_clks[] = {
	[GPLL] = PLL(pll_rk3328, PLL_GPLL, RK182X_PLL_CON(24),
		     RK182X_MODE_CON0, 2, 10, 0, rk182x_pll_rates),
	[MPLL] = PLL(pll_rk3328, PLL_MPLL, RK182X_PLL_CON(0),
		     RK182X_MODE_CON1, 0, 10, 0, rk182x_pll_rates),
	[DPLL] = PLL(pll_rk3328, PLL_DPLL, RK182X_PLL_CON(16),
		     RK182X_MODE_CON1, 2, 10, 0, rk182x_pll_rates),
	[L0PLL] = PLL(pll_rk3328, PLL_L0PLL, RK182X_PLL_CON(32),
		      RK182X_CLKSEL_CON(0), 5, 10, 0, rk182x_pll_rates),
};

#define RK182X_CLK_DUMP(_id, _name)		\
{						\
	.id = _id,				\
	.name = _name,				\
}

static const struct rk182x_clk_info clks_dump[] = {
	RK182X_CLK_DUMP(PLL_GPLL, "gpll"),
	RK182X_CLK_DUMP(PLL_MPLL, "mpll"),
	RK182X_CLK_DUMP(PLL_DPLL, "dpll"),
	RK182X_CLK_DUMP(PLL_L0PLL, "l0pll"),
	RK182X_CLK_DUMP(CCLK_SRC_SDMMC0, "cclk_src_sdmmc0"),
	RK182X_CLK_DUMP(SCLK_FSPI_X2, "sclk_fspi_x2"),
};

static ulong rk182x_mmc_get_clk(struct rk182x_clk_priv *priv, ulong clk_id)
{
	struct rk182x_cru *cru = priv->cru;
	u32 con, div, sel;
	ulong prate;

	switch (clk_id) {
	case CCLK_SRC_SDMMC0:
	case HCLK_SDMMC0:
		con = readl(&cru->clksel_con[51]);
		div = (con & CCLK_SRC_SDMMC0_DIV_MASK) >> CCLK_SRC_SDMMC0_DIV_SHIFT;
		sel = (con & CCLK_SRC_SDMMC0_SEL_MASK) >> CCLK_SRC_SDMMC0_SEL_SHIFT;
		break;
	case SCLK_FSPI_X2:
	case HCLK_FSPI:
		con = readl(&cru->clksel_con[53]);
		div = (con & SCLK_FSPI_X2_DIV_MASK) >> SCLK_FSPI_X2_DIV_SHIFT;
		sel = (con & SCLK_FSPI_X2_SEL_MASK) >> SCLK_FSPI_X2_SEL_SHIFT;
		break;

	default:
		return -ENOENT;
	}
	if (sel == 0)
		prate = priv->gpll_hz;
	else if (sel == 1)
		prate = priv->mpll_hz;
	else if (sel == 2)
		prate = priv->dpll_hz;
	else if (sel == 3)
		prate = priv->l0pll_hz;
	else
		prate = OSC_HZ;

	return DIV_TO_RATE(prate, div);
}

static ulong rk182x_mmc_set_clk(struct rk182x_clk_priv *priv,
				ulong clk_id, ulong rate)
{
	struct rk182x_cru *cru = priv->cru;
	u32 div, sel;

	if (OSC_HZ % rate == 0) {
		div = DIV_ROUND_UP(OSC_HZ, rate);
		sel = 4;
	} else if ((priv->dpll_hz % rate) == 0) {
		div = DIV_ROUND_UP(priv->dpll_hz, rate);
		sel = 2;
	} else if ((priv->mpll_hz % rate) == 0) {
		div = DIV_ROUND_UP(priv->mpll_hz, rate);
		sel = 1;
	} else if ((priv->l0pll_hz % rate) == 0) {
		div = DIV_ROUND_UP(priv->l0pll_hz, rate);
		sel = 3;
	} else {
		div = DIV_ROUND_UP(priv->gpll_hz, rate);
		sel = 0;
	}

	assert(div - 1 <= 64);
	switch (clk_id) {
	case CCLK_SRC_SDMMC0:
	case HCLK_SDMMC0:
		rk_clrsetreg(&cru->clksel_con[51],
			     CCLK_SRC_SDMMC0_DIV_MASK | CCLK_SRC_SDMMC0_SEL_MASK,
			     (sel << CCLK_SRC_SDMMC0_SEL_SHIFT) |
			     ((div - 1) << CCLK_SRC_SDMMC0_DIV_SHIFT));
		break;
	case SCLK_FSPI_X2:
	case HCLK_FSPI:
		rk_clrsetreg(&cru->clksel_con[53],
			     SCLK_FSPI_X2_DIV_MASK | SCLK_FSPI_X2_SEL_MASK,
			     (sel << SCLK_FSPI_X2_SEL_SHIFT) |
			     ((div - 1) << SCLK_FSPI_X2_DIV_SHIFT));
		break;

	default:
		return -ENOENT;
	}

	return rk182x_mmc_get_clk(priv, clk_id);
}

static ulong rk182x_clk_get_rate(struct clk *clk)
{
	struct rk182x_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	if (!priv->gpll_hz || !priv->mpll_hz) {
		printf("%s: gpll=%lu, mpll=%ld\n",
		       __func__, priv->gpll_hz, priv->mpll_hz);
		return -ENOENT;
	}

	switch (clk->id) {
	case PLL_GPLL:
		rate = rockchip_pll_get_rate(&rk182x_pll_clks[GPLL], priv->cru,
					     GPLL);
		break;
	case PLL_MPLL:
		rate = rockchip_pll_get_rate(&rk182x_pll_clks[MPLL], priv->cru,
					     MPLL);
		break;
	case PLL_DPLL:
		rate = rockchip_pll_get_rate(&rk182x_pll_clks[DPLL], priv->cru,
					     DPLL);
		break;
	case PLL_L0PLL:
		rate = rockchip_pll_get_rate(&rk182x_pll_clks[L0PLL], priv->cru,
					     L0PLL) / 2;
		break;

	case TCLK_WDT0:
		rate = OSC_HZ;
		break;
	case CCLK_SRC_SDMMC0:
	case HCLK_SDMMC0:
	case SCLK_FSPI_X2:
	case HCLK_FSPI:
		rate = rk182x_mmc_get_clk(priv, clk->id);
		break;
	default:
		return -ENOENT;
	}

	return rate;
};

static ulong rk182x_clk_set_rate(struct clk *clk, ulong rate)
{
	struct rk182x_clk_priv *priv = dev_get_priv(clk->dev);
	ulong ret = 0;

	if (!priv->gpll_hz) {
		printf("%s gpll=%lu\n", __func__, priv->gpll_hz);
		return -ENOENT;
	}

	switch (clk->id) {
	case PLL_GPLL:
		ret = rockchip_pll_set_rate(&rk182x_pll_clks[GPLL], priv->cru,
					    GPLL, rate);
		priv->gpll_hz = rockchip_pll_get_rate(&rk182x_pll_clks[GPLL],
						      priv->cru, GPLL);
	case PLL_MPLL:
		ret = rockchip_pll_set_rate(&rk182x_pll_clks[MPLL], priv->cru,
					    MPLL, rate);
		priv->mpll_hz = rockchip_pll_get_rate(&rk182x_pll_clks[MPLL],
						      priv->cru, MPLL);
		break;
	case PLL_L0PLL:
		ret = rockchip_pll_set_rate(&rk182x_pll_clks[L0PLL], priv->cru,
					    L0PLL, rate);
		priv->l0pll_hz = rockchip_pll_get_rate(&rk182x_pll_clks[L0PLL],
						       priv->cru, L0PLL) / 2;
		break;
	case TCLK_WDT0:
		return (rate == OSC_HZ) ? 0 : -EINVAL;
	case CCLK_SRC_SDMMC0:
	case HCLK_SDMMC0:
	case SCLK_FSPI_X2:
	case HCLK_FSPI:
		ret = rk182x_mmc_set_clk(priv, clk->id, rate);
		break;
	default:
		return -ENOENT;
	}

	return ret;
};

static struct clk_ops rk182x_clk_ops = {
	.get_rate = rk182x_clk_get_rate,
	.set_rate = rk182x_clk_set_rate,
};

static int rk182x_clk_init(struct rk182x_clk_priv *priv)
{
	int ret;

	priv->sync_kernel = false;

	if (priv->gpll_hz != GPLL_HZ) {
		ret = rockchip_pll_set_rate(&rk182x_pll_clks[GPLL], priv->cru,
					    GPLL, GPLL_HZ);
		if (!ret)
			priv->gpll_hz = GPLL_HZ;
	}
	if (priv->mpll_hz != MPLL_HZ) {
		ret = rockchip_pll_set_rate(&rk182x_pll_clks[MPLL], priv->cru,
					    MPLL, MPLL_HZ);
		if (!ret)
			priv->mpll_hz = MPLL_HZ;
	}
	if (priv->l0pll_hz != L0PLL_HZ) {
		ret = rockchip_pll_set_rate(&rk182x_pll_clks[L0PLL], priv->cru,
					    L0PLL, L0PLL_HZ);
		if (!ret)
			priv->l0pll_hz = L0PLL_HZ / 2;
	}
	priv->dpll_hz = rockchip_pll_get_rate(&rk182x_pll_clks[DPLL],
					      priv->cru, DPLL);
	return 0;
}

static int rk182x_clk_probe(struct udevice *dev)
{
	struct rk182x_clk_priv *priv = dev_get_priv(dev);
	int ret;

	ret = rk182x_clk_init(priv);
	if (ret)
		return ret;

	/* Process 'assigned-{clocks/clock-parents/clock-rates}' properties */
	ret = clk_set_defaults(dev, 1);
	if (ret)
		debug("%s clk_set_defaults failed %d\n", __func__, ret);
	else
		priv->sync_kernel = true;

	return 0;
}

static int rk182x_clk_ofdata_to_platdata(struct udevice *dev)
{
	struct rk182x_clk_priv *priv = dev_get_priv(dev);

	priv->cru = dev_read_addr_ptr(dev);

	return 0;
}

static int rk182x_clk_bind(struct udevice *dev)
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
		priv->glb_srst_fst_value = offsetof(struct rk182x_cru,
						    glb_srst_fst);
		priv->glb_srst_snd_value = offsetof(struct rk182x_cru,
						    glb_srst_snd);
		dev_set_priv(sys_child, priv);
	}

#if CONFIG_IS_ENABLED(RESET_ROCKCHIP)
	ret = offsetof(struct rk182x_cru, softrst_con[0]);
	ret = rockchip_reset_bind(dev, ret, 51);
	if (ret)
		debug("Warning: software reset driver bind failed\n");
#endif

	return 0;
}

static const struct udevice_id rk182x_clk_ids[] = {
	{ .compatible = "rockchip,rk182x-cru" },
	{ }
};

U_BOOT_DRIVER(rockchip_rk182x_cru) = {
	.name		= "rockchip_rk182x_cru",
	.id		= UCLASS_CLK,
	.of_match	= rk182x_clk_ids,
	.priv_auto	= sizeof(struct rk182x_clk_priv),
	.of_to_plat	= rk182x_clk_ofdata_to_platdata,
	.ops		= &rk182x_clk_ops,
	.bind		= rk182x_clk_bind,
	.probe		= rk182x_clk_probe,
};

/**
 * soc_clk_dump() - Print clock frequencies
 * Returns zero on success
 *
 * Implementation for the clk dump command.
 */
int soc_clk_dump(void)
{
	const struct rk182x_clk_info *clk_dump;
	struct rk182x_clk_priv *priv;
	struct udevice *cru_dev;
	struct clk clk;
	ulong clk_count = ARRAY_SIZE(clks_dump);
	ulong rate;
	int i, ret;

	ret = uclass_get_device_by_driver(UCLASS_CLK,
					  DM_DRIVER_GET(rockchip_rk182x_cru),
					  &cru_dev);
	if (ret) {
		printf("%s failed to get cru device\n", __func__);
		return ret;
	}

	printf("Clock:\n");
	priv = dev_get_priv(cru_dev);
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
