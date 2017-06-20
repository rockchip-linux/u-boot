/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
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
#include <asm/io.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_lvds.h"

enum rockchip_lvds_sub_devtype {
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
	u32	pmugrf_gpio0b_iomux;
	u32	pmugrf_gpio0c_iomux;
	u32	pmugrf_gpio0d_iomux;
};

struct rockchip_lvds_device {
	u32	regbase;
	u32	ctrl_reg;
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
	return readl(LVDS_PMUGRF_BASE + offset);
}

static inline void lvds_pmugrf_writel(u32 offset, u32 val)
{
	writel(val, LVDS_PMUGRF_BASE + offset);
}

static inline u32 lvds_phy_lock(struct rockchip_lvds_device *lvds)
{
	u32 val = 0;
	val = readl(lvds->ctrl_reg + 0x10);
	val &= 0x1;
	return val;
}

static int rockchip_lvds_clk_enable(struct rockchip_lvds_device *lvds)
{
	return 0;
}

const struct rockchip_lvds_chip_data rk33xx_lvds_drv_data = {
	.chip_type = RK3368_LVDS,
	.has_vop_sel = false,
	.grf_soc_con7  = 0x041c,
	.grf_soc_con15 = 0x043c,
	.pmugrf_gpio0b_iomux = 0x4,
	.pmugrf_gpio0c_iomux = 0x8,
	.pmugrf_gpio0d_iomux = 0xc,
};

static int rockchip_lvds_pwr_off(struct rockchip_lvds_device *lvds)
{
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

static int rockchip_lvds_pwr_on(struct rockchip_lvds_device *lvds)
{
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


static void rockchip_output_ttl(struct rockchip_lvds_device *lvds)
{
	u32 val = 0;

	/* iomux to lcdc */
	val = 0xf0005000;/*lcdc data 11 10*/
	lvds_pmugrf_writel(lvds->pdata->pmugrf_gpio0b_iomux, val);

	val = 0xFFFF5555;/*lcdc data 12 13 14 15 16 17 18 19*/
	lvds_pmugrf_writel(lvds->pdata->pmugrf_gpio0c_iomux, val);

	val = 0xFFFF5555;/*lcdc data 20 21 22 23 HSYNC VSYNC DEN DCLK*/
	lvds_pmugrf_writel(lvds->pdata->pmugrf_gpio0d_iomux, val);

	lvds_ctrl_writel(lvds, 0x0, 0x4);/*set clock lane enable*/
	/* enable lvds mode */
	val = v_RK3368_LVDSMODE_EN(0) | v_RK3368_MIPIPHY_TTL_EN(1) |
		v_RK3368_MIPIPHY_LANE0_EN(1) |
		v_RK3368_MIPIDPI_FORCEX_EN(1);
	grf_writel(val, lvds->pdata->grf_soc_con7);
	val = v_RK3368_FORCE_JETAG(0);
	grf_writel(val, lvds->pdata->grf_soc_con15);

	/* enable lane */
	lvds_writel(lvds, MIPIPHY_REG0, 0x7f);
	val = v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) | v_LANE3_EN(1) |
		v_LANECLK_EN(1) | v_PLL_PWR_OFF(1);
	lvds_writel(lvds, MIPIPHY_REGEB, val);

	/* set ttl mode and reset phy config */
	val = v_LVDS_MODE_EN(0) | v_TTL_MODE_EN(1) | v_MIPI_MODE_EN(0) |
		v_MSB_SEL(1) | v_DIG_INTER_RST(1);
	lvds_writel(lvds, MIPIPHY_REGE0, val);

	rockchip_lvds_pwr_on(lvds);
}


static void rockchip_output_lvds(struct rockchip_lvds_device *lvds)
{
	u32 val = 0;

	/* enable lvds mode */
	val |= v_RK3368_LVDSMODE_EN(1) | v_RK3368_MIPIPHY_TTL_EN(0);
	/* config lvds_format */
	val |= v_RK3368_LVDS_OUTPUT_FORMAT(lvds->format);
	/* LSB receive mode */
	val |= v_RK3368_LVDS_MSBSEL(LVDS_MSB_D7);
	val |= v_RK3368_MIPIPHY_LANE0_EN(1) |
	       v_RK3368_MIPIDPI_FORCEX_EN(1);
	grf_writel(val, lvds->pdata->grf_soc_con7);
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

	rockchip_lvds_pwr_on(lvds);
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(1));
}


