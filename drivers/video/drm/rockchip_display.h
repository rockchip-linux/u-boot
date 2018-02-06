/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_DISPLAY_H
#define _ROCKCHIP_DISPLAY_H

#include <bmp_layout.h>
#include <drm_modes.h>
#include <edid.h>
#include <dm/ofnode.h>

#define ROCKCHIP_OUTPUT_DSI_DUAL_CHANNEL	BIT(0)
#define ROCKCHIP_OUTPUT_DSI_DUAL_LINK		BIT(1)

enum data_format {
	ROCKCHIP_FMT_ARGB8888 = 0,
	ROCKCHIP_FMT_RGB888,
	ROCKCHIP_FMT_RGB565,
	ROCKCHIP_FMT_YUV420SP = 4,
	ROCKCHIP_FMT_YUV422SP,
	ROCKCHIP_FMT_YUV444SP,
};

enum display_mode {
	ROCKCHIP_DISPLAY_FULLSCREEN,
	ROCKCHIP_DISPLAY_CENTER,
};

/*
 * display output interface supported by rockchip lcdc
 */
#define ROCKCHIP_OUT_MODE_P888	0
#define ROCKCHIP_OUT_MODE_P666	1
#define ROCKCHIP_OUT_MODE_P565	2
/* for use special outface */
#define ROCKCHIP_OUT_MODE_AAAA	15

struct crtc_state {
	struct udevice *dev;
	const struct rockchip_crtc *crtc;
	void *private;
	ofnode node;
	int crtc_id;

	int format;
	u32 dma_addr;
	int ymirror;
	int rb_swap;
	int xvir;
	int src_x;
	int src_y;
	int src_w;
	int src_h;
	int crtc_x;
	int crtc_y;
	int crtc_w;
	int crtc_h;
};

struct panel_state {
	struct udevice *dev;
	ofnode node;
	ofnode dsp_lut_node;

	const struct rockchip_panel *panel;
	void *private;
};

struct connector_state {
	struct udevice *dev;
	const struct rockchip_connector *connector;
	struct udevice *phy_dev;
	const struct rockchip_phy *phy;
	ofnode node;
	ofnode phy_node;

	void *private;
	void *phy_private;

	struct drm_display_mode mode;
	u8 edid[EDID_SIZE * 4];
	int bus_format;
	int output_mode;
	int type;
	int output_type;

	struct {
		u32 *lut;
		int size;
	} gamma;
};

struct logo_info {
	int mode;
	char *mem;
	bool ymirror;
	u32 offset;
	u32 width;
	u32 height;
	u32 bpp;
};

struct rockchip_logo_cache {
	struct list_head head;
	char name[20];
	struct logo_info logo;
};

struct display_state {
	struct list_head head;

	const void *blob;
	ofnode node;

	struct crtc_state crtc_state;
	struct connector_state conn_state;
	struct panel_state panel_state;

	const char *ulogo_name;
	const char *klogo_name;

	struct logo_info logo;
	int logo_mode;
	int charge_logo_mode;
	void *mem_base;
	int mem_size;

	int enable;
	int is_init;
	int is_enable;
};

int drm_mode_vrefresh(const struct drm_display_mode *mode);

#endif
