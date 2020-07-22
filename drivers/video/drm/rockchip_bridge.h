/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2008-2018 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef _ROCKCHIP_BRIDGE_H_
#define _ROCKCHIP_BRIDGE_H_

#include <config.h>
#include <common.h>
#include <dm/device.h>
#include <errno.h>

struct display_state;
struct rockchip_bridge;
struct drm_display_mode;

struct rockchip_bridge_funcs {
	void (*enable)(struct rockchip_bridge *bridge);
	void (*disable)(struct rockchip_bridge *bridge);
	void (*pre_enable)(struct rockchip_bridge *bridge);
	void (*post_disable)(struct rockchip_bridge *bridge);
	void (*mode_set)(struct rockchip_bridge *bridge,
			 const struct drm_display_mode *mode);
};

struct rockchip_bridge {
	struct udevice *dev;
	const struct rockchip_bridge_funcs *funcs;

	struct display_state *state;
};

void rockchip_bridge_enable(struct rockchip_bridge *bridge);
void rockchip_bridge_disable(struct rockchip_bridge *bridge);
void rockchip_bridge_pre_enable(struct rockchip_bridge *bridge);
void rockchip_bridge_post_disable(struct rockchip_bridge *bridge);
void rockchip_bridge_mode_set(struct rockchip_bridge *bridge,
			      const struct drm_display_mode *mode);

#endif
