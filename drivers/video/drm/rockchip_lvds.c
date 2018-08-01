/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <asm/unaligned.h>
#include <linux/list.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <dm/device.h>
#include <dm/read.h>
#include <dm/ofnode.h>
#include <syscon.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/gpio.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_lvds.h"

enum rockchip_lvds_sub_devtype {
	PX30_LVDS,
	RK3126_LVDS,
	RK3288_LVDS,
	RK3368_LVDS,
};

struct rockchip_lvds_chip_data {
	u32	chip_type;
	bool	has_vop_sel;
	u32	grf_soc_con5;
	u32	grf_soc_con6;
	u32	grf_soc_con7;
	u32	grf_soc_con15;
	u32	grf_gpio1d_iomux;
};

struct rockchip_lvds_device {
	void	*regbase;
	void	*grf;
	void	*ctrl_reg;
	u32	channel;
	u32	output;
	u32	format;
	struct drm_display_mode *mode;
	const struct rockchip_lvds_chip_data *pdata;
};

static inline int lvds_name_to_format(const char *s)
{
	if (!s)
		return -EINVAL;

	if (strncmp(s, "jeida", 6) == 0)
		return LVDS_FORMAT_JEIDA;
	else if (strncmp(s, "vesa", 5) == 0)
		return LVDS_FORMAT_VESA;

	return -EINVAL;
}

static inline int lvds_name_to_output(const char *s)
{
	if (!s)
		return -EINVAL;

	if (strncmp(s, "rgb", 3) == 0)
		return DISPLAY_OUTPUT_RGB;
	else if (strncmp(s, "lvds", 4) == 0)
		return DISPLAY_OUTPUT_LVDS;
	else if (strncmp(s, "duallvds", 8) == 0)
		return DISPLAY_OUTPUT_DUAL_LVDS;

	return -EINVAL;
}

static inline void lvds_writel(struct rockchip_lvds_device *lvds,
			      u32 offset, u32 val)
{
	writel(val, lvds->regbase + offset);

	if ((lvds->pdata->chip_type == RK3288_LVDS) &&
	    (lvds->output == DISPLAY_OUTPUT_DUAL_LVDS))
		writel(val, lvds->regbase + offset + 0x100);
}

static inline void lvds_msk_reg(struct rockchip_lvds_device *lvds, u32 offset,
			       u32 msk, u32 val)
{
	u32 temp;

	temp = readl(lvds->regbase + offset) & (0xFF - (msk));
	writel(temp | ((val) & (msk)), lvds->regbase + offset);
}

static inline u32 lvds_readl(struct rockchip_lvds_device *lvds, u32 offset)
{
	return readl(lvds->regbase + offset);
}

static inline void lvds_ctrl_writel(struct rockchip_lvds_device *lvds,
				   u32 offset, u32 val)
{
	writel(val, lvds->ctrl_reg + offset);
}

static inline u32 lvds_pmugrf_readl(u32 offset)
{
	return readl((void *)LVDS_PMUGRF_BASE + offset);
}

static inline void lvds_pmugrf_writel(u32 offset, u32 val)
{
	writel(val, (void *)LVDS_PMUGRF_BASE + offset);
}

static inline u32 lvds_phy_lock(struct rockchip_lvds_device *lvds)
{
	u32 val = 0;
	val = readl(lvds->ctrl_reg + MIPIC_PHY_STATUS);
	return (val & m_PHY_LOCK_STATUS) ? 1 : 0;
}

static int rockchip_lvds_clk_enable(struct rockchip_lvds_device *lvds)
{
	return 0;
}

static int rk336x_lvds_pwr_off(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	/* disable lvds lane and power off pll */
	lvds_writel(lvds, MIPIPHY_REGEB,
		    v_LANE0_EN(0) | v_LANE1_EN(0) | v_LANE2_EN(0) |
		    v_LANE3_EN(0) | v_LANECLK_EN(0) | v_PLL_PWR_OFF(1));

	/* power down lvds pll and bandgap */
	lvds_msk_reg(lvds, MIPIPHY_REG1,
		     m_SYNC_RST | m_LDO_PWR_DOWN | m_PLL_PWR_DOWN,
		     v_SYNC_RST(1) | v_LDO_PWR_DOWN(1) | v_PLL_PWR_DOWN(1));

	/* disable lvds */
	lvds_msk_reg(lvds, MIPIPHY_REGE3, m_LVDS_EN | m_TTL_EN,
		     v_LVDS_EN(0) | v_TTL_EN(0));

	return 0;
}

