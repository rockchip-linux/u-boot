/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_CRTC_H_
#define _ROCKCHIP_CRTC_H_

struct rockchip_crtc {
	char compatible[30];
	const struct rockchip_crtc_funcs *funcs;
	const void *data;
};

struct rockchip_crtc_funcs {
	int (*init)(struct display_state *state);
	void (*deinit)(struct display_state *state);
	int (*set_plane)(struct display_state *state);
	int (*prepare)(struct display_state *state);
	int (*enable)(struct display_state *state);
	int (*disable)(struct display_state *state);
	void (*unprepare)(struct display_state *state);
	int (*fixup_dts)(struct display_state *state, void *blob);
};

const struct rockchip_crtc *
rockchip_get_crtc(const void *blob, int crtc_node);

#ifdef CONFIG_ROCKCHIP_VOP
struct vop_data;
extern const struct rockchip_crtc_funcs rockchip_vop_funcs;
extern const struct vop_data rk3399_vop;
#endif
#endif
