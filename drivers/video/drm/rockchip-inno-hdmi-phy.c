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
#include <div64.h>
#include <linux/media-bus-format.h>

#include "../rockchip_display.h"
#include "../rockchip_crtc.h"
#include "../rockchip_connector.h"
#include "../rockchip_phy.h"

#define INNO_HDMI_PHY_TIMEOUT_LOOP_COUNT	1000
/*#define UPDATE(x, h, l)	(((x) << (l)) & GENMASK((h), (l)))*/

/* REG: 0x00 */
#define PRE_PLL_REFCLK_SEL_MASK			BIT(0)
#define PRE_PLL_REFCLK_SEL_PCLK			BIT(0)
#define PRE_PLL_REFCLK_SEL_OSCCLK		0
/* REG: 0x01 */
#define BYPASS_RXSENSE_EN_MASK			BIT(2)
#define BYPASS_RXSENSE_EN			BIT(2)
#define BYPASS_PWRON_EN_MASK			BIT(1)
#define BYPASS_PWRON_EN				BIT(1)
#define BYPASS_PLLPD_EN_MASK			BIT(0)
#define BYPASS_PLLPD_EN				BIT(0)
/* REG: 0x02 */
#define BYPASS_PDATA_EN_MASK			BIT(4)
#define BYPASS_PDATA_EN				BIT(4)
#define PDATAEN_MASK				BIT(0)
#define PDATAEN_DISABLE				BIT(0)
#define PDATAEN_ENABLE				0
/* REG: 0x03 */
#define BYPASS_AUTO_TERM_RES_CAL		BIT(7)
#define AUDO_TERM_RES_CAL_SPEED_14_8(x)		UPDATE(x, 6, 0)
/* REG: 0x04 */
#define AUDO_TERM_RES_CAL_SPEED_7_0(x)		UPDATE(x, 7, 0)
/* REG: 0xaa */
#define POST_PLL_CTRL_MASK			BIT(0)
#define POST_PLL_CTRL_MANUAL			BIT(0)
/* REG: 0xe0 */
#define POST_PLL_POWER_MASK			BIT(5)
#define POST_PLL_POWER_DOWN			BIT(5)
#define POST_PLL_POWER_UP			0
#define PRE_PLL_POWER_MASK			BIT(4)
#define PRE_PLL_POWER_DOWN			BIT(4)
#define PRE_PLL_POWER_UP			0
#define RXSENSE_CLK_CH_MASK			BIT(3)
#define RXSENSE_CLK_CH_ENABLE			BIT(3)
#define RXSENSE_DATA_CH2_MASK			BIT(2)
#define RXSENSE_DATA_CH2_ENABLE			BIT(2)
#define RXSENSE_DATA_CH1_MASK			BIT(1)
#define RXSENSE_DATA_CH1_ENABLE			BIT(1)
#define RXSENSE_DATA_CH0_MASK			BIT(0)
#define RXSENSE_DATA_CH0_ENABLE			BIT(0)
/* REG: 0xe1 */
#define BANDGAP_MASK				BIT(4)
#define BANDGAP_ENABLE				BIT(4)
#define BANDGAP_DISABLE				0
#define TMDS_DRIVER_MASK			GENMASK(3, 0)
#define TMDS_DRIVER_ENABLE			UPDATE(0xf, 3, 0)
#define TMDS_DRIVER_DISABLE			0
/* REG: 0xe2 */
#define PRE_PLL_FB_DIV_8_MASK			BIT(7)
#define PRE_PLL_FB_DIV_8_SHIFT			7
#define PRE_PLL_FB_DIV_8(x)			UPDATE(x, 7, 7)
#define PCLK_VCO_DIV_5_MASK			BIT(5)
#define PCLK_VCO_DIV_5_SHIFT			5
#define PCLK_VCO_DIV_5(x)			UPDATE(x, 5, 5)
#define PRE_PLL_PRE_DIV_MASK			GENMASK(4, 0)
#define PRE_PLL_PRE_DIV(x)			UPDATE(x, 4, 0)
/* REG: 0xe3 */
#define PRE_PLL_FB_DIV_7_0(x)			UPDATE(x, 7, 0)
/* REG: 0xe4 */
#define PRE_PLL_PCLK_DIV_B_MASK			GENMASK(6, 5)
#define PRE_PLL_PCLK_DIV_B_SHIFT		5
#define PRE_PLL_PCLK_DIV_B(x)			UPDATE(x, 6, 5)
#define PRE_PLL_PCLK_DIV_A_MASK			GENMASK(4, 0)
#define PRE_PLL_PCLK_DIV_A_SHIFT		0
#define PRE_PLL_PCLK_DIV_A(x)			UPDATE(x, 4, 0)
/* REG: 0xe5 */
#define PRE_PLL_PCLK_DIV_C_MASK			GENMASK(6, 5)
#define PRE_PLL_PCLK_DIV_C_SHIFT		5
#define PRE_PLL_PCLK_DIV_C(x)			UPDATE(x, 6, 5)
#define PRE_PLL_PCLK_DIV_D_MASK			GENMASK(4, 0)
#define PRE_PLL_PCLK_DIV_D_SHIFT		0
#define PRE_PLL_PCLK_DIV_D(x)			UPDATE(x, 4, 0)
/* REG: 0xe6 */
#define PRE_PLL_TMDSCLK_DIV_C_MASK		GENMASK(5, 4)
#define PRE_PLL_TMDSCLK_DIV_C(x)		UPDATE(x, 5, 4)
#define PRE_PLL_TMDSCLK_DIV_A_MASK		GENMASK(3, 2)
#define PRE_PLL_TMDSCLK_DIV_A(x)		UPDATE(x, 3, 2)
#define PRE_PLL_TMDSCLK_DIV_B_MASK		GENMASK(1, 0)
#define PRE_PLL_TMDSCLK_DIV_B(x)		UPDATE(x, 1, 0)
/* REG: 0xe8 */
#define PRE_PLL_LOCK_STATUS			BIT(0)
/* REG: 0xe9 */
#define POST_PLL_POST_DIV_EN_MASK		GENMASK(7, 6)
#define POST_PLL_POST_DIV_ENABLE		UPDATE(3, 7, 6)
#define POST_PLL_POST_DIV_DISABLE		0
#define POST_PLL_PRE_DIV_MASK			GENMASK(4, 0)
#define POST_PLL_PRE_DIV(x)			UPDATE(x, 4, 0)
/* REG: 0xea */
#define POST_PLL_FB_DIV_7_0(x)			UPDATE(x, 7, 0)
/* REG: 0xeb */
#define POST_PLL_FB_DIV_8_MASK			BIT(7)
#define POST_PLL_FB_DIV_8(x)			UPDATE(x, 7, 7)
#define POST_PLL_POST_DIV_MASK			GENMASK(5, 4)
#define POST_PLL_POST_DIV(x)			UPDATE(x, 5, 4)
#define POST_PLL_LOCK_STATUS			BIT(0)
/* REG: 0xee */
#define TMDS_CH_TA_MASK				GENMASK(7, 4)
#define TMDS_CH_TA_ENABLE			UPDATE(0xf, 7, 4)
#define TMDS_CH_TA_DISABLE			0
/* REG: 0xef */
#define TMDS_CLK_CH_TA(x)			UPDATE(x, 7, 6)
#define TMDS_DATA_CH2_TA(x)			UPDATE(x, 5, 4)
#define TMDS_DATA_CH1_TA(x)			UPDATE(x, 3, 2)
#define TMDS_DATA_CH0_TA(x)			UPDATE(x, 1, 0)
/* REG: 0xf0 */
#define TMDS_DATA_CH2_PRE_EMPHASIS_MASK		GENMASK(5, 4)
#define TMDS_DATA_CH2_PRE_EMPHASIS(x)		UPDATE(x, 5, 4)
#define TMDS_DATA_CH1_PRE_EMPHASIS_MASK		GENMASK(3, 2)
#define TMDS_DATA_CH1_PRE_EMPHASIS(x)		UPDATE(x, 3, 2)
#define TMDS_DATA_CH0_PRE_EMPHASIS_MASK		GENMASK(1, 0)
#define TMDS_DATA_CH0_PRE_EMPHASIS(x)		UPDATE(x, 1, 0)
/* REG: 0xf1 */
#define TMDS_CLK_CH_OUTPUT_SWING(x)		UPDATE(x, 7, 4)
#define TMDS_DATA_CH2_OUTPUT_SWING(x)		UPDATE(x, 3, 0)
/* REG: 0xf2 */
#define TMDS_DATA_CH1_OUTPUT_SWING(x)		UPDATE(x, 7, 4)
#define TMDS_DATA_CH0_OUTPUT_SWING(x)		UPDATE(x, 3, 0)