static int rk3288_lvds_pwr_off(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	lvds_writel(lvds, RK3288_LVDS_CFG_REG21, RK3288_LVDS_CFG_REG21_TX_DISABLE);
	lvds_writel(lvds, RK3288_LVDS_CFG_REGC, RK3288_LVDS_CFG_REGC_PLL_DISABLE);

	writel(0xffff8000, lvds->grf + lvds->pdata->grf_soc_con7);

	return 0;
}

static int rk336x_lvds_pwr_on(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	u32 delay_times = 20;

	if (lvds->output == DISPLAY_OUTPUT_LVDS) {
		/* set VOCM 900 mv and V-DIFF 350 mv */
		lvds_msk_reg(lvds, MIPIPHY_REGE4, m_VOCM | m_DIFF_V,
			     v_VOCM(0) | v_DIFF_V(2));
		/* power up lvds pll and ldo */
		lvds_msk_reg(lvds, MIPIPHY_REG1,
			     m_SYNC_RST | m_LDO_PWR_DOWN | m_PLL_PWR_DOWN,
			     v_SYNC_RST(0) | v_LDO_PWR_DOWN(0) |
			     v_PLL_PWR_DOWN(0));
		/* enable lvds lane and power on pll */
		lvds_writel(lvds, MIPIPHY_REGEB,
			    v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) |
			    v_LANE3_EN(1) | v_LANECLK_EN(1) | v_PLL_PWR_OFF(0));

		/* enable lvds */
		lvds_msk_reg(lvds, MIPIPHY_REGE3,
			     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
			     v_MIPI_EN(0) | v_LVDS_EN(1) | v_TTL_EN(0));
	} else {
		lvds_msk_reg(lvds, MIPIPHY_REGE3,
			     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
			     v_MIPI_EN(0) | v_LVDS_EN(0) | v_TTL_EN(1));

		/* set clock lane enable */
		lvds_ctrl_writel(lvds, 0xa0, 0x4);
	}
	/* delay for waitting pll lock on */
	while (delay_times--) {
		if (lvds_phy_lock(lvds))
			break;
		udelay(100);
	}

	if (delay_times <= 0)
		printf("wait lvds phy lock failed, please check the hardware!\n");

	return 0;
}

static void px30_output_ttl(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	u32 val = 0;

	/* enable lvds mode */
	val = PX30_RGB_SYNC_BYPASS(1) | PX30_DPHY_FORCERXMODE(1);
	writel(val, lvds->grf + PX30_GRF_PD_VO_CON1);

	/* enable lane */
	lvds_writel(lvds, MIPIPHY_REG0, 0x7f);
	val = v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) | v_LANE3_EN(1) |
		v_LANECLK_EN(1) | v_PLL_PWR_OFF(1);
	lvds_writel(lvds, MIPIPHY_REGEB, val);
	/* set ttl mode and reset phy config */
	val = v_LVDS_MODE_EN(0) | v_TTL_MODE_EN(1) | v_MIPI_MODE_EN(0) |
		v_MSB_SEL(1) | v_DIG_INTER_RST(1);
	lvds_writel(lvds, MIPIPHY_REGE0, val);
	rk336x_lvds_pwr_on(state);
}

static void rk3126_output_ttl(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	u32 val = 0;

	/* enable lvds mode */
	val = v_RK3126_LVDSMODE_EN(0) |
		v_RK3126_MIPIPHY_TTL_EN(1) |
		v_RK3126_MIPIPHY_LANE0_EN(1) |
		v_RK3126_MIPIDPI_FORCEX_EN(1);
	writel(val, lvds->grf + lvds->pdata->grf_soc_con7);
	val = v_RK3126_MIPITTL_CLK_EN(1) |
		v_RK3126_MIPITTL_LANE0_EN(1) |
		v_RK3126_MIPITTL_LANE1_EN(1) |
		v_RK3126_MIPITTL_LANE2_EN(1) |
		v_RK3126_MIPITTL_LANE3_EN(1);
	writel(val, lvds->grf + lvds->pdata->grf_soc_con15);
	/* enable lane */
	lvds_writel(lvds, MIPIPHY_REG0, 0x7f);
	val = v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) | v_LANE3_EN(1) |
		v_LANECLK_EN(1) | v_PLL_PWR_OFF(1);
	lvds_writel(lvds, MIPIPHY_REGEB, val);
	/* set ttl mode and reset phy config */
	val = v_LVDS_MODE_EN(0) | v_TTL_MODE_EN(1) | v_MIPI_MODE_EN(0) |
		v_MSB_SEL(1) | v_DIG_INTER_RST(1);
	lvds_writel(lvds, MIPIPHY_REGE0, val);
	rk336x_lvds_pwr_on(state);
}

