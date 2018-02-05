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
#include "rockchip_panel.h"

static const struct drm_display_mode auo_b125han03_mode = {
	.clock = 146900,
	.hdisplay = 1920,
	.hsync_start = 1920 + 48,
	.hsync_end = 1920 + 48 + 32,
	.htotal = 1920 + 48 + 32 + 140,
	.vdisplay = 1080,
	.vsync_start = 1080 + 2,
	.vsync_end = 1080 + 2 + 5,
	.vtotal = 1080 + 2 + 5 + 57,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct drm_display_mode boe_mv238qum_n20_mode = {
	.clock = 559440,
	.hdisplay = 3840,
	.hsync_start = 3840 + 150,
	.hsync_end = 3840 + 150 + 60,
	.htotal = 3840 + 150 + 60 + 150,
	.vdisplay = 2160,
	.vsync_start = 2160 + 24,
	.vsync_end = 2160 + 24 + 12,
	.vtotal = 2160 + 24 + 12 + 24,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct drm_display_mode lg_lm238wr2_spa1_mode = {
	.clock = 533250,
	.hdisplay = 3840,
	.hsync_start = 3840 + 48,
	.hsync_end = 3840 + 48 + 32,
	.htotal = 3840 + 48 + 32 + 80,
	.vdisplay = 2160,
	.vsync_start = 2160 + 3,
	.vsync_end = 2160 + 3 + 5,
	.vtotal = 2160 + 3 + 5 + 54,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct drm_display_mode lg_lm270wr3_ssa1_mode = {
	.clock = 533250,
	.hdisplay = 3840,
	.hsync_start = 3840 + 48,
	.hsync_end = 3840 + 48 + 32,
	.htotal = 3840 + 48 + 32 + 80,
	.vdisplay = 2160,
	.vsync_start = 2160 + 3,
	.vsync_end = 2160 + 3 + 5,
	.vtotal = 2160 + 3 + 5 + 54,
	.vrefresh = 60,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
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
		.compatible = "simple-panel",
		.funcs = &panel_simple_funcs,
	}, {
		.compatible = "simple-panel-dsi",
		.funcs = &rockchip_dsi_panel_funcs,
	}, {
		.compatible = "auo,b125han03",
		.funcs = &panel_simple_funcs,
		.data = &auo_b125han03_mode,
	}, {
		.compatible = "boe,mv238qum-n20",
		.funcs = &panel_simple_funcs,
		.data = &boe_mv238qum_n20_mode,
	}, {
		.compatible = "lg,lm238wr2-spa1",
		.funcs = &panel_simple_funcs,
		.data = &lg_lm238wr2_spa1_mode,
	}, {
		.compatible = "lg,lm270wr3-ssa1",
		.funcs = &panel_simple_funcs,
		.data = &lg_lm270wr3_ssa1_mode,
	}, {
		.compatible = "lg,lp079qx1-sp0v",
		.funcs = &panel_simple_funcs,
		.data = &lg_lp079qx1_sp0v_mode,
	}, {
		.compatible = "rockchip,rk1000-tve",
		.funcs = &rockchip_rk1000_funcs,
	},
};

const struct drm_display_mode *
rockchip_get_display_mode_from_panel(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	const struct rockchip_panel *panel = panel_state->panel;

	if (!panel || !panel->data)
		return NULL;

	return (const struct drm_display_mode *)panel->data;
}

const struct rockchip_panel *rockchip_get_panel(const void *blob, int node)
{
	const char *name;
	int i, ret, index = 0;

	for (;;) {
		ret = fdt_get_string_index(blob, node, "compatible", index++, &name);
		if (ret < 0)
			break;

		for (i = 0; i < ARRAY_SIZE(g_panel); i++)
			if (!strcmp(name, g_panel[i].compatible))
				return &g_panel[i];
	}

	return NULL;
}

int rockchip_panel_init(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	const struct rockchip_panel *panel = panel_state->panel;

	if (!panel || !panel->funcs || !panel->funcs->init) {
		printf("%s: failed to find panel init funcs\n", __func__);
		return -ENODEV;
	}

	return panel->funcs->init(state);
}

void rockchip_panel_deinit(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	const struct rockchip_panel *panel = panel_state->panel;

	if (!panel || !panel->funcs || !panel->funcs->deinit) {
		printf("%s: failed to find panel deinit funcs\n", __func__);
		return;
	}

	panel->funcs->deinit(state);
}

int rockchip_panel_prepare(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	const struct rockchip_panel *panel = panel_state->panel;

	if (!panel || !panel->funcs || !panel->funcs->prepare) {
		printf("%s: failed to find panel prepare funcs\n", __func__);
		return -ENODEV;
	}

	return panel->funcs->prepare(state);
}

int rockchip_panel_unprepare(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	const struct rockchip_panel *panel = panel_state->panel;

	if (!panel || !panel->funcs || !panel->funcs->unprepare) {
		printf("%s: failed to find panel unprepare funcs\n", __func__);
		return -ENODEV;
	}

	return panel->funcs->unprepare(state);
}

int rockchip_panel_enable(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	const struct rockchip_panel *panel = panel_state->panel;

	if (!panel || !panel->funcs || !panel->funcs->enable) {
		printf("%s: failed to find panel prepare funcs\n", __func__);
		return -ENODEV;
	}

	return panel->funcs->enable(state);
}

int rockchip_panel_disable(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	const struct rockchip_panel *panel = panel_state->panel;

	if (!panel || !panel->funcs || !panel->funcs->disable) {
		printf("%s: failed to find panel disable funcs\n", __func__);
		return -ENODEV;
	}

	return panel->funcs->disable(state);
}
