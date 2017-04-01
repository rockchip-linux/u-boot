/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <resource.h>
#include <asm/arch/rkplat.h>
#include <asm/unaligned.h>
#include <linux/list.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip-dw-mipi-dsi.h"
#include "rockchip_phy.h"

#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

#define BIT(x)		(1 << (x))

#define INNO_PHY_LANE_CTRL	0x00000
#define INNO_PHY_POWER_CTRL	0x00004
#define INNO_PHY_PLL_CTRL_0	0x0000c
#define INNO_PHY_PLL_CTRL_1	0x00010
#define INNO_PHY_DIG_CTRL	0x00080
#define INNO_PHY_PIN_CTRL	0x00084

#define INNO_CLOCK_LANE_REG_BASE	0x00100
#define INNO_DATA_LANE_0_REG_BASE	0x00180
#define INNO_DATA_LANE_1_REG_BASE	0x00200
#define INNO_DATA_LANE_2_REG_BASE	0x00280
#define INNO_DATA_LANE_3_REG_BASE	0x00300

#define T_LPX_OFFSET		0x00014
#define T_HS_PREPARE_OFFSET	0x00018
#define T_HS_ZERO_OFFSET	0x0001c
#define T_HS_TRAIL_OFFSET	0x00020
#define T_HS_EXIT_OFFSET	0x00024
#define T_CLK_POST_OFFSET	0x00028
#define T_WAKUP_H_OFFSET	0x00030
#define T_WAKUP_L_OFFSET	0x00034
#define T_CLK_PRE_OFFSET	0x00038
#define T_TA_GO_OFFSET		0x00040
#define T_TA_SURE_OFFSET	0x00044
#define T_TA_WAIT_OFFSET	0x00048

#define CLK_LANE_EN_MASK	BIT(6)
#define DATA_LANE_3_EN_MASK	BIT(5)
#define DATA_LANE_2_EN_MASK	BIT(4)
#define DATA_LANE_1_EN_MASK	BIT(3)
#define DATA_LANE_0_EN_MASK	BIT(2)
#define CLK_LANE_EN		BIT(6)
#define DATA_LANE_3_EN		BIT(5)
#define DATA_LANE_2_EN		BIT(4)
#define DATA_LANE_1_EN		BIT(3)
#define DATA_LANE_0_EN		BIT(2)
#define FBDIV_8(x)		(((x) & 0x1) << 5)
#define PREDIV(x)		(((x) & 0x1f) << 0)
#define FBDIV_7_0(x)		(((x) & 0xff) << 0)
#define T_LPX(x)		(((x) & 0x3f) << 0)
#define T_HS_PREPARE(x)		(((x) & 0x7f) << 0)
#define T_HS_ZERO(x)		(((x) & 0x3f) << 0)
#define T_HS_TRAIL(x)		(((x) & 0x7f) << 0)
#define T_HS_EXIT(x)		(((x) & 0x1f) << 0)
#define T_CLK_POST(x)		(((x) & 0xf) << 0)
#define T_WAKUP_H(x)		(((x) & 0x3) << 0)
#define T_WAKUP_L(x)		(((x) & 0xff) << 0)
#define T_CLK_PRE(x)		(((x) & 0xf) << 0)
#define T_TA_GO(x)		(((x) & 0x3f) << 0)
#define T_TA_SURE(x)		(((x) & 0x3f) << 0)
#define T_TA_WAIT(x)		(((x) & 0x3f) << 0)

enum lane_type {
	CLOCK_LANE,
	DATA_LANE_0,
	DATA_LANE_1,
	DATA_LANE_2,
	DATA_LANE_3,
};

static const u32 lane_reg_offset[] = {
	[CLOCK_LANE] = INNO_CLOCK_LANE_REG_BASE,
	[DATA_LANE_0] = INNO_DATA_LANE_0_REG_BASE,
	[DATA_LANE_1] = INNO_DATA_LANE_1_REG_BASE,
	[DATA_LANE_2] = INNO_DATA_LANE_2_REG_BASE,
	[DATA_LANE_3] = INNO_DATA_LANE_3_REG_BASE,
};