static void rk336x_output_ttl(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	u32 val = 0;

	/* enable lvds mode */
	val = v_RK336X_LVDSMODE_EN(0) | v_RK336X_MIPIPHY_TTL_EN(1) |
		v_RK336X_MIPIPHY_LANE0_EN(1) |
		v_RK336X_MIPIDPI_FORCEX_EN(1);
	writel(val, lvds->grf + lvds->pdata->grf_soc_con7);
	val = v_RK336X_FORCE_JETAG(0);
	writel(val, lvds->grf + lvds->pdata->grf_soc_con15);

	/* enable lane */
	lvds_writel(lvds, MIPIPHY_REG0, 0x7f);
	val = v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) | v_LANE3_EN(1) |
		v_LANECLK_EN(1) | v_PLL_PWR_OFF(1);
	lvds_writel(lvds, MIPIPHY_REGEB, val);

	/* set ttl mode and reset phy config */
	val = v_LVDS_MODE_EN(0) | v_TTL_MODE_EN(1) | v_MIPI_MODE_EN(0) |
		v_MSB_SEL(1) | v_DIG_INTER_RST(1);
	lvds_writel(lvds, MIPIPHY_REGE0, val);

	rk336x_lvds_pwr_on(state);
}

static void px30_output_lvds(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	u32 val = 0;

	/* enable lvds mode */
	val = PX30_LVDS_PHY_MODE(1) | PX30_DPHY_FORCERXMODE(1);
	/* config lvds_format */
	val |= PX30_LVDS_OUTPUT_FORMAT(lvds->format);
	/* LSB receive mode */
	val |= PX30_LVDS_MSBSEL(LVDS_MSB_D7);
	writel(val, lvds->grf + PX30_GRF_PD_VO_CON1);

	/* digital internal disable */
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(0));

	/* set pll prediv and fbdiv */
	lvds_writel(lvds, MIPIPHY_REG3, v_PREDIV(2) | v_FBDIV_MSB(0));
	lvds_writel(lvds, MIPIPHY_REG4, v_FBDIV_LSB(28));

	lvds_writel(lvds, MIPIPHY_REGE8, 0xfc);

	lvds_msk_reg(lvds, MIPIPHY_REG8,
		     m_SAMPLE_CLK_DIR, v_SAMPLE_CLK_DIR_REVERSE);

	/* set lvds mode and reset phy config */
	lvds_msk_reg(lvds, MIPIPHY_REGE0,
		     m_MSB_SEL | m_DIG_INTER_RST,
		     v_MSB_SEL(1) | v_DIG_INTER_RST(1));

	rk336x_lvds_pwr_on(state);
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(1));
}

static void rk3126_output_lvds(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	u32 val = 0;

	/* enable lvds mode */
	val = v_RK3126_LVDSMODE_EN(1) |
	      v_RK3126_MIPIPHY_TTL_EN(0);
	/* config lvds_format */
	val |= v_RK3126_LVDS_OUTPUT_FORMAT(lvds->format);
	/* LSB receive mode */
	val |= v_RK3126_LVDS_MSBSEL(LVDS_MSB_D7);
	val |= v_RK3126_MIPIPHY_LANE0_EN(1) |
	       v_RK3126_MIPIDPI_FORCEX_EN(1);
	writel(val, lvds->grf + lvds->pdata->grf_soc_con7);

	/* digital internal disable */
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(0));

	/* set pll prediv and fbdiv */
	lvds_writel(lvds, MIPIPHY_REG3, v_PREDIV(2) | v_FBDIV_MSB(0));
	lvds_writel(lvds, MIPIPHY_REG4, v_FBDIV_LSB(28));

	lvds_writel(lvds, MIPIPHY_REGE8, 0xfc);

	/* set lvds mode and reset phy config */
	lvds_msk_reg(lvds, MIPIPHY_REGE0,
		     m_MSB_SEL | m_DIG_INTER_RST,
		     v_MSB_SEL(1) | v_DIG_INTER_RST(1));

	rk336x_lvds_pwr_on(state);
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(1));
}

