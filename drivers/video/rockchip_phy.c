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
	 .compatible = "rockchip,rk3368-mipi-dphy",
	 .funcs = &rockchip_inno_mipi_dphy_funcs,
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

int rockchip_phy_get_data(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_phy *phy = conn_state->phy;

	if (!phy || !phy->funcs || !phy->funcs->get_data) {
		printf("%s: failed to find phy get_data funcs\n", __func__);
		return -ENODEV;
	}

	return phy->funcs->get_data(state);
}