enum hs_clk_range {
	HS_CLK_RANGE_80_110_MHZ,
	HS_CLK_RANGE_110_150_MHZ,
	HS_CLK_RANGE_150_200_MHZ,
	HS_CLK_RANGE_200_250_MHZ,
	HS_CLK_RANGE_250_300_MHZ,
	HS_CLK_RANGE_300_400_MHZ,
	HS_CLK_RANGE_400_500_MHZ,
	HS_CLK_RANGE_500_600_MHZ,
	HS_CLK_RANGE_600_700_MHZ,
	HS_CLK_RANGE_700_800_MHZ,
	HS_CLK_RANGE_800_1000_MHZ,
};

static const u8 t_hs_prepare_val[] = {
	[HS_CLK_RANGE_80_110_MHZ] = 0x20,
	[HS_CLK_RANGE_110_150_MHZ] = 0x06,
	[HS_CLK_RANGE_150_200_MHZ] = 0x18,
	[HS_CLK_RANGE_200_250_MHZ] = 0x05,
	[HS_CLK_RANGE_250_300_MHZ] = 0x51,
	[HS_CLK_RANGE_300_400_MHZ] = 0x64,
	[HS_CLK_RANGE_400_500_MHZ] = 0x20,
	[HS_CLK_RANGE_500_600_MHZ] = 0x6a,
	[HS_CLK_RANGE_600_700_MHZ] = 0x3e,
	[HS_CLK_RANGE_700_800_MHZ] = 0x21,
	[HS_CLK_RANGE_800_1000_MHZ] = 0x09,
};

static const u8 clock_lane_t_hs_zero_val[] = {
	[HS_CLK_RANGE_80_110_MHZ] = 0x16,
	[HS_CLK_RANGE_110_150_MHZ] = 0x16,
	[HS_CLK_RANGE_150_200_MHZ] = 0x17,
	[HS_CLK_RANGE_200_250_MHZ] = 0x17,
	[HS_CLK_RANGE_250_300_MHZ] = 0x18,
	[HS_CLK_RANGE_300_400_MHZ] = 0x19,
	[HS_CLK_RANGE_400_500_MHZ] = 0x1b,
	[HS_CLK_RANGE_500_600_MHZ] = 0x1d,
	[HS_CLK_RANGE_600_700_MHZ] = 0x1e,
	[HS_CLK_RANGE_700_800_MHZ] = 0x1f,
	[HS_CLK_RANGE_800_1000_MHZ] = 0x20,
};

static const u8 data_lane_t_hs_zero_val[] = {
	[HS_CLK_RANGE_80_110_MHZ] = 2,
	[HS_CLK_RANGE_110_150_MHZ] = 3,
	[HS_CLK_RANGE_150_200_MHZ] = 4,
	[HS_CLK_RANGE_200_250_MHZ] = 5,
	[HS_CLK_RANGE_250_300_MHZ] = 6,
	[HS_CLK_RANGE_300_400_MHZ] = 7,
	[HS_CLK_RANGE_400_500_MHZ] = 7,
	[HS_CLK_RANGE_500_600_MHZ] = 8,
	[HS_CLK_RANGE_600_700_MHZ] = 8,
	[HS_CLK_RANGE_700_800_MHZ] = 9,
	[HS_CLK_RANGE_800_1000_MHZ] = 9,
};

static const u8 t_hs_trail_val[] = {
	[HS_CLK_RANGE_80_110_MHZ] = 0x22,
	[HS_CLK_RANGE_110_150_MHZ] = 0x45,
	[HS_CLK_RANGE_150_200_MHZ] = 0x0b,
	[HS_CLK_RANGE_200_250_MHZ] = 0x16,
	[HS_CLK_RANGE_250_300_MHZ] = 0x2c,
	[HS_CLK_RANGE_300_400_MHZ] = 0x33,
	[HS_CLK_RANGE_400_500_MHZ] = 0x4e,
	[HS_CLK_RANGE_500_600_MHZ] = 0x3a,
	[HS_CLK_RANGE_600_700_MHZ] = 0x6a,
	[HS_CLK_RANGE_700_800_MHZ] = 0x29,
	[HS_CLK_RANGE_800_1000_MHZ] = 0x27,
};