enum inno_hdmi_phy_type {
	INNO_HDMI_PHY_RK3228,
	INNO_HDMI_PHY_RK3328
};

struct inno_hdmi_phy_drv_data;

struct inno_hdmi_phy {
	const void *blob;
	int node;
	u32 regs;

	/* platform data */
	const struct inno_hdmi_phy_drv_data *plat_data;
	unsigned long pixclock;
};

struct pre_pll_config {
	unsigned long pixclock;
	unsigned long tmdsclock;
	u8 prediv;
	u16 fbdiv;
	u8 tmds_div_a;
	u8 tmds_div_b;
	u8 tmds_div_c;
	u8 pclk_div_a;
	u8 pclk_div_b;
	u8 pclk_div_c;
	u8 pclk_div_d;
	u8 vco_div_5_en;
	u32 fracdiv;
};

struct post_pll_config {
	unsigned long tmdsclock;
	u8 prediv;
	u16 fbdiv;
	u8 postdiv;
	u8 version;
};

struct phy_config {
	unsigned long	tmdsclock;
	u8		regs[14];
};

struct inno_hdmi_phy_ops {
	void (*init)(struct inno_hdmi_phy *inno);
	int (*power_on)(struct inno_hdmi_phy *inno,
			const struct post_pll_config *cfg,
			const struct phy_config *phy_cfg);
	void (*power_off)(struct inno_hdmi_phy *inno);
	int (*pre_pll_update)(struct inno_hdmi_phy *inno,
			      const struct pre_pll_config *cfg);
	unsigned long (*recalc_rate)(struct inno_hdmi_phy *inno,
				     unsigned long parent_rate);
};

