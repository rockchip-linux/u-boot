/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <syscon.h>
#include <regmap.h>
#include <dm/device.h>
#include <dm/read.h>
#include <linux/media-bus-format.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"

#define PX30_GRF_PD_VO_CON1	0x0438
#define PX30_LCDC_DCLK_INV(v)	HIWORD_UPDATE(v, 4, 4)
#define PX30_RGB_SYNC_BYPASS(v)	HIWORD_UPDATE(v, 3, 3)
#define PX30_RGB_VOP_SEL(v)	HIWORD_UPDATE(v, 2, 2)

struct rockchip_rgb_priv {
	struct regmap *grf;
};

static int rockchip_rgb_enable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct rockchip_rgb_priv *priv = dev_get_priv(conn_state->dev);
	struct crtc_state *crtc_state = &state->crtc_state;
	int pipe = crtc_state->crtc_id;

	if (!IS_ERR_OR_NULL(priv->grf)) {
		regmap_write(priv->grf, PX30_GRF_PD_VO_CON1,
			     PX30_RGB_VOP_SEL(pipe));
		regmap_write(priv->grf, PX30_GRF_PD_VO_CON1,
			     PX30_RGB_SYNC_BYPASS(1));
	}

	return 0;
}

static int rockchip_rgb_disable(struct display_state *state)
{
	return 0;
}

static int rockchip_rgb_init(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;

	conn_state->type = DRM_MODE_CONNECTOR_LVDS;
	conn_state->color_space = V4L2_COLORSPACE_DEFAULT;

	switch (conn_state->bus_format) {
	case MEDIA_BUS_FMT_RGB666_1X18:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P666;
		break;
	case MEDIA_BUS_FMT_RGB565_1X16:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P565;
		break;
	case MEDIA_BUS_FMT_RGB888_1X24:
	case MEDIA_BUS_FMT_RGB666_1X24_CPADHI:
	default:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P888;
		break;
	}

	return 0;
}

static const struct rockchip_connector_funcs rockchip_rgb_funcs = {
	.init = rockchip_rgb_init,
	.enable = rockchip_rgb_enable,
	.disable = rockchip_rgb_disable,
};

static int rockchip_rgb_probe(struct udevice *dev)
{
	struct rockchip_rgb_priv *priv = dev_get_priv(dev);

	priv->grf = syscon_get_regmap(dev_get_parent(dev));

	return 0;
}

static const struct rockchip_connector rockchip_rgb_data = {
	 .funcs = &rockchip_rgb_funcs,
};

static const struct udevice_id rockchip_rgb_ids[] = {
	{
		.compatible = "rockchip,px30-rgb",
		.data = (ulong)&rockchip_rgb_data,
	},
	{
		.compatible = "rockchip,rk3066-rgb",
		.data = (ulong)&rockchip_rgb_data,
	},
	{
		.compatible = "rockchip,rk3308-rgb",
		.data = (ulong)&rockchip_rgb_data,
	},
	{
		.compatible = "rockchip,rv1108-rgb",
		.data = (ulong)&rockchip_rgb_data,
	},
	{}
};

U_BOOT_DRIVER(rockchip_rgb) = {
	.name = "rockchip_rgb",
	.id = UCLASS_DISPLAY,
	.of_match = rockchip_rgb_ids,
	.probe = rockchip_rgb_probe,
	.priv_auto_alloc_size = sizeof(struct rockchip_rgb_priv),
};
