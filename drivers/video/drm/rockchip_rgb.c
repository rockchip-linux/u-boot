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
#include <dm/pinctrl.h>
#include <linux/media-bus-format.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_phy.h"

#define HIWORD_UPDATE(v, h, l)		(((v) << (l)) | (GENMASK(h, l) << 16))

#define PX30_GRF_PD_VO_CON1		0x0438
#define PX30_RGB_DATA_SYNC_BYPASS(v)	HIWORD_UPDATE(v, 3, 3)
#define PX30_RGB_VOP_SEL(v)		HIWORD_UPDATE(v, 2, 2)

#define RK1808_GRF_PD_VO_CON1		0x0444
#define RK1808_RGB_DATA_SYNC_BYPASS(v)	HIWORD_UPDATE(v, 3, 3)

#define RK3288_GRF_SOC_CON6		0x025c
#define RK3288_LVDS_LCDC_SEL(v)		HIWORD_UPDATE(v,  3,  3)
#define RK3288_GRF_SOC_CON7		0x0260
#define RK3288_LVDS_PWRDWN(v)		HIWORD_UPDATE(v, 15, 15)
#define RK3288_LVDS_CON_ENABLE_2(v)	HIWORD_UPDATE(v, 12, 12)
#define RK3288_LVDS_CON_ENABLE_1(v)	HIWORD_UPDATE(v, 11, 11)
#define RK3288_LVDS_CON_CLKINV(v)	HIWORD_UPDATE(v,  8,  8)
#define RK3288_LVDS_CON_TTL_EN(v)	HIWORD_UPDATE(v,  6,  6)

#define RK3368_GRF_SOC_CON15		0x043c
#define RK3368_FORCE_JETAG(v)		HIWORD_UPDATE(v,  13,  13)

struct rockchip_rgb;

struct rockchip_rgb_funcs {
	void (*enable)(struct rockchip_rgb *rgb, int pipe);
	void (*disable)(struct rockchip_rgb *rgb);
};

struct rockchip_rgb {
	struct udevice *dev;
	struct regmap *grf;
	bool data_sync;
	struct rockchip_phy *phy;
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
	int ret;

	pinctrl_select_state(rgb->dev, "default");

	if (rgb->funcs && rgb->funcs->enable)
		rgb->funcs->enable(rgb, pipe);

	if (rgb->phy) {
		ret = rockchip_phy_set_mode(rgb->phy, PHY_MODE_VIDEO_TTL);
		if (ret) {
			dev_err(rgb->dev, "failed to set phy mode: %d\n", ret);
			return ret;
		}

		rockchip_phy_power_on(rgb->phy);
	}

	return 0;
}

static int rockchip_rgb_connector_disable(struct display_state *state)
{
	struct rockchip_rgb *rgb = state_to_rgb(state);

	if (rgb->phy)
		rockchip_phy_power_off(rgb->phy);

	if (rgb->funcs && rgb->funcs->disable)
		rgb->funcs->disable(rgb);

	pinctrl_select_state(rgb->dev, "sleep");

	return 0;
}

static int rockchip_rgb_connector_init(struct display_state *state)
{
	struct rockchip_rgb *rgb = state_to_rgb(state);
	struct connector_state *conn_state = &state->conn_state;

	rgb->phy = conn_state->phy;

	conn_state->type = DRM_MODE_CONNECTOR_LVDS;
	conn_state->color_space = V4L2_COLORSPACE_DEFAULT;

	switch (conn_state->bus_format) {
	case MEDIA_BUS_FMT_RGB666_1X18:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P666;
		break;
	case MEDIA_BUS_FMT_RGB565_1X16:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P565;
		break;
	case MEDIA_BUS_FMT_SRGB888_3X8:
	case MEDIA_BUS_FMT_SBGR888_3X8:
	case MEDIA_BUS_FMT_SRBG888_3X8:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_S888;
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

	rgb->dev = dev;
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

static void rk3288_rgb_enable(struct rockchip_rgb *rgb, int pipe)
{
	regmap_write(rgb->grf, RK3288_GRF_SOC_CON6, RK3288_LVDS_LCDC_SEL(pipe));
	regmap_write(rgb->grf, RK3288_GRF_SOC_CON7,
		     RK3288_LVDS_PWRDWN(0) | RK3288_LVDS_CON_ENABLE_2(1) |
		     RK3288_LVDS_CON_ENABLE_1(1) | RK3288_LVDS_CON_CLKINV(0) |
		     RK3288_LVDS_CON_TTL_EN(1));
}

static void rk3288_rgb_disable(struct rockchip_rgb *rgb)
{
	regmap_write(rgb->grf, RK3288_GRF_SOC_CON7,
		     RK3288_LVDS_PWRDWN(1) | RK3288_LVDS_CON_ENABLE_2(0) |
		     RK3288_LVDS_CON_ENABLE_1(0) | RK3288_LVDS_CON_TTL_EN(0));
}

static const struct rockchip_rgb_funcs rk3288_rgb_funcs = {
	.enable = rk3288_rgb_enable,
	.disable = rk3288_rgb_disable,
};

static const struct rockchip_connector rk3288_rgb_driver_data = {
	.funcs = &rockchip_rgb_connector_funcs,
	.data = &rk3288_rgb_funcs,
};

static void rk3368_rgb_enable(struct rockchip_rgb *rgb, int pipe)
{
	regmap_write(rgb->grf, RK3368_GRF_SOC_CON15, RK3368_FORCE_JETAG(0));
}

static const struct rockchip_rgb_funcs rk3368_rgb_funcs = {
	.enable = rk3368_rgb_enable,
};

static const struct rockchip_connector rk3368_rgb_driver_data = {
	.funcs = &rockchip_rgb_connector_funcs,
	.data = &rk3368_rgb_funcs,
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
		.compatible = "rockchip,rk3128-rgb",
		.data = (ulong)&rockchip_rgb_driver_data,
	},
	{
		.compatible = "rockchip,rk3288-rgb",
		.data = (ulong)&rk3288_rgb_driver_data,
	},
	{
		.compatible = "rockchip,rk3308-rgb",
		.data = (ulong)&rockchip_rgb_driver_data,
	},
	{
		.compatible = "rockchip,rk3368-rgb",
		.data = (ulong)&rk3368_rgb_driver_data,
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