struct inno_hdmi_phy_drv_data {
	enum inno_hdmi_phy_type		dev_type;
	const struct inno_hdmi_phy_ops	*ops;
	const struct phy_config		*phy_cfg_table;
};

static const struct pre_pll_config pre_pll_cfg_table[] = {
	{ 27000000,  27000000, 1,  90, 3, 2, 2, 10, 3, 3, 4, 0, 0},
	{ 27000000,  33750000, 1,  90, 1, 3, 3, 10, 3, 3, 4, 0, 0},
	{ 40000000,  40000000, 1,  80, 2, 2, 2, 12, 2, 2, 2, 0, 0},
	{ 59341000,  59341000, 1,  98, 3, 1, 2,  1, 3, 3, 4, 0, 0xE6AE6B},
	{ 59400000,  59400000, 1,  99, 3, 1, 1,  1, 3, 3, 4, 0, 0},
	{ 59341000,  74176250, 1,  98, 0, 3, 3,  1, 3, 3, 4, 0, 0xE6AE6B},
	{ 59400000,  74250000, 1,  99, 1, 2, 2,  1, 3, 3, 4, 0, 0},
	{ 74176000,  74176000, 1,  98, 1, 2, 2,  1, 2, 3, 4, 0, 0xE6AE6B},
	{ 74250000,  74250000, 1,  99, 1, 2, 2,  1, 2, 3, 4, 0, 0},
	{ 74176000,  92720000, 4, 494, 1, 2, 2,  1, 3, 3, 4, 0, 0x816817},
	{ 74250000,  92812500, 4, 495, 1, 2, 2,  1, 3, 3, 4, 0, 0},
	{148352000, 148352000, 1,  98, 1, 1, 1,  1, 2, 2, 2, 0, 0xE6AE6B},
	{148500000, 148500000, 1,  99, 1, 1, 1,  1, 2, 2, 2, 0, 0},
	{148352000, 185440000, 4, 494, 0, 2, 2,  1, 3, 2, 2, 0, 0x816817},
	{148500000, 185625000, 4, 495, 0, 2, 2,  1, 3, 2, 2, 0, 0},
	{296703000, 296703000, 1,  98, 0, 1, 1,  1, 0, 2, 2, 0, 0xE6AE6B},
	{297000000, 297000000, 1,  99, 0, 1, 1,  1, 0, 2, 2, 0, 0},
	{296703000, 370878750, 4, 494, 1, 2, 0,  1, 3, 1, 1, 0, 0x816817},
	{297000000, 371250000, 4, 495, 1, 2, 0,  1, 3, 1, 1, 0, 0},
	{593407000, 296703500, 1,  98, 0, 1, 1,  1, 0, 2, 1, 0, 0xE6AE6B},
	{594000000, 297000000, 1,  99, 0, 1, 1,  1, 0, 2, 1, 0, 0},
	{593407000, 370879375, 4, 494, 1, 2, 0,  1, 3, 1, 1, 1, 0x816817},
	{594000000, 371250000, 4, 495, 1, 2, 0,  1, 3, 1, 1, 1, 0},
	{593407000, 593407000, 1,  98, 0, 2, 0,  1, 0, 1, 1, 0, 0xE6AE6B},
	{594000000, 594000000, 1,  99, 0, 2, 0,  1, 0, 1, 1, 0, 0},
	{     ~0UL,	    0, 0,   0, 0, 0, 0,  0, 0, 0, 0, 0, 0}
};

static const struct post_pll_config post_pll_cfg_table[] = {
	{33750000,  1, 40, 8, 1},
	{33750000,  1, 80, 8, 2},
	{74250000,  1, 40, 8, 1},
	{74250000, 18, 80, 8, 2},
	{148500000, 2, 40, 4, 3},
	{297000000, 4, 40, 2, 3},
	{594000000, 8, 40, 1, 3},
	{     ~0UL, 0,  0, 0, 0}
};

static const struct phy_config rk3228_phy_cfg[] = {
	{	165000000, {
			0xaa, 0x00, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00,
		},
	}, {
		340000000, {
			0xaa, 0x15, 0x6a, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00,
		},
	}, {
		594000000, {
			0xaa, 0x15, 0x7a, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00,
		},
	}, {
		~0UL, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00,
		},
	}
};