struct mipi_dphy_timing {
	unsigned int clkmiss;
	unsigned int clkpost;
	unsigned int clkpre;
	unsigned int clkprepare;
	unsigned int clksettle;
	unsigned int clktermen;
	unsigned int clktrail;
	unsigned int clkzero;
	unsigned int dtermen;
	unsigned int eot;
	unsigned int hsexit;
	unsigned int hsprepare;
	unsigned int hszero;
	unsigned int hssettle;
	unsigned int hsskip;
	unsigned int hstrail;
	unsigned int init;
	unsigned int lpx;
	unsigned int taget;
	unsigned int tago;
	unsigned int tasure;
	unsigned int wakeup;
};

struct inno_mipi_dphy_timing {
	u8 t_lpx;
	u8 t_hs_prepare;
	u8 t_hs_zero;
	u8 t_hs_trail;
	u8 t_hs_exit;
	u8 t_clk_post;
	u8 t_wakup_h;
	u8 t_wakup_l;
	u8 t_clk_pre;
	u8 t_ta_go;
	u8 t_ta_sure;
	u8 t_ta_wait;
};

struct inno_mipi_dphy {
	const void *blob;
	int node;
	u32 regs;

	struct drm_display_mode *mode;
	unsigned int lane_mbps;
	int lanes;
	int bpp;
};

static inline void inno_write(struct inno_mipi_dphy *inno, u32 reg, u32 val)
{
	writel(val, inno->regs + reg);
}

static inline u32 inno_read(struct inno_mipi_dphy *inno, u32 reg)
{
	return readl(inno->regs + reg);
}

static inline void inno_update_bits(struct inno_mipi_dphy *inno, u32 reg,
				    u32 mask, u32 val)
{
	u32 tmp, orig;

	orig = inno_read(inno, reg);
	tmp = orig & ~mask;
	tmp |= val & mask;
	inno_write(inno, reg, tmp);
}

static void mipi_dphy_timing_get_default(struct mipi_dphy_timing *timing,
					 unsigned long period)
{
	/* Global Operation Timing Parameters */
	timing->clkmiss = 0;
	timing->clkpost = 70 + 52 * period;
	timing->clkpre = 8;
	timing->clkprepare = 65;
	timing->clksettle = 95;
	timing->clktermen = 0;
	timing->clktrail = 80;
	timing->clkzero = 260;
	timing->dtermen = 0;
	timing->eot = 0;
	timing->hsexit = 120;
	timing->hsprepare = 65 + 5 * period;
	timing->hszero = 145 + 5 * period;
	timing->hssettle = 85 + 6 * period;
	timing->hsskip = 40;
	timing->hstrail = max(4 * 8 * period, 60 + 4 * 4 * period);
	timing->init = 100000;
	timing->lpx = 60;
	timing->taget = 5 * timing->lpx;
	timing->tago = 4 * timing->lpx;
	timing->tasure = timing->lpx;
	timing->wakeup = 1000000;
}

static void inno_mipi_dphy_timing_update(struct inno_mipi_dphy *inno,
					 enum lane_type lane_type,
					 struct inno_mipi_dphy_timing *t)
{
	u32 base = lane_reg_offset[lane_type];