static void rk336x_output_lvds(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	u32 val = 0;

	/* enable lvds mode */
	val |= v_RK336X_LVDSMODE_EN(1) | v_RK336X_MIPIPHY_TTL_EN(0);
	/* config lvds_format */
	val |= v_RK336X_LVDS_OUTPUT_FORMAT(lvds->format);
	/* LSB receive mode */
	val |= v_RK336X_LVDS_MSBSEL(LVDS_MSB_D7);
	val |= v_RK336X_MIPIPHY_LANE0_EN(1) |
	       v_RK336X_MIPIDPI_FORCEX_EN(1);
	writel(val, lvds->grf + lvds->pdata->grf_soc_con7);
	/* digital internal disable */
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(0));

	/* set pll prediv and fbdiv */
	lvds_writel(lvds, MIPIPHY_REG3, v_PREDIV(2) | v_FBDIV_MSB(0));
	lvds_writel(lvds, MIPIPHY_REG4, v_FBDIV_LSB(28));

	lvds_writel(lvds, MIPIPHY_REGE8, 0xfc);

	/* set lvds mode and reset phy config */
	lvds_msk_reg(lvds, MIPIPHY_REGE0,
		     m_MSB_SEL | m_DIG_INTER_RST,
		     v_MSB_SEL(1) | v_DIG_INTER_RST(1));

	rk336x_lvds_pwr_on(state);
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(1));
}

static int rk3288_lvds_pwr_on(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	struct drm_display_mode *mode = &conn_state->mode;
	u32 val;
	u32 h_bp = mode->htotal - mode->hsync_start;
	u8 pin_hsync = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 1 : 0;
	u8 pin_dclk = (mode->flags & DRM_MODE_FLAG_PCSYNC) ? 1 : 0;

	val = lvds->format;
	if (lvds->output == DISPLAY_OUTPUT_DUAL_LVDS)
		val |= LVDS_DUAL | LVDS_CH0_EN | LVDS_CH1_EN;
	else if (lvds->output == DISPLAY_OUTPUT_LVDS)
		val |= LVDS_CH0_EN;
	else if (lvds->output == DISPLAY_OUTPUT_RGB)
		val |= LVDS_TTL_EN | LVDS_CH0_EN | LVDS_CH1_EN;

	if (h_bp & 0x01)
		val |= LVDS_START_PHASE_RST_1;

	val |= (pin_dclk << 8) | (pin_hsync << 9);
	val |= (0xffff << 16);
	writel(val, lvds->grf + lvds->pdata->grf_soc_con7);

	return 0;
}