static const struct phy_config rk3328_phy_cfg[] = {
	{	165000000, {
			0x07, 0x08, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08, 0x08,
			0x00, 0xac, 0xcc, 0xcc, 0xcc,
		},
	}, {
		340000000, {
			0x0b, 0x0d, 0x0d, 0x0d, 0x07, 0x15, 0x08, 0x08, 0x08,
			0x3f, 0xac, 0xcc, 0xcd, 0xdd,
		},
	}, {
		594000000, {
			0x10, 0x1a, 0x1a, 0x1a, 0x07, 0x15, 0x08, 0x08, 0x08,
			0x00, 0xac, 0xcc, 0xcc, 0xcc,
		},
	}, {
		~0UL, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00,
		},
	}
};

static inline void inno_write(struct inno_hdmi_phy *inno, u32 reg, u8 val)
{
	writel(val, inno->regs + (reg * 4));
}

static inline u8 inno_read(struct inno_hdmi_phy *inno, u32 reg)
{
	u32 val;

	val = readl(inno->regs + (reg * 4));

	return val;
}

static inline void inno_update_bits(struct inno_hdmi_phy *inno, u8 reg,
				    u8 mask, u8 val)
{
	u32 tmp, orig;

	orig = inno_read(inno, reg);
	tmp = orig & ~mask;
	tmp |= val & mask;
	inno_write(inno, reg, tmp);
}

static u32 inno_hdmi_phy_get_tmdsclk(struct inno_hdmi_phy *inno, unsigned long rate,
				     int format)
{
	u32 tmdsclk;

	switch (format) {
	case MEDIA_BUS_FMT_UYYVYY8_0_5X24:
		tmdsclk = rate / 2;
		break;
	case MEDIA_BUS_FMT_UYYVYY10_0_5X30:
		tmdsclk = rate * 5 / 8;
		break;
	case MEDIA_BUS_FMT_RGB101010_1X30:
	case MEDIA_BUS_FMT_YUV10_1X30:
	case MEDIA_BUS_FMT_UYVY10_1X20:
		tmdsclk = rate * 5 / 4;
		break;
	default:
		tmdsclk = rate;
	}

	return tmdsclk;
}

static int inno_hdmi_phy_power_on(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_hdmi_phy *inno = conn_state->phy_private;
	const struct post_pll_config *cfg = post_pll_cfg_table;
	const struct phy_config *phy_cfg = inno->plat_data->phy_cfg_table;
	int format = conn_state->bus_format;
	u32 tmdsclock = inno_hdmi_phy_get_tmdsclk(inno, inno->pixclock, format);
	u32 chipversion = 1;

	debug("start Inno HDMI PHY Power On\n");
	if (!tmdsclock) {
		printf("TMDS clock is zero!\n");
		return -EINVAL;
	}

	if (inno->plat_data->dev_type == INNO_HDMI_PHY_RK3328 &&
	    rk_get_cpu_version())
		chipversion = 2;

	debug("tmdsclock = %d; chipversion = %d\n", tmdsclock, chipversion);

	for (; cfg->tmdsclock != ~0UL; cfg++)
		if (tmdsclock <= cfg->tmdsclock &&
		    cfg->version & chipversion)
			break;

	for (; phy_cfg->tmdsclock != ~0UL; phy_cfg++)
		if (tmdsclock <= phy_cfg->tmdsclock)
			break;

	if (cfg->tmdsclock == ~0UL || phy_cfg->tmdsclock == ~0UL)
		return -EINVAL;

	debug("Inno HDMI PHY Power On\n");
	if (inno->plat_data->ops->power_on)
		return inno->plat_data->ops->power_on(inno, cfg, phy_cfg);
	else
		return -EINVAL;
}

static int inno_hdmi_phy_power_off(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_hdmi_phy *inno = conn_state->phy_private;

	if (inno->plat_data->ops->power_off)
		inno->plat_data->ops->power_off(inno);
	debug("Inno HDMI PHY Power Off\n");

	return 0;
}

static int inno_hdmi_phy_clk_is_prepared(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_hdmi_phy *inno = conn_state->phy_private;
	u8 status;

	if (inno->plat_data->dev_type == INNO_HDMI_PHY_RK3228)
		status = inno_read(inno, 0xe0) & PRE_PLL_POWER_MASK;
	else
		status = inno_read(inno, 0xa0) & 1;

	return status ? 0 : 1;
}

static int inno_hdmi_phy_clk_prepare(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_hdmi_phy *inno = conn_state->phy_private;

	if (inno->plat_data->dev_type == INNO_HDMI_PHY_RK3228)
		inno_update_bits(inno, 0xe0, PRE_PLL_POWER_MASK,
				 PRE_PLL_POWER_UP);
	else
		inno_update_bits(inno, 0xa0, 1, 0);

	return 0;
}

