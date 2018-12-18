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

#define PX30_GRF_PD_VO_CON1		0x0438
#define PX30_RGB_DATA_SYNC_BYPASS(v)	HIWORD_UPDATE(v, 3, 3)
#define PX30_RGB_VOP_SEL(v)		HIWORD_UPDATE(v, 2, 2)

#define RK1808_GRF_PD_VO_CON1		0x0444
#define RK1808_RGB_DATA_SYNC_BYPASS(v)	HIWORD_UPDATE(v, 3, 3)

struct rockchip_rgb;

struct rockchip_rgb_funcs {
	void (*enable)(struct rockchip_rgb *rgb, int pipe);
	void (*disable)(struct rockchip_rgb *rgb);
};

struct rockchip_rgb {
	struct regmap *grf;
	bool data_sync;
	const struct rockchip_rgb_funcs *funcs;
};

static inline struct rockchip_rgb *state_to_rgb(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;

	return dev_get_priv(conn_state->dev);
}

static int rockchip_rgb_connector_enable(struct display_state *state)
{
	struct rockchip_rgb *rgb = state_to_rgb(state);
	struct crtc_state *crtc_state = &state->crtc_state;
	int pipe = crtc_state->crtc_id;

	if (rgb->funcs && rgb->funcs->enable)
		rgb->funcs->enable(rgb, pipe);

	return 0;
}

static int rockchip_rgb_connector_disable(struct display_state *state)
{
	struct rockchip_rgb *rgb = state_to_rgb(state);

	if (rgb->funcs && rgb->funcs->disable)
		rgb->funcs->disable(rgb);

	return 0;
}

static int rockchip_rgb_connector_init(struct display_state *state)
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

static const struct rockchip_connector_funcs rockchip_rgb_connector_funcs = {
	.init = rockchip_rgb_connector_init,
	.enable = rockchip_rgb_connector_enable,
	.disable = rockchip_rgb_connector_disable,
};

static int rockchip_rgb_probe(struct udevice *dev)
{
	struct rockchip_rgb *rgb = dev_get_priv(dev);
	const struct rockchip_connector *connector =
		(const struct rockchip_connector *)dev_get_driver_data(dev);

	rgb->funcs = connector->data;
	rgb->grf = syscon_get_regmap(dev_get_parent(dev));
	rgb->data_sync = dev_read_bool(dev, "rockchip,data-sync");

	return 0;
}

static void px30_rgb_enable(struct rockchip_rgb *rgb, int pipe)
{
	regmap_write(rgb->grf, PX30_GRF_PD_VO_CON1, PX30_RGB_VOP_SEL(pipe) |
		     PX30_RGB_DATA_SYNC_BYPASS(!rgb->data_sync));
}

static const struct rockchip_rgb_funcs px30_rgb_funcs = {
	.enable = px30_rgb_enable,
};

static const struct rockchip_connector px30_rgb_driver_data = {
	 .funcs = &rockchip_rgb_connector_funcs,
	 .data = &px30_rgb_funcs,
};

static void rk1808_rgb_enable(struct rockchip_rgb *rgb, int pipe)
{
	regmap_write(rgb->grf, RK1808_GRF_PD_VO_CON1,
		     RK1808_RGB_DATA_SYNC_BYPASS(!rgb->data_sync));
}

static const struct rockchip_rgb_funcs rk1808_rgb_funcs = {
	.enable = rk1808_rgb_enable,
};

static const struct rockchip_connector rk1808_rgb_driver_data = {
	.funcs = &rockchip_rgb_connector_funcs,
	.data = &rk1808_rgb_funcs,
};

static const struct rockchip_connector rockchip_rgb_driver_data = {
	.funcs = &rockchip_rgb_connector_funcs,
};

static const struct udevice_id rockchip_rgb_ids[] = {
	{
		.compatible = "rockchip,px30-rgb",
		.data = (ulong)&px30_rgb_driver_data,
	},
	{
		.compatible = "rockchip,rk1808-rgb",
		.data = (ulong)&rk1808_rgb_driver_data,
	},
	{
		.compatible = "rockchip,rk3066-rgb",
		.data = (ulong)&rockchip_rgb_driver_data,
	},
	{
		.compatible = "rockchip,rk3308-rgb",
		.data = (ulong)&rockchip_rgb_driver_data,
	},
	{
		.compatible = "rockchip,rv1108-rgb",
		.data = (ulong)&rockchip_rgb_driver_data,
	},
	{}
};

U_BOOT_DRIVER(rockchip_rgb) = {
	.name = "rockchip_rgb",
	.id = UCLASS_DISPLAY,
	.of_match = rockchip_rgb_ids,
	.probe = rockchip_rgb_probe,
	.priv_auto_alloc_size = sizeof(struct rockchip_rgb),
};