static void rk3288_output_ttl(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	rk3288_lvds_pwr_on(state);
	lvds_writel(lvds, RK3288_LVDS_CH0_REG0,
		    RK3288_LVDS_CH0_REG0_TTL_EN |
		    RK3288_LVDS_CH0_REG0_LANECK_EN |
		    RK3288_LVDS_CH0_REG0_LANE4_EN |
		    RK3288_LVDS_CH0_REG0_LANE3_EN |
		    RK3288_LVDS_CH0_REG0_LANE2_EN |
		    RK3288_LVDS_CH0_REG0_LANE1_EN |
		    RK3288_LVDS_CH0_REG0_LANE0_EN);
	lvds_writel(lvds, RK3288_LVDS_CH0_REG2,
		    RK3288_LVDS_PLL_FBDIV_REG2(0x46));

	lvds_writel(lvds, RK3288_LVDS_CH0_REG3,
		    RK3288_LVDS_PLL_FBDIV_REG3(0x46));
	lvds_writel(lvds, RK3288_LVDS_CH0_REG4,
		    RK3288_LVDS_CH0_REG4_LANECK_TTL_MODE |
		    RK3288_LVDS_CH0_REG4_LANE4_TTL_MODE |
		    RK3288_LVDS_CH0_REG4_LANE3_TTL_MODE |
		    RK3288_LVDS_CH0_REG4_LANE2_TTL_MODE |
		    RK3288_LVDS_CH0_REG4_LANE1_TTL_MODE |
		    RK3288_LVDS_CH0_REG4_LANE0_TTL_MODE);
	lvds_writel(lvds, RK3288_LVDS_CH0_REG5,
		    RK3288_LVDS_CH0_REG5_LANECK_TTL_DATA |
		    RK3288_LVDS_CH0_REG5_LANE4_TTL_DATA |
		    RK3288_LVDS_CH0_REG5_LANE3_TTL_DATA |
		    RK3288_LVDS_CH0_REG5_LANE2_TTL_DATA |
		    RK3288_LVDS_CH0_REG5_LANE1_TTL_DATA |
		    RK3288_LVDS_CH0_REG5_LANE0_TTL_DATA);
	lvds_writel(lvds, RK3288_LVDS_CH0_REGD,
		    RK3288_LVDS_PLL_PREDIV_REGD(0x0a));
	lvds_writel(lvds, RK3288_LVDS_CH0_REG20,
		    RK3288_LVDS_CH0_REG20_LSB);

	lvds_writel(lvds, RK3288_LVDS_CFG_REGC, RK3288_LVDS_CFG_REGC_PLL_ENABLE);
	lvds_writel(lvds, RK3288_LVDS_CFG_REG21, RK3288_LVDS_CFG_REG21_TX_ENABLE);
}

static void rk3288_output_lvds(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	rk3288_lvds_pwr_on(state);

	lvds_writel(lvds, RK3288_LVDS_CH0_REG0,
		    RK3288_LVDS_CH0_REG0_LVDS_EN |
		    RK3288_LVDS_CH0_REG0_LANECK_EN |
		    RK3288_LVDS_CH0_REG0_LANE4_EN |
		    RK3288_LVDS_CH0_REG0_LANE3_EN |
		    RK3288_LVDS_CH0_REG0_LANE2_EN |
		    RK3288_LVDS_CH0_REG0_LANE1_EN |
		    RK3288_LVDS_CH0_REG0_LANE0_EN);
	lvds_writel(lvds, RK3288_LVDS_CH0_REG1,
		    RK3288_LVDS_CH0_REG1_LANECK_BIAS |
		    RK3288_LVDS_CH0_REG1_LANE4_BIAS |
		    RK3288_LVDS_CH0_REG1_LANE3_BIAS |
		    RK3288_LVDS_CH0_REG1_LANE2_BIAS |
		    RK3288_LVDS_CH0_REG1_LANE1_BIAS |
		    RK3288_LVDS_CH0_REG1_LANE0_BIAS);
	lvds_writel(lvds, RK3288_LVDS_CH0_REG2,
		    RK3288_LVDS_CH0_REG2_RESERVE_ON |
		    RK3288_LVDS_CH0_REG2_LANECK_LVDS_MODE |
		    RK3288_LVDS_CH0_REG2_LANE4_LVDS_MODE |
		    RK3288_LVDS_CH0_REG2_LANE3_LVDS_MODE |
		    RK3288_LVDS_CH0_REG2_LANE2_LVDS_MODE |
		    RK3288_LVDS_CH0_REG2_LANE1_LVDS_MODE |
		    RK3288_LVDS_CH0_REG2_LANE0_LVDS_MODE |
		    RK3288_LVDS_PLL_FBDIV_REG2(0x46));
	lvds_writel(lvds, RK3288_LVDS_CH0_REG3,
		    RK3288_LVDS_PLL_FBDIV_REG3(0x46));
	lvds_writel(lvds, RK3288_LVDS_CH0_REG4, 0x00);
	lvds_writel(lvds, RK3288_LVDS_CH0_REG5, 0x00);
	lvds_writel(lvds, RK3288_LVDS_CH0_REGD,
		    RK3288_LVDS_PLL_PREDIV_REGD(0x0a));
	lvds_writel(lvds, RK3288_LVDS_CH0_REG20,
		    RK3288_LVDS_CH0_REG20_LSB);

	lvds_writel(lvds, RK3288_LVDS_CFG_REGC, RK3288_LVDS_CFG_REGC_PLL_ENABLE);
	lvds_writel(lvds, RK3288_LVDS_CFG_REG21, RK3288_LVDS_CFG_REG21_TX_ENABLE);
}