static int inno_hdmi_phy_clk_set_rate(struct display_state *state,
				      unsigned long rate)
{
	struct connector_state *conn_state = &state->conn_state;
	struct inno_hdmi_phy *inno = conn_state->phy_private;
	const struct pre_pll_config *cfg = pre_pll_cfg_table;
	int format = conn_state->bus_format;
	u32 tmdsclock = inno_hdmi_phy_get_tmdsclk(inno, rate, format);

	printf("%s rate %lu tmdsclk %u\n", __func__, rate, tmdsclock);
	for (; cfg->pixclock != ~0UL; cfg++)
		if (cfg->pixclock == rate && cfg->tmdsclock == tmdsclock)
			break;

	if (cfg->pixclock == ~0UL) {
		printf("unsupported rate %lu\n", rate);
		return -EINVAL;
	}

	if (inno->plat_data->ops->pre_pll_update)
		inno->plat_data->ops->pre_pll_update(inno, cfg);

	inno->pixclock = rate;

	return 0;
}

static void inno_hdmi_phy_rk3228_init(struct inno_hdmi_phy *inno)
{
	u32 m, v;

	/*
	 * Use phy internal register control
	 * rxsense/poweron/pllpd/pdataen signal.
	 */
	m = BYPASS_RXSENSE_EN_MASK | BYPASS_PWRON_EN_MASK |
	    BYPASS_PLLPD_EN_MASK;
	v = BYPASS_RXSENSE_EN | BYPASS_PWRON_EN | BYPASS_PLLPD_EN;
	inno_update_bits(inno, 0x01, m, v);
	inno_update_bits(inno, 0x02, BYPASS_PDATA_EN_MASK, BYPASS_PDATA_EN);

	/* manual power down post-PLL */
	inno_update_bits(inno, 0xaa, POST_PLL_CTRL_MASK, POST_PLL_CTRL_MANUAL);
}

static int
inno_hdmi_phy_rk3228_power_on(struct inno_hdmi_phy *inno,
			      const struct post_pll_config *cfg,
			      const struct phy_config *phy_cfg)
{
	int pll_tries;
	u32 m, v;

	/* pdata_en disable */
	inno_update_bits(inno, 0x02, PDATAEN_MASK, PDATAEN_DISABLE);

	/* Power down Post-PLL */
	inno_update_bits(inno, 0xe0, POST_PLL_POWER_MASK, POST_PLL_POWER_DOWN);

	/* Post-PLL update */
	m = POST_PLL_PRE_DIV_MASK;
	v = POST_PLL_PRE_DIV(cfg->prediv);
	inno_update_bits(inno, 0xe9, m, v);

	m = POST_PLL_FB_DIV_8_MASK;
	v = POST_PLL_FB_DIV_8(cfg->fbdiv >> 8);
	inno_update_bits(inno, 0xeb, m, v);
	inno_write(inno, 0xea, POST_PLL_FB_DIV_7_0(cfg->fbdiv));

	if (cfg->postdiv == 1) {
		/* Disable Post-PLL post divider */
		m = POST_PLL_POST_DIV_EN_MASK;
		v = POST_PLL_POST_DIV_DISABLE;
		inno_update_bits(inno, 0xe9, m, v);
	} else {
		/* Enable Post-PLL post divider */
		m = POST_PLL_POST_DIV_EN_MASK;
		v = POST_PLL_POST_DIV_ENABLE;
		inno_update_bits(inno, 0xe9, m, v);

		m = POST_PLL_POST_DIV_MASK;
		v = POST_PLL_POST_DIV(cfg->postdiv / 2 - 1);
		inno_update_bits(inno, 0xeb, m, v);
	}

	for (v = 0; v < 4; v++)
		inno_write(inno, 0xef + v, phy_cfg->regs[v]);

	/* Power up Post-PLL */
	inno_update_bits(inno, 0xe0, POST_PLL_POWER_MASK, POST_PLL_POWER_UP);

	/* BandGap enable */
	inno_update_bits(inno, 0xe1, BANDGAP_MASK, BANDGAP_ENABLE);

	/* TMDS driver enable */
	inno_update_bits(inno, 0xe1, TMDS_DRIVER_MASK, TMDS_DRIVER_ENABLE);

	/* Wait for post PLL lock */
	pll_tries = 0;
	while (!(inno_read(inno, 0xeb) & POST_PLL_LOCK_STATUS)) {
		if (pll_tries == INNO_HDMI_PHY_TIMEOUT_LOOP_COUNT) {
			printf("Post-PLL unlock\n");
			return -ETIMEDOUT;
		}

		pll_tries++;
		udelay(100);
	}

	if (cfg->tmdsclock > 340000000)
		mdelay(100);

	/* pdata_en enable */
	inno_update_bits(inno, 0x02, PDATAEN_MASK, PDATAEN_ENABLE);
	return 0;
}

static void inno_hdmi_phy_rk3228_power_off(struct inno_hdmi_phy *inno)
{
	/* TMDS driver Disable */
	inno_update_bits(inno, 0xe1, TMDS_DRIVER_MASK, TMDS_DRIVER_DISABLE);

	/* BandGap Disable */
	inno_update_bits(inno, 0xe1, BANDGAP_MASK, BANDGAP_DISABLE);

	/* Post-PLL power down */
	inno_update_bits(inno, 0xe0, POST_PLL_POWER_MASK, POST_PLL_POWER_DOWN);
}

