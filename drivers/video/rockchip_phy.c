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
#include "rockchip_phy.h"

static const struct rockchip_phy g_phy[] = {
#ifdef CONFIG_ROCKCHIP_DW_MIPI_DSI
	{
	 .compatible = "rockchip,rk3128-mipi-dphy",
	 .funcs = &inno_mipi_dphy_funcs,
	},
	{
	 .compatible = "rockchip,rk3366-mipi-dphy",
	 .funcs = &inno_mipi_dphy_funcs,
	},
	{
	 .compatible = "rockchip,rk3368-mipi-dphy",
	 .funcs = &inno_mipi_dphy_funcs,
	},
#endif

#ifdef CONFIG_RKCHIP_INNO_HDMI_PHY
	{
	 .compatible = "rockchip,rk3328-hdmi-phy",
	 .funcs = &inno_hdmi_phy_funcs,
	},
	{
	 .compatible = "rockchip,rk3228-hdmi-phy",
	 .funcs = &inno_hdmi_phy_funcs,
	},

#endif

};

const struct rockchip_phy *rockchip_get_phy(const void *blob, int phy_node)
{
	const char *name;
	int i;

	fdt_get_string(blob, phy_node, "compatible", &name);

	for (i = 0; i < ARRAY_SIZE(g_phy); i++) {
		if (!strcmp(name, g_phy[i].compatible))
			break;
	}
	if (i >= ARRAY_SIZE(g_phy))
		return NULL;

	return &g_phy[i];
}

int rockchip_phy_power_on(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->power_on) {
		debug("%s: failed to find phy power on funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->power_on(state);
}

int rockchip_phy_power_off(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->power_off) {
		debug("%s: failed to find phy power_off funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->power_off(state);
}

unsigned long rockchip_phy_set_pll(struct display_state *state,
				   unsigned long rate)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->set_pll) {
		debug("%s: failed to find phy set_pll funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->set_pll(state, rate);
}

void rockchip_phy_set_bus_width(struct display_state *state, u32 bus_width)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = (struct rockchip_phy *) conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->set_bus_width) {
		debug("%s: failed to find phy set_bus_width funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->set_bus_width(state, bus_width);
}

long rockchip_phy_round_rate(struct display_state *state, unsigned long rate)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = (struct rockchip_phy *) conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->round_rate) {
		debug("%s: failed to find phy round_rate funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->round_rate(state, rate);
}