	inno_write(inno, base + T_HS_PREPARE_OFFSET,
		   T_HS_PREPARE(t->t_hs_prepare));
	inno_write(inno, base + T_HS_ZERO_OFFSET, T_HS_ZERO(t->t_hs_zero));
	inno_write(inno, base + T_HS_TRAIL_OFFSET, T_HS_TRAIL(t->t_hs_trail));
	inno_write(inno, base + T_HS_EXIT_OFFSET, T_HS_EXIT(t->t_hs_exit));
	inno_write(inno, base + T_CLK_POST_OFFSET, T_CLK_POST(t->t_clk_post));
	inno_write(inno, base + T_CLK_PRE_OFFSET, T_CLK_PRE(t->t_clk_pre));
	inno_write(inno, base + T_WAKUP_H_OFFSET, T_WAKUP_H(t->t_wakup_h));
	inno_write(inno, base + T_WAKUP_L_OFFSET, T_WAKUP_L(t->t_wakup_l));
	inno_write(inno, base + T_LPX_OFFSET, T_LPX(t->t_lpx));
	inno_write(inno, base + T_TA_GO_OFFSET, T_TA_GO(t->t_ta_go));
	inno_write(inno, base + T_TA_SURE_OFFSET, T_TA_SURE(t->t_ta_sure));
	inno_write(inno, base + T_TA_WAIT_OFFSET, T_TA_WAIT(t->t_ta_wait));
}

static enum hs_clk_range inno_mipi_dphy_get_hs_clk_range(u32 lane_mbps)
{
	if (lane_mbps < 110)
		return HS_CLK_RANGE_80_110_MHZ;
	else if (lane_mbps < 150)
		return HS_CLK_RANGE_110_150_MHZ;
	else if (lane_mbps < 200)
		return HS_CLK_RANGE_150_200_MHZ;
	else if (lane_mbps < 250)
		return HS_CLK_RANGE_200_250_MHZ;
	else if (lane_mbps < 300)
		return HS_CLK_RANGE_250_300_MHZ;
	else if (lane_mbps < 400)
		return HS_CLK_RANGE_400_500_MHZ;
	else if (lane_mbps < 500)
		return HS_CLK_RANGE_400_500_MHZ;
	else if (lane_mbps < 600)
		return HS_CLK_RANGE_500_600_MHZ;
	else if (lane_mbps < 700)
		return HS_CLK_RANGE_600_700_MHZ;
	else if (lane_mbps < 800)
		return HS_CLK_RANGE_700_800_MHZ;
	else
		return HS_CLK_RANGE_800_1000_MHZ;
}

static void inno_mipi_dphy_lane_timing_init(struct inno_mipi_dphy *inno,
					    enum lane_type lane_type)
{
	struct mipi_dphy_timing timing;
	struct inno_mipi_dphy_timing data;
	u32 txbyteclkhs = inno->lane_mbps / 8;	/* MHz */
	u32 txclkesc = 20;	/* MHz */
	u32 UI = DIV_ROUND_UP(NSEC_PER_USEC, inno->lane_mbps);	/* ns */
	enum hs_clk_range range;

	memset(&timing, 0, sizeof(timing));
	memset(&data, 0, sizeof(data));

	mipi_dphy_timing_get_default(&timing, UI);

	range = inno_mipi_dphy_get_hs_clk_range(inno->lane_mbps);

	if (lane_type == CLOCK_LANE)
		data.t_hs_zero = clock_lane_t_hs_zero_val[range];
	else
		data.t_hs_zero = data_lane_t_hs_zero_val[range];

	data.t_hs_prepare = t_hs_prepare_val[range];
	data.t_hs_trail = t_hs_trail_val[range];

	/* txbyteclkhs domain */
	data.t_hs_exit = DIV_ROUND_UP(txbyteclkhs * timing.hsexit, NSEC_PER_USEC);
	data.t_clk_post = DIV_ROUND_UP(txbyteclkhs * timing.clkpost, NSEC_PER_USEC);
	data.t_clk_pre = DIV_ROUND_UP(txbyteclkhs * timing.clkpre, NSEC_PER_USEC);
	data.t_wakup_h = 0x3;
	data.t_wakup_l = 0xff;
	data.t_lpx = DIV_ROUND_UP(txbyteclkhs * timing.lpx, NSEC_PER_USEC) - 2;