static int
inno_hdmi_phy_rk3228_pre_pll_update(struct inno_hdmi_phy *inno,
				    const struct pre_pll_config *cfg)
{
	int pll_tries;
	u32 m, v;

	/* Power down PRE-PLL */
	inno_update_bits(inno, 0xe0, PRE_PLL_POWER_MASK, PRE_PLL_POWER_DOWN);

	m = PRE_PLL_FB_DIV_8_MASK | PCLK_VCO_DIV_5_MASK | PRE_PLL_PRE_DIV_MASK;
	v = PRE_PLL_FB_DIV_8(cfg->fbdiv >> 8) |
	    PCLK_VCO_DIV_5(cfg->vco_div_5_en) | PRE_PLL_PRE_DIV(cfg->prediv);
	inno_update_bits(inno, 0xe2, m, v);

	inno_write(inno, 0xe3, PRE_PLL_FB_DIV_7_0(cfg->fbdiv));

	m = PRE_PLL_PCLK_DIV_B_MASK | PRE_PLL_PCLK_DIV_A_MASK;
	v = PRE_PLL_PCLK_DIV_B(cfg->pclk_div_b) |
	    PRE_PLL_PCLK_DIV_A(cfg->pclk_div_a);
	inno_update_bits(inno, 0xe4, m, v);

	m = PRE_PLL_PCLK_DIV_C_MASK | PRE_PLL_PCLK_DIV_D_MASK;
	v = PRE_PLL_PCLK_DIV_C(cfg->pclk_div_c) |
	    PRE_PLL_PCLK_DIV_D(cfg->pclk_div_d);
	inno_update_bits(inno, 0xe5, m, v);

	m = PRE_PLL_TMDSCLK_DIV_C_MASK | PRE_PLL_TMDSCLK_DIV_A_MASK |
	    PRE_PLL_TMDSCLK_DIV_B_MASK;
	v = PRE_PLL_TMDSCLK_DIV_C(cfg->tmds_div_c) |
	    PRE_PLL_TMDSCLK_DIV_A(cfg->tmds_div_a) |
	    PRE_PLL_TMDSCLK_DIV_B(cfg->tmds_div_b);
	inno_update_bits(inno, 0xe6, m, v);

	/* Power up PRE-PLL */
	inno_update_bits(inno, 0xe0, PRE_PLL_POWER_MASK, PRE_PLL_POWER_UP);

	/* Wait for Pre-PLL lock */
	pll_tries = 0;
	while (!(inno_read(inno, 0xe8) & PRE_PLL_LOCK_STATUS)) {
		if (pll_tries == INNO_HDMI_PHY_TIMEOUT_LOOP_COUNT) {
			printf("Pre-PLL unlock\n");
			return -ETIMEDOUT;
		}

		pll_tries++;
		udelay(100);
	}

	return 0;
}

static void inno_hdmi_phy_rk3328_init(struct inno_hdmi_phy *inno)
{
	/*
	 * Use phy internal register control
	 * rxsense/poweron/pllpd/pdataen signal.
	 */
	inno_write(inno, 0x01, 0x07);
	inno_write(inno, 0x02, 0x91);
}