static int rockchip_lvds_init(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	const struct rockchip_lvds_chip_data *pdata = connector->data;
	int lvds_node = conn_state->node;
	struct rockchip_lvds_device *lvds;
	const char *name;
	int i;
	struct fdt_resource lvds_phy, lvds_ctrl;
	struct panel_state *panel_state = &state->panel_state;
	int panel_node = panel_state->node;

	lvds = malloc(sizeof(*lvds));
	if (!lvds)
		return -ENOMEM;

	i = fdt_get_named_resource(state->blob, lvds_node, "reg", "reg-names",
				   "mipi_lvds_phy", &lvds_phy);
	if (i) {
		printf("can't get regs lvds_phy addresses!\n");
		free(lvds);
		return -ENOMEM;
	}

	i = fdt_get_named_resource(state->blob, lvds_node, "reg", "reg-names",
				   "mipi_lvds_ctl", &lvds_ctrl);
	if (i) {
		printf("can't get regs lvds_ctrl addresses!\n");
		free(lvds);
		return -ENOMEM;
	}

	lvds->regbase = lvds_phy.start;
	lvds->ctrl_reg = lvds_ctrl.start;
	lvds->pdata = pdata;

	fdt_get_string(state->blob, panel_node, "rockchip,output", &name);
	if (fdt_get_string(state->blob, panel_node, "rockchip,output", &name))
		/* default set it as output rgb */
		lvds->output = DISPLAY_OUTPUT_RGB;
	else
		lvds->output = lvds_name_to_output(name);
	if (lvds->output < 0) {
		printf("invalid output type [%s]\n", name);
		free(lvds);
		return lvds->output;
	}
	if (fdt_get_string(state->blob, panel_node, "rockchip,data-mapping",
			   &name))
		/* default set it as format jeida */
		lvds->format = LVDS_FORMAT_JEIDA;
	else
		lvds->format = lvds_name_to_format(name);

	if (lvds->format < 0) {
		printf("invalid data-mapping format [%s]\n", name);
		free(lvds);
		return lvds->format;
	}
	i = fdtdec_get_int(state->blob, panel_node, "rockchip,data-width", 24);
	if (i == 24) {
		lvds->format |= LVDS_24BIT;
	} else if (i == 18) {
		lvds->format |= LVDS_18BIT;
	} else {
		printf("rockchip-lvds unsupport data-width[%d]\n", i);
		free(lvds);
		return -EINVAL;
	}
	printf("LVDS: data mapping: %s, data-width:%d, format:%d,\n",
		name, i, lvds->format);
	conn_state->private = lvds;
	conn_state->type = DRM_MODE_CONNECTOR_LVDS;
	conn_state->output_mode = ROCKCHIP_OUT_MODE_P888;

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
static int rockchip_lvds_enable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	if (lvds->output == DISPLAY_OUTPUT_LVDS)
		rockchip_output_lvds(lvds);
	else
		rockchip_output_ttl(lvds);

	return 0;
}

static int rockchip_lvds_disable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_lvds_device *lvds = conn_state->private;

	rockchip_lvds_pwr_off(lvds);

	return 0;
}

const struct rockchip_connector_funcs rockchip_lvds_funcs = {
	.init = rockchip_lvds_init,
	.deinit = rockchip_lvds_deinit,
	.prepare = rockchip_lvds_prepare,
	.enable = rockchip_lvds_enable,
	.disable = rockchip_lvds_disable,
};
