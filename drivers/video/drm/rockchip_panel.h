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
	void (*unprepare)(struct display_state *state);
	int (*enable)(struct display_state *state);
	void (*disable)(struct display_state *state);
};

struct rockchip_panel {
	const struct rockchip_panel_funcs *funcs;
	const void *data;
};

#endif	/* _ROCKCHIP_PANEL_H_ */