static int
inno_hdmi_phy_rk3328_power_on(struct inno_hdmi_phy *inno,
			      const struct post_pll_config *cfg,
			      const struct phy_config *phy_cfg)
{
	u32 val;

	/* set pdata_en to 0 */
	inno_update_bits(inno, 0x02, 1, 0);
	/* Power off post PLL */
	inno_update_bits(inno, 0xaa, 1, 1);

	val = cfg->fbdiv & 0xff;
	inno_write(inno, 0xac, val);
	if (cfg->postdiv == 1) {
		inno_write(inno, 0xaa, 2);
		val = (cfg->fbdiv >> 8) | cfg->prediv;
		inno_write(inno, 0xab, val);
	} else {
		val = (cfg->postdiv / 2) - 1;
		inno_write(inno, 0xad, val);
		val = (cfg->fbdiv >> 8) | cfg->prediv;
		inno_write(inno, 0xab, val);
		inno_write(inno, 0xaa, 0x0e);
	}

	for (val = 0; val < 14; val++)
		inno_write(inno, 0xb5 + val, phy_cfg->regs[val]);

	/* bit[7:6] of reg c8/c9/ca/c8 is ESD detect threshold:
	 * 00 - 340mV
	 * 01 - 280mV
	 * 10 - 260mV
	 * 11 - 240mV
	 * default is 240mV, now we set it to 340mV
	 */
	inno_write(inno, 0xc8, 0);
	inno_write(inno, 0xc9, 0);
	inno_write(inno, 0xca, 0);
	inno_write(inno, 0xcb, 0);

	if (phy_cfg->tmdsclock > 340000000) {
		/* Set termination resistor to 100ohm */
		val = 75000000 / 100000;
		inno_write(inno, 0xc5, ((val >> 8) & 0xff) | 0x80);
		inno_write(inno, 0xc6, val & 0xff);
		inno_write(inno, 0xc7, 3 << 1);
		inno_write(inno, 0xc5, ((val >> 8) & 0xff));
	} else if (phy_cfg->tmdsclock > 165000000) {
		inno_write(inno, 0xc5, 0x81);
		/* clk termination resistor is 50ohm
		 * data termination resistor is 150ohm
		 */
		inno_write(inno, 0xc8, 0x30);
		inno_write(inno, 0xc9, 0x10);
		inno_write(inno, 0xca, 0x10);
		inno_write(inno, 0xcb, 0x10);
	} else {
		inno_write(inno, 0xc5, 0x81);
	}

	/* Power up post PLL */
	inno_update_bits(inno, 0xaa, 1, 0);
	/* Power up tmds driver */
	inno_update_bits(inno, 0xb0, 4, 4);
	inno_write(inno, 0xb2, 0x0f);

	/* Wait for post PLL lock */
	for (val = 0; val < 5; val++) {
		if (inno_read(inno, 0xaf) & 1)
			break;
		udelay(1000);
	}
	if (!(inno_read(inno, 0xaf) & 1)) {
		printf("HDMI PHY Post PLL unlock\n");
		return -ETIMEDOUT;
	}
	if (phy_cfg->tmdsclock > 340000000)
		mdelay(100);
	/* set pdata_en to 1 */
	inno_update_bits(inno, 0x02, 1, 1);

	return 0;
}

static void inno_hdmi_phy_rk3328_power_off(struct inno_hdmi_phy *inno)
{
	/* Power off driver */
	inno_write(inno, 0xb2, 0);
	/* Power off band gap */
	inno_update_bits(inno, 0xb0, 4, 0);
	/* Power off post pll */
	inno_update_bits(inno, 0xaa, 1, 1);
}