static int rockchip_lvds_init(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	const struct rockchip_lvds_chip_data *pdata = connector->data;
	struct rockchip_lvds_device *lvds;
	const char *name;
	int i, width;
	struct resource lvds_phy, lvds_ctrl;
	struct panel_state *panel_state = &state->panel_state;
	ofnode panel_node = panel_state->node;
	int ret;

	lvds = malloc(sizeof(*lvds));
	if (!lvds)
		return -ENOMEM;
	lvds->pdata = pdata;

	if (pdata->chip_type == RK3288_LVDS) {
		lvds->regbase = dev_read_addr_ptr(conn_state->dev);
	} else {
		i = dev_read_resource(conn_state->dev, 0, &lvds_phy);
		if (i) {
			printf("can't get regs lvds_phy addresses!\n");
			free(lvds);
			return -ENOMEM;
		}

		i = dev_read_resource(conn_state->dev, 1, &lvds_ctrl);
		if (i) {
			printf("can't get regs lvds_ctrl addresses!\n");
			free(lvds);
			return -ENOMEM;
		}

		lvds->regbase = (void *)lvds_phy.start;
		lvds->ctrl_reg = (void *)lvds_ctrl.start;
	}

	lvds->grf = syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
	if (lvds->grf <= 0) {
		printf("%s: Get syscon grf failed (ret=%p)\n",
		      __func__, lvds->grf);
		return  -ENXIO;
	}

	ret = dev_read_string_index(panel_state->dev, "rockchip,output", 0, &name);
	if (ret)
		/* default set it as output rgb */
		lvds->output = DISPLAY_OUTPUT_RGB;
	else
		lvds->output = lvds_name_to_output(name);
	if (lvds->output < 0) {
		printf("invalid output type [%s]\n", name);
		free(lvds);
		return lvds->output;
	}
	ret = dev_read_string_index(panel_state->dev, "rockchip,data-mapping", 0, &name);
	if (ret)
		/* default set it as format jeida */
		lvds->format = LVDS_FORMAT_JEIDA;
	else
		lvds->format = lvds_name_to_format(name);

	if (lvds->format < 0) {
		printf("invalid data-mapping format [%s]\n", name);
		free(lvds);
		return lvds->format;
	}
	width = ofnode_read_u32_default(panel_node, "rockchip,data-width", 24);
	if (width == 24) {
		lvds->format |= LVDS_24BIT;
	} else if (width == 18) {
		lvds->format |= LVDS_18BIT;
	} else {
		printf("rockchip-lvds unsupport data-width[%d]\n", width);
		free(lvds);
		return -EINVAL;
	}

	printf("LVDS: data mapping: %s, data-width:%d, format:%d,\n",
		name, width, lvds->format);
	conn_state->private = lvds;
	conn_state->type = DRM_MODE_CONNECTOR_LVDS;

	if ((lvds->output == DISPLAY_OUTPUT_RGB) && (width == 18))
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P666;
	else
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P888;
	conn_state->color_space = V4L2_COLORSPACE_DEFAULT;

	return 0;
}

static void rockchip_lvds_deinit(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	free(lvds);
}

static int rockchip_lvds_prepare(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	lvds->mode = &conn_state->mode;

	rockchip_lvds_clk_enable(lvds);

	return 0;
}

static void rockchip_lvds_vop_routing(struct rockchip_lvds_device *lvds, int pipe)
{
	u32 val;

	if (lvds->pdata->chip_type == RK3288_LVDS) {
		if (pipe)
			val = RK3288_LVDS_SOC_CON6_SEL_VOP_LIT |
				(RK3288_LVDS_SOC_CON6_SEL_VOP_LIT << 16);
		else
			val = RK3288_LVDS_SOC_CON6_SEL_VOP_LIT << 16;
		writel(val, lvds->grf + lvds->pdata->grf_soc_con6);
	} else if (lvds->pdata->chip_type == PX30_LVDS) {
		if (lvds->output == DISPLAY_OUTPUT_RGB)
			writel(PX30_RGB_VOP_SEL(pipe),
			       lvds->grf + PX30_GRF_PD_VO_CON1);
		else if (lvds->output == DISPLAY_OUTPUT_LVDS)
			writel(PX30_LVDS_VOP_SEL(pipe),
			       lvds->grf + PX30_GRF_PD_VO_CON1);
	}
}

