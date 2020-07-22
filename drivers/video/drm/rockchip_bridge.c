// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2008-2018 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "rockchip_bridge.h"

void rockchip_bridge_pre_enable(struct rockchip_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->funcs && bridge->funcs->pre_enable)
		bridge->funcs->pre_enable(bridge);
}

void rockchip_bridge_post_disable(struct rockchip_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->funcs && bridge->funcs->post_disable)
		bridge->funcs->post_disable(bridge);
}

void rockchip_bridge_enable(struct rockchip_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->funcs && bridge->funcs->enable)
		bridge->funcs->enable(bridge);
}

void rockchip_bridge_disable(struct rockchip_bridge *bridge)
{
	if (!bridge)
		return;

	if (bridge->funcs && bridge->funcs->disable)
		bridge->funcs->disable(bridge);
}

void rockchip_bridge_mode_set(struct rockchip_bridge *bridge,
			      const struct drm_display_mode *mode)
{
	if (!bridge || !mode)
		return;

	if (bridge->funcs && bridge->funcs->mode_set)
		bridge->funcs->mode_set(bridge, mode);
}
