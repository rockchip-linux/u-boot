/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_CONNECTOR_H_
#define _ROCKCHIP_CONNECTOR_H_

struct rockchip_connector {
	const struct rockchip_connector_funcs *funcs;

	const void *data;
};

struct rockchip_connector_funcs {
	/*
	 * pre init connector, prepare some parameter out_if, this will be
	 * used by rockchip_display.c and vop
	 */
	int (*pre_init)(struct display_state *state);

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
};

const struct rockchip_connector *
rockchip_get_connector(const void *blob, int connector_node);

#ifdef CONFIG_DRM_ROCKCHIP_ANALOGIX_DP
struct rockchip_dp_chip_data;
extern const struct rockchip_connector_funcs rockchip_analogix_dp_funcs;
extern const struct rockchip_dp_chip_data rk3399_analogix_edp_drv_data;
extern const struct rockchip_dp_chip_data rk3368_analogix_edp_drv_data;
extern const struct rockchip_dp_chip_data rk3288_analogix_dp_drv_data;
#endif
#endif
