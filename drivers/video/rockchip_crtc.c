/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <resource.h>
#include <asm/arch/rkplat.h>
#include <asm/unaligned.h>
#include <linux/list.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"

static const struct rockchip_crtc g_crtc[] = {
#ifdef CONFIG_ROCKCHIP_VOP
	{
		.compatible = "rockchip,rk3036-vop",
		.funcs = &rockchip_vop_funcs,
		.data = &rk3036_vop,
	}, {
		.compatible = "rockchip,rk3288-vop",
		.funcs = &rockchip_vop_funcs,
		.data = &rk3288_vop,
	}, {
		.compatible = "rockchip,rk3368-vop",
		.funcs = &rockchip_vop_funcs,
		.data = &rk3368_vop,
	}, {
		.compatible = "rockchip,rk3366-vop",
		.funcs = &rockchip_vop_funcs,
		.data = &rk3366_vop,
	}, {
		.compatible = "rockchip,rk3399-vop-big",
		.funcs = &rockchip_vop_funcs,
		.data = &rk3399_vop_big,
	}, {
		.compatible = "rockchip,rk3399-vop-lit",
		.funcs = &rockchip_vop_funcs,
		.data = &rk3399_vop_lit,
	}, {
		.compatible = "rockchip,rk322x-vop",
		.funcs = &rockchip_vop_funcs,
		.data = &rk322x_vop,
	}, {
		.compatible = "rockchip,rk3328-vop",
		.funcs = &rockchip_vop_funcs,
		.data = &rk3328_vop,
	},
#endif
};

const struct rockchip_crtc *
rockchip_get_crtc(const void *blob, int crtc_node)
{
	const char *name;
	int i;

	fdt_get_string(blob, crtc_node, "compatible", &name);

	for (i = 0; i < ARRAY_SIZE(g_crtc); i++) {
		if (!strcmp(name, g_crtc[i].compatible))
			break;
	}
	if (i >= ARRAY_SIZE(g_crtc))
		return NULL;

	return &g_crtc[i];
}
