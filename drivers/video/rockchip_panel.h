/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_PANEL_H_
#define _ROCKCHIP_PANEL_H_

struct rockchip_panel_funcs {
	int (*init)(struct display_state *state);
	void (*deinit)(struct display_state *state);
	int (*prepare)(struct display_state *state);
	int (*unprepare)(struct display_state *state);
	int (*enable)(struct display_state *state);
	int (*disable)(struct display_state *state);
};

struct rockchip_panel {
	char compatible[30];
	const struct rockchip_panel_funcs *funcs;
	const void *data;
};

const struct rockchip_panel *rockchip_get_panel(const void *blob, int node);
const struct drm_display_mode *
rockchip_get_display_mode_from_panel(struct display_state *state);
int rockchip_panel_init(struct display_state *state);
void rockchip_panel_deinit(struct display_state *state);
int rockchip_panel_enable(struct display_state *state);
int rockchip_panel_disable(struct display_state *state);
int rockchip_panel_prepare(struct display_state *state);
int rockchip_panel_unprepare(struct display_state *state);

#ifdef CONFIG_ROCKCHIP_PANEL
extern const struct rockchip_panel_funcs panel_simple_funcs;
extern const struct rockchip_panel_funcs rockchip_dsi_panel_funcs;
#endif

#endif	/* _ROCKCHIP_PANEL_H_ */