	/* txclkesc domain */
	data.t_ta_go = DIV_ROUND_UP(txclkesc * timing.tago, NSEC_PER_USEC);
	data.t_ta_sure = DIV_ROUND_UP(txclkesc * timing.tasure, NSEC_PER_USEC);
	data.t_ta_wait = DIV_ROUND_UP(txclkesc * timing.taget, NSEC_PER_USEC);

	inno_mipi_dphy_timing_update(inno, lane_type, &data);
}

static void inno_mipi_dphy_pll_init(struct inno_mipi_dphy *inno)
{
	unsigned int i, pre;
	unsigned int mpclk, pllref, tmp;
	unsigned int target_mbps = 1000;
	unsigned int max_mbps = 1000;
	u32 prediv = 1, fbdiv = 1;
	u32 val;

	mpclk = DIV_ROUND_UP(inno->mode->clock, MSEC_PER_SEC);
	if (mpclk) {
		/* take 1 / 0.9, since mbps must big than bandwidth of RGB */
		tmp = mpclk * (inno->bpp / inno->lanes) * 10 / 9;
		if (tmp < max_mbps)
			target_mbps = tmp;
		else
			printf("DPHY clock frequency is out of range\n");
	}

	debug("mpclk=%d, target_mbps=%d\n", mpclk, target_mbps);

	pllref = DIV_ROUND_UP(12 * MHZ, USEC_PER_SEC);
	tmp = pllref;

	/* PLL_Output_Frequency = FREF / PREDIV * FBDIV */
	for (i = 1; i < 6; i++) {
		pre = pllref / i;

		if ((tmp > (target_mbps % pre)) && (target_mbps / pre < 512)) {
			tmp = target_mbps % pre;
			prediv = i;
			fbdiv = target_mbps / pre;
		}

		if (tmp == 0)
			break;
	}

	inno->lane_mbps = pllref / prediv * fbdiv;

	val = FBDIV_8(fbdiv >> 8) | PREDIV(prediv);
	inno_write(inno, INNO_PHY_PLL_CTRL_0, val);

	val = FBDIV_7_0(fbdiv);
	inno_write(inno, INNO_PHY_PLL_CTRL_1, val);

	debug("MIPI-PHY: fin=%d, fout=%d, prediv=%d, fbdiv=%d\n",
	      pllref, inno->lane_mbps, prediv, fbdiv);
}

static inline void inno_mipi_dphy_reset(struct inno_mipi_dphy *inno)
{
	/* Reset analog */
	inno_write(inno, INNO_PHY_POWER_CTRL, 0xe0);
	udelay(20);
	/* Reset digital */
	inno_write(inno, INNO_PHY_DIG_CTRL, 0x1e);
	udelay(20);
	inno_write(inno, INNO_PHY_DIG_CTRL, 0x1f);
	udelay(20);
}

static void inno_mipi_dphy_timing_init(struct inno_mipi_dphy *inno)
{
	switch (inno->lanes) {
	case 4:
		inno_mipi_dphy_lane_timing_init(inno, DATA_LANE_3);
		/* Fall through */
	case 3:
		inno_mipi_dphy_lane_timing_init(inno, DATA_LANE_2);
		/* Fall through */
	case 2:
		inno_mipi_dphy_lane_timing_init(inno, DATA_LANE_1);
		/* Fall through */
	case 1:
	default:
		inno_mipi_dphy_lane_timing_init(inno, DATA_LANE_0);
		inno_mipi_dphy_lane_timing_init(inno, CLOCK_LANE);
		break;
	}
}

static inline void inno_mipi_dphy_lane_enable(struct inno_mipi_dphy *inno)
{
	u32 val = 0;
	u32 mask = 0;

	switch (inno->lanes) {
	case 4:
		mask |= DATA_LANE_3_EN_MASK;
		val |= DATA_LANE_3_EN;
		/* Fall through */
	case 3:
		mask |= DATA_LANE_2_EN_MASK;
		val |= DATA_LANE_2_EN;
		/* Fall through */
	case 2:
		mask |= DATA_LANE_1_EN_MASK;
		val |= DATA_LANE_1_EN;
		/* Fall through */
	default:
	case 1:
		mask |= DATA_LANE_0_EN_MASK | CLK_LANE_EN_MASK;
		val |= DATA_LANE_0_EN | CLK_LANE_EN;
		break;
	}

	inno_update_bits(inno, INNO_PHY_LANE_CTRL, mask, val);
}

