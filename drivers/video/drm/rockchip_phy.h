/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_PHY_H_
#define _ROCKCHIP_PHY_H_

struct rockchip_phy_funcs {
	int (*init)(struct display_state *state);
	int (*power_on)(struct display_state *state);
	int (*power_off)(struct display_state *state);
	unsigned long (*set_pll)(struct display_state *state,
				 unsigned long rate);
	void (*set_bus_width)(struct display_state *state, u32 bus_width);
	long (*round_rate)(struct display_state *state, unsigned long rate);
};

struct rockchip_phy {
	const struct rockchip_phy_funcs *funcs;
	const void *data;
};

const struct rockchip_phy *
rockchip_get_phy(const void *blob, int phy_node);
int rockchip_phy_power_off(struct display_state *state);
int rockchip_phy_power_on(struct display_state *state);
unsigned long rockchip_phy_set_pll(struct display_state *state,
				   unsigned long rate);
void rockchip_phy_set_bus_width(struct display_state *state, u32 bus_width);
long rockchip_phy_round_rate(struct display_state *state, unsigned long rate);

#ifdef CONFIG_DRM_ROCKCHIP_DW_MIPI_DSI
extern const struct rockchip_phy_funcs inno_mipi_dphy_funcs;
#endif
#ifdef CONFIG_ROCKCHIP_INNO_HDMI_PHY
extern const struct rockchip_phy_funcs inno_hdmi_phy_funcs;
#endif
#endif
