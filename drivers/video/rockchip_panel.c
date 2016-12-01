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

struct rockchip_panel {
	char compatible[30];

	const struct drm_display_mode *mode;
};

static const struct drm_display_mode lg_lp079qx1_sp0v_mode = {
	.clock = 200000,
	.hdisplay = 1536,
	.hsync_start = 1536 + 12,
	.hsync_end = 1536 + 12 + 16,
	.htotal = 1536 + 12 + 16 + 48,
	.vdisplay = 2048,
	.vsync_start = 2048 + 8,
	.vsync_end = 2048 + 8 + 4,
	.vtotal = 2048 + 8 + 4 + 8,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct rockchip_panel g_panel[] = {
	{
	 .compatible = "lg,lp079qx1-sp0v",
	 .mode = &lg_lp079qx1_sp0v_mode,
	},
};

const struct drm_display_mode *
rockchip_get_display_mode_from_panel(const void *blob, int panel_node)
{
	const char *name;
	int i;

	fdt_get_string(blob, panel_node, "compatible", &name);

	for (i = 0; i < ARRAY_SIZE(g_panel); i++) {
		if (!strcmp(name, g_panel[i].compatible))
			break;
	}
	if (i >= ARRAY_SIZE(g_panel))
		return NULL;

	return g_panel[i].mode;
}