static inline void inno_mipi_dphy_pll_ldo_enable(struct inno_mipi_dphy *inno)
{
	inno_write(inno, INNO_PHY_POWER_CTRL, 0xe4);
	udelay(20);
}

static int inno_mipi_dphy_power_on(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_mipi_dphy *inno = conn_state->phy_private;

	inno->mode = &conn_state->mode;

	inno_mipi_dphy_pll_init(inno);
	inno_mipi_dphy_pll_ldo_enable(inno);
	inno_mipi_dphy_lane_enable(inno);
	inno_mipi_dphy_reset(inno);
	inno_mipi_dphy_timing_init(inno);

	return 0;
}

static inline void inno_mipi_dphy_lane_disable(struct inno_mipi_dphy *inno)
{
	inno_update_bits(inno, INNO_PHY_LANE_CTRL, 0x7c, 0x00);
}

static inline void inno_mipi_dphy_pll_ldo_disable(struct inno_mipi_dphy *inno)
{
	inno_write(inno, INNO_PHY_POWER_CTRL, 0xe3);
	udelay(20);
}

static int inno_mipi_dphy_power_off(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_mipi_dphy *inno = conn_state->phy_private;

	inno_mipi_dphy_lane_disable(inno);
	inno_mipi_dphy_pll_ldo_disable(inno);

	return 0;
}

static int inno_mipi_dphy_get_data(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_mipi_dphy *inno = conn_state->phy_private;

	return inno->lane_mbps;
}

static int inno_mipi_dphy_parse_dt(int node, struct inno_mipi_dphy *inno)
{
	const void *blob = inno->blob;
	int phandle, panel_node;
	int format;

	phandle = fdt_getprop_u32_default_node(blob, node, 0,
					       "rockchip,dsi-panel", -1);
	if (phandle < 0) {
		printf("failed to find 'rockchip,dsi-panel' property\n");
		goto set_default;
	}

	panel_node = fdt_node_offset_by_phandle(blob, phandle);
	if (panel_node < 0) {
		printf("failed to find panel node\n");
		goto set_default;
	}

	inno->lanes = fdtdec_get_int(blob, panel_node, "dsi,lanes", -1);
	if (inno->lanes < 0)
		inno->lanes = 4;

	format = fdtdec_get_int(blob, panel_node, "dsi,format", -1);
	inno->bpp = mipi_dsi_pixel_format_to_bpp(format);
	if (inno->bpp < 0)
		inno->bpp = 24;

	return 0;

set_default:
	inno->bpp = 24;
	inno->lanes = 4;
	return 0;
}

static int inno_mipi_dphy_init(struct display_state *state)
{
	const void *blob = state->blob;
	struct connector_state *conn_state = &state->conn_state;
	int phy_node = conn_state->phy_node;
	struct inno_mipi_dphy *inno;
	int ret;

	inno = malloc(sizeof(*inno));
	if (!inno)
		return -ENOMEM;

	inno->blob = blob;
	inno->node = phy_node;

	ret = inno_mipi_dphy_parse_dt(phy_node, inno);
	if (ret) {
		printf("%s: failed to parse DT\n", __func__);
		return ret;
	}

	inno->regs = fdtdec_get_addr(blob, phy_node, "reg");
	if (inno->regs == FDT_ADDR_T_NONE) {
		printf("%s: failed to get mipi phy address\n", __func__);
		return -ENOMEM;
	}

	conn_state->phy_private = inno;

	return 0;
}

const struct rockchip_phy_funcs rockchip_inno_mipi_dphy_funcs = {
	.init = inno_mipi_dphy_init,
	.power_on = inno_mipi_dphy_power_on,
	.power_off = inno_mipi_dphy_power_off,
	.get_data = inno_mipi_dphy_get_data,
};