static int rockchip_lvds_enable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;
	struct crtc_state *crtc_state = &state->crtc_state;

	if (lvds->pdata->has_vop_sel)
		rockchip_lvds_vop_routing(lvds, crtc_state->crtc_id);

	if ((lvds->output == DISPLAY_OUTPUT_LVDS) ||
	    (lvds->output == DISPLAY_OUTPUT_DUAL_LVDS)) {
		if (lvds->pdata->chip_type == RK3288_LVDS)
			rk3288_output_lvds(state);
		else if (lvds->pdata->chip_type == RK3126_LVDS)
			rk3126_output_lvds(state);
		else if (lvds->pdata->chip_type == PX30_LVDS)
			px30_output_lvds(state);
		else
			rk336x_output_lvds(state);
	} else {
		if (lvds->pdata->chip_type == RK3288_LVDS)
			rk3288_output_ttl(state);
		else if (lvds->pdata->chip_type == RK3126_LVDS)
			rk3126_output_ttl(state);
		else if (lvds->pdata->chip_type == PX30_LVDS)
			px30_output_ttl(state);
		else
			rk336x_output_ttl(state);
	}

	return 0;
}

static int rockchip_lvds_disable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	if (lvds->pdata->chip_type == RK3288_LVDS)
		rk3288_lvds_pwr_off(state);
	else
		rk336x_lvds_pwr_off(state);

	return 0;
}

const struct rockchip_connector_funcs rockchip_lvds_funcs = {
	.init = rockchip_lvds_init,
	.deinit = rockchip_lvds_deinit,
	.prepare = rockchip_lvds_prepare,
	.enable = rockchip_lvds_enable,
	.disable = rockchip_lvds_disable,
};

static const struct rockchip_lvds_chip_data px30_lvds_drv_data = {
	.chip_type = PX30_LVDS,
	.has_vop_sel = true,
};

static const struct rockchip_connector px30_lvds_data = {
	 .funcs = &rockchip_lvds_funcs,
	 .data = &px30_lvds_drv_data,
};

static const struct rockchip_lvds_chip_data rk3126_lvds_drv_data = {
	.chip_type = RK3126_LVDS,
	.grf_soc_con7  = RK3126_GRF_LVDS_CON0,
	.grf_soc_con15 = RK3126_GRF_CON1,
	.has_vop_sel = true,
};

static const struct rockchip_connector rk3126_lvds_data = {
	 .funcs = &rockchip_lvds_funcs,
	 .data = &rk3126_lvds_drv_data,
};

static const struct rockchip_lvds_chip_data rk3288_lvds_drv_data = {
	.chip_type = RK3288_LVDS,
	.has_vop_sel = true,
	.grf_soc_con6 = 0x025c,
	.grf_soc_con7 = 0x0260,
	.grf_gpio1d_iomux = 0x000c,
};

static const struct rockchip_connector rk3288_lvds_data = {
	 .funcs = &rockchip_lvds_funcs,
	 .data = &rk3288_lvds_drv_data,
};

static const struct rockchip_lvds_chip_data rk3368_lvds_drv_data = {
	.chip_type = RK3368_LVDS,
	.grf_soc_con7  = RK3368_GRF_SOC_CON7,
	.grf_soc_con15 = RK3368_GRF_SOC_CON15,
	.has_vop_sel = false,
};

static const struct rockchip_connector rk3368_lvds_data = {
	 .funcs = &rockchip_lvds_funcs,
	 .data = &rk3368_lvds_drv_data,
};

static const struct udevice_id rockchip_lvds_ids[] = {
	{
		.compatible = "rockchip,px30-lvds",
		.data = (ulong)&px30_lvds_data,
	},
	{
		.compatible = "rockchip,rk3126-lvds",
		.data = (ulong)&rk3126_lvds_data,
	},
	{
		.compatible = "rockchip,rk3288-lvds",
		.data = (ulong)&rk3288_lvds_data,
	},
	{
		.compatible = "rockchip,rk3368-lvds",
		.data = (ulong)&rk3368_lvds_data,
	},
	{}
};

U_BOOT_DRIVER(rockchip_lvds) = {
	.name = "rockchip_lvds",
	.id = UCLASS_DISPLAY,
	.of_match = rockchip_lvds_ids,
};
