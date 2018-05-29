/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <asm/unaligned.h>
#include <linux/list.h>
#include <dm/device.h>
#include <dm.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"

static const struct rockchip_crtc rk3036_vop_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3036_vop,
};

static const struct rockchip_crtc rv1108_vop_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rv1108_vop,
};

static const struct rockchip_crtc px30_vop_lit_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &px30_vop_lit,
};

static const struct rockchip_crtc px30_vop_big_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &px30_vop_big,
};

static const struct rockchip_crtc rk3308_vop_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3308_vop,
};

static const struct rockchip_crtc rk3288_vop_big_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3288_vop_big,
};

static const struct rockchip_crtc rk3288_vop_lit_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3288_vop_lit,
};

static const struct rockchip_crtc rk3368_vop_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3368_vop,
};

static const struct rockchip_crtc rk3366_vop_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3366_vop,
};

static const struct rockchip_crtc rk3399_vop_big_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3399_vop_big,
};

static const struct rockchip_crtc rk3399_vop_lit_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3399_vop_lit,
};

static const struct rockchip_crtc rk322x_vop_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk322x_vop,
};

static const struct rockchip_crtc rk3328_vop_data = {
	.funcs = &rockchip_vop_funcs,
	.data = &rk3328_vop,
};

static const struct udevice_id rockchip_vop_ids[] = {
	{
		.compatible = "rockchip,rk3036-vop",
		.data = (ulong)&rk3036_vop_data,
	}, {
		.compatible = "rockchip,rv1108-vop",
		.data = (ulong)&rv1108_vop_data,
	}, {
		.compatible = "rockchip,rk3126-vop",
		.data = (ulong)&rk3036_vop_data,
	}, {
		.compatible = "rockchip,px30-vop-lit",
		.data = (ulong)&px30_vop_lit_data,
	}, {
		.compatible = "rockchip,px30-vop-big",
		.data = (ulong)&px30_vop_big_data,
	}, {
		.compatible = "rockchip,rk3308-vop",
		.data = (ulong)&rk3308_vop_data,
	}, {
		.compatible = "rockchip,rk3288-vop-big",
		.data = (ulong)&rk3288_vop_big_data,
	}, {
		.compatible = "rockchip,rk3288-vop-lit",
		.data = (ulong)&rk3288_vop_lit_data,
	}, {
		.compatible = "rockchip,rk3368-vop",
		.data = (ulong)&rk3368_vop_data,
	}, {
		.compatible = "rockchip,rk3366-vop",
		.data = (ulong)&rk3366_vop_data,
	}, {
		.compatible = "rockchip,rk3399-vop-big",
		.data = (ulong)&rk3399_vop_big_data,
	}, {
		.compatible = "rockchip,rk3399-vop-lit",
		.data = (ulong)&rk3399_vop_lit_data,
	}, {
		.compatible = "rockchip,rk322x-vop",
		.data = (ulong)&rk322x_vop_data,
	}, {
		.compatible = "rockchip,rk3328-vop",
		.data = (ulong)&rk3328_vop_data,
	}, { }
};

static int rockchip_vop_probe(struct udevice *dev)
{
	return 0;
}

static int rockchip_vop_bind(struct udevice *dev)
{
	return 0;
}

U_BOOT_DRIVER(rockchip_vop) = {
	.name	= "rockchip-vop",
	.id	= UCLASS_VIDEO_CRTC,
	.of_match = rockchip_vop_ids,
	.bind	= rockchip_vop_bind,
	.probe	= rockchip_vop_probe,
};

UCLASS_DRIVER(rockchip_crtc) = {
	.id		= UCLASS_VIDEO_CRTC,
	.name		= "CRTC",
};