static int
inno_hdmi_phy_rk3328_pre_pll_update(struct inno_hdmi_phy *inno,
				    const struct pre_pll_config *cfg)
{
	u32 val;

	/* Power off PLL */
	inno_update_bits(inno, 0xa0, 1, 1);
	/* Configure pre-pll */
	inno_update_bits(inno, 0xa0, 2, (cfg->vco_div_5_en & 1) << 1);
	inno_write(inno, 0xa1, cfg->prediv);
	if (cfg->fracdiv)
		val = ((cfg->fbdiv >> 8) & 0x0f) | 0xc0;
	else
		val = ((cfg->fbdiv >> 8) & 0x0f) | 0xf0;
	inno_write(inno, 0xa2, val);
	inno_write(inno, 0xa3, cfg->fbdiv & 0xff);
	val = (cfg->pclk_div_a & 0x1f) |
	      ((cfg->pclk_div_b & 3) << 5);
	inno_write(inno, 0xa5, val);
	val = (cfg->pclk_div_d & 0x1f) |
	      ((cfg->pclk_div_c & 3) << 5);
	inno_write(inno, 0xa6, val);
	val = ((cfg->tmds_div_a & 3) << 4) |
	      ((cfg->tmds_div_b & 3) << 2) |
	      (cfg->tmds_div_c & 3);
	inno_write(inno, 0xa4, val);

	if (cfg->fracdiv) {
		val = cfg->fracdiv & 0xff;
		inno_write(inno, 0xd3, val);
		val = (cfg->fracdiv >> 8) & 0xff;
		inno_write(inno, 0xd2, val);
		val = (cfg->fracdiv >> 16) & 0xff;
		inno_write(inno, 0xd1, val);
	} else {
		inno_write(inno, 0xd3, 0);
		inno_write(inno, 0xd2, 0);
		inno_write(inno, 0xd1, 0);
	}

	/* Power up PLL */
	inno_update_bits(inno, 0xa0, 1, 0);

	/* Wait for PLL lock */
	for (val = 0; val < 5; val++) {
		if (inno_read(inno, 0xa9) & 1)
			break;
		udelay(1000);
	}
	if (val == 5) {
		printf("Pre-PLL unlock\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static unsigned long
inno_hdmi_3328_phy_pll_recalc_rate(struct inno_hdmi_phy *inno,
				   unsigned long parent_rate)
{
	unsigned long rate, vco, frac;
	u8 nd, no_a, no_b, no_d;
	__maybe_unused u8 no_c;
	u16 nf;

	nd = inno_read(inno, 0xa1) & 0x3f;
	nf = ((inno_read(inno, 0xa2) & 0x0f) << 8) | inno_read(inno, 0xa3);
	vco = parent_rate * nf;
	if ((inno_read(inno, 0xa2) & 0x30) == 0) {
		frac = inno_read(inno, 0xd3) |
		       (inno_read(inno, 0xd2) << 8) |
		       (inno_read(inno, 0xd1) << 16);
		vco += DIV_ROUND_CLOSEST(parent_rate * frac, (1 << 24));
	}
	if (inno_read(inno, 0xa0) & 2) {
		rate = vco / (nd * 5);
	} else {
		no_a = inno_read(inno, 0xa5) & 0x1f;
		no_b = ((inno_read(inno, 0xa5) >> 5) & 7) + 2;
		no_c = (1 << ((inno_read(inno, 0xa6) >> 5) & 7));
		no_d = inno_read(inno, 0xa6) & 0x1f;
		if (no_a == 1)
			rate = vco / (nd * no_b * no_d * 2);
		else
			rate = vco / (nd * no_a * no_d * 2);
	}
	inno->pixclock = rate;

	return rate;
}

static const struct inno_hdmi_phy_ops rk3228_hdmi_phy_ops = {
	.init = inno_hdmi_phy_rk3228_init,
	.power_on = inno_hdmi_phy_rk3228_power_on,
	.power_off = inno_hdmi_phy_rk3228_power_off,
	.pre_pll_update = inno_hdmi_phy_rk3228_pre_pll_update,
};

static const struct inno_hdmi_phy_ops rk3328_hdmi_phy_ops = {
	.init = inno_hdmi_phy_rk3328_init,
	.power_on = inno_hdmi_phy_rk3328_power_on,
	.power_off = inno_hdmi_phy_rk3328_power_off,
	.pre_pll_update = inno_hdmi_phy_rk3328_pre_pll_update,
	.recalc_rate = inno_hdmi_3328_phy_pll_recalc_rate,
};

static const struct inno_hdmi_phy_drv_data rk3228_hdmi_phy_drv_data = {
	.dev_type = INNO_HDMI_PHY_RK3228,
	.ops = &rk3228_hdmi_phy_ops,
	.phy_cfg_table = rk3228_phy_cfg,
};

static const struct inno_hdmi_phy_drv_data rk3328_hdmi_phy_drv_data = {
	.dev_type = INNO_HDMI_PHY_RK3328,
	.ops = &rk3328_hdmi_phy_ops,
	.phy_cfg_table = rk3328_phy_cfg,
};

static const struct rockchip_phy inno_hdmi_phy_of_match[] = {
	{ .compatible = "rockchip,rk3228-hdmi-phy",
	  .data = &rk3228_hdmi_phy_drv_data
	},
	{ .compatible = "rockchip,rk3328-hdmi-phy",
	  .data = &rk3328_hdmi_phy_drv_data
	},
	{}
};

static int inno_hdmi_phy_init(struct display_state *state)
{
	const void *blob = state->blob;
	struct connector_state *conn_state = &state->conn_state;
	int conn_node = conn_state->node;
	int node = conn_state->phy_node;
	struct inno_hdmi_phy *inno;
	int phy_node, phandle, i;
	const char *name;

	inno = malloc(sizeof(*inno));
	if (!inno)
		return -ENOMEM;

	inno->blob = blob;
	inno->node = node;

	inno->regs = fdtdec_get_addr_size_auto_noparent(blob, node, "reg",
							0, NULL);
	if (inno->regs == FDT_ADDR_T_NONE) {
		printf("%s: failed to get mipi phy address\n", __func__);
		return -ENOMEM;
	}
	conn_state->phy_private = inno;

	phandle = fdt_getprop_u32_default_node(blob, conn_node, 0,
					       "phys", -1);
	if (phandle < 0)
		return 0;

	phy_node = fdt_node_offset_by_phandle(blob, phandle);
	if (phy_node < 0) {
		printf("failed to find inno phy node\n");
		return phy_node;
	}

	fdt_get_string(blob, phy_node, "compatible", &name);
	for (i = 0; i < ARRAY_SIZE(inno_hdmi_phy_of_match); i++) {
		if (!strcmp(name, inno_hdmi_phy_of_match[i].compatible)) {
			inno->plat_data = inno_hdmi_phy_of_match[i].data;
			break;
		}
	}

	if (i >= ARRAY_SIZE(inno_hdmi_phy_of_match))
		return 0;

	if (!inno->plat_data || !inno->plat_data->ops)
		return -EINVAL;

	if (inno->plat_data->ops->init)
		inno->plat_data->ops->init(inno);

	return 0;
}

static unsigned long inno_hdmi_phy_set_pll(struct display_state *state,
					   unsigned long rate)
{
	inno_hdmi_phy_clk_prepare(state);
	inno_hdmi_phy_clk_is_prepared(state);
	inno_hdmi_phy_clk_set_rate(state, rate);
	return 0;
}

const struct rockchip_phy_funcs inno_hdmi_phy_funcs = {
	.init = inno_hdmi_phy_init,
	.power_on = inno_hdmi_phy_power_on,
	.power_off = inno_hdmi_phy_power_off,
	.set_pll = inno_hdmi_phy_set_pll,
};
