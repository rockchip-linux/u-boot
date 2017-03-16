/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_PHY_H_
#define _ROCKCHIP_PHY_H_

struct rockchip_phy_funcs {
	int (*init)(struct display_state *state);
	int (*power_on)(struct display_state *state);
	void (*power_off)(struct display_state *state);
};

struct rockchip_phy {
	char compatible[30];

	struct rockchip_phy_funcs *funcs;
	const void *data;
};

const struct rockchip_phy *
rockchip_get_phy(const void *blob, int phy_node);

#ifdef CONFIG_ROCKCHIP_DW_MIPI_DSI
extern const struct rockchip_phy_funcs rockchip_inno_mipi_dphy_funcs;
#endif
#endif
