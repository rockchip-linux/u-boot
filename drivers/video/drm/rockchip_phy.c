/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <dm/device.h>
#include <errno.h>
#include <asm/unaligned.h>
#include <linux/list.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_phy.h"

#ifdef CONFIG_DRM_ROCKCHIP_DW_MIPI_DSI
static const struct rockchip_phy rockchip_inno_mipi_dphy_data = {
	 .funcs = &inno_mipi_dphy_funcs,
};
#endif

#ifdef CONFIG_ROCKCHIP_INNO_HDMI_PHY
static const struct rockchip_phy rockchip_inno_hdmi_phy_data = {
	 .funcs = &inno_hdmi_phy_funcs,
};
#endif

static const struct udevice_id rockchip_phy_ids[] = {
#ifdef CONFIG_DRM_ROCKCHIP_DW_MIPI_DSI
	{
		.compatible = "rockchip,px30-mipi-dphy",
		.data = (ulong)&rockchip_inno_mipi_dphy_data,
	},
	{
		.compatible = "rockchip,rk3128-mipi-dphy",
		.data = (ulong)&rockchip_inno_mipi_dphy_data,
	},
	{
		.compatible = "rockchip,rk3366-mipi-dphy",
		.data = (ulong)&rockchip_inno_mipi_dphy_data,
	},
	{
		.compatible = "rockchip,rk3368-mipi-dphy",
		.data = (ulong)&rockchip_inno_mipi_dphy_data,
	},
	{
		.compatible = "rockchip,rv1108-mipi-dphy",
		.data = (ulong)&rockchip_inno_mipi_dphy_data,
	},
#endif
#ifdef CONFIG_ROCKCHIP_INNO_HDMI_PHY
	{
	 .compatible = "rockchip,rk3328-hdmi-phy",
	 .data = (ulong)&rockchip_inno_hdmi_phy_data,
	},
	{
	 .compatible = "rockchip,rk3228-hdmi-phy",
	 .data = (ulong)&rockchip_inno_hdmi_phy_data,
	},

#endif
	{}
};

static int rockchip_phy_probe(struct udevice *dev)
{
	return 0;
}

static int rockchip_phy_bind(struct udevice *dev)
{
	return 0;
}

U_BOOT_DRIVER(rockchip_phy) = {
	.name = "rockchip_phy",
	.id = UCLASS_PHY,
	.of_match = rockchip_phy_ids,
	.bind	= rockchip_phy_bind,
	.probe	= rockchip_phy_probe,
};

int rockchip_phy_power_on(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->power_on) {
		printf("%s: failed to find phy power on funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->power_on(state);
}

int rockchip_phy_power_off(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->power_off) {
		printf("%s: failed to find phy power_off funcs\n", __func__);
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
		printf("%s: failed to find phy set_pll funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->set_pll(state, rate);
}

void rockchip_phy_set_bus_width(struct display_state *state, u32 bus_width)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy =
		(struct rockchip_phy *)conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->set_bus_width) {
		debug("%s: failed to find phy set_bus_width funcs\n", __func__);
		return;
	}

	return phy->funcs->set_bus_width(state, bus_width);
}

long rockchip_phy_round_rate(struct display_state *state, unsigned long rate)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy =
		(struct rockchip_phy *)conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->round_rate) {
		debug("%s: failed to find phy round_rate funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->round_rate(state, rate);
}
