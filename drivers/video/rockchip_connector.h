/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_CONNECTOR_H_
#define _ROCKCHIP_CONNECTOR_H_

#include "rockchip_mipi_dsi.h"

struct rockchip_connector {
	char compatible[30];

	const struct rockchip_connector_funcs *funcs;
	const void *data;
};

struct rockchip_connector_funcs {
	/*
	 * init connector, prepare resource to ensure
	 * detect and get_timing can works
	 */
	int (*init)(struct display_state *state);

	void (*deinit)(struct display_state *state);
	/*
	 * Optional, if connector not support hotplug,
	 * Returns:
	 *   0 means disconnected, else means connected
	 */
	int (*detect)(struct display_state *state);
	/*
	 * Optional, if implement it, need fill the timing data:
	 *     state->conn_state->mode
	 * you can refer to the rockchip_display: display_get_timing(),
	 * Returns:
	 *   0 means success, else means failed
	 */
	int (*get_timing)(struct display_state *state);
	/*
	 * Optional, if implement it, need fill the edid data:
	 *     state->conn_state->edid
	 * Returns:
	 *   0 means success, else means failed
	 */
	int (*get_edid)(struct display_state *state);
	/*
	 * call before crtc enable.
	 */
	int (*prepare)(struct display_state *state);
	/*
	 * call after crtc enable
	 */
	int (*enable)(struct display_state *state);
	int (*disable)(struct display_state *state);
	void (*unprepare)(struct display_state *state);
	/*
	 * Save data to dts, then you can share data to kernel space.
	 */
	int (*fixup_dts)(struct display_state *state, void *blob);
	/* transmit a DSI packet */
	ssize_t (*transfer)(struct display_state *state,
			    const struct mipi_dsi_msg *msg);
};

const struct rockchip_connector *
rockchip_get_connector(const void *blob, int connector_node);

#ifdef CONFIG_ROCKCHIP_DW_MIPI_DSI
struct dw_mipi_dsi_plat_data;
extern const struct rockchip_connector_funcs rockchip_dw_mipi_dsi_funcs;
extern const struct dw_mipi_dsi_plat_data rk3288_mipi_dsi_drv_data;
extern const struct dw_mipi_dsi_plat_data rk3368_mipi_dsi_drv_data;
extern const struct dw_mipi_dsi_plat_data rk3399_mipi_dsi_drv_data;
#endif
#ifdef CONFIG_ROCKCHIP_ANALOGIX_DP
struct rockchip_dp_chip_data;
extern const struct rockchip_connector_funcs rockchip_analogix_dp_funcs;
extern const struct rockchip_dp_chip_data rk3399_analogix_edp_drv_data;
extern const struct rockchip_dp_chip_data rk3368_analogix_edp_drv_data;
extern const struct rockchip_dp_chip_data rk3288_analogix_dp_drv_data;
#endif
#ifdef CONFIG_ROCKCHIP_LVDS
struct rockchip_lvds_chip_data;
extern const struct rockchip_connector_funcs rockchip_lvds_funcs;
extern const struct rockchip_lvds_chip_data rk33xx_lvds_drv_data;
#endif
#ifdef CONFIG_ROCKCHIP_DW_HDMI
struct dw_hdmi_dsi_plat_data;
extern const struct rockchip_connector_funcs rockchip_dw_hdmi_funcs;
extern const struct dw_hdmi_plat_data rk3399_hdmi_drv_data;
extern const struct dw_hdmi_plat_data rk3288_hdmi_drv_data;
#endif
#endif
