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

enum rockchip_cmd_type {
	CMD_TYPE_DEFAULT,
	CMD_TYPE_SPI,
	CMD_TYPE_MCU
};

enum rockchip_mcu_cmd {
	MCU_WRCMD = 0,
	MCU_WRDATA,
	MCU_SETBYPASS,
};

/*
 * display output interface supported by rockchip lcdc
 */
#define ROCKCHIP_OUT_MODE_P888	0
#define ROCKCHIP_OUT_MODE_P666	1
#define ROCKCHIP_OUT_MODE_P565	2
#define ROCKCHIP_OUT_MODE_S888		8
#define ROCKCHIP_OUT_MODE_S888_DUMMY	12
#define ROCKCHIP_OUT_MODE_YUV420	14
/* for use special outface */
#define ROCKCHIP_OUT_MODE_AAAA	15

struct rockchip_mcu_timing {
	int mcu_pix_total;
	int mcu_cs_pst;
	int mcu_cs_pend;
	int mcu_rw_pst;
	int mcu_rw_pend;
	int mcu_hold_mode;
};

struct crtc_state {
	struct udevice *dev;
	struct rockchip_crtc *crtc;
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
	bool yuv_overlay;
	struct rockchip_mcu_timing mcu_timing;
};

struct panel_state {
	struct rockchip_panel *panel;

	ofnode dsp_lut_node;
};

struct overscan {
	int left_margin;
	int right_margin;
	int top_margin;
	int bottom_margin;
};

struct connector_state {
	struct udevice *dev;
	const struct rockchip_connector *connector;
	struct rockchip_bridge *bridge;
	struct rockchip_phy *phy;
	ofnode node;

	void *private;

	struct drm_display_mode mode;
	struct overscan overscan;
	u8 edid[EDID_SIZE * 4];
	int bus_format;
	int output_mode;
	int type;
	int output_type;
	int color_space;

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
	int height;
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

	char ulogo_name[30];
	char klogo_name[30];

	struct logo_info logo;
	int logo_mode;
	int charge_logo_mode;
	void *mem_base;
	int mem_size;

	int enable;
	int is_init;
	int is_enable;
};

static inline struct rockchip_panel *state_get_panel(struct display_state *s)
{
	struct panel_state *panel_state = &s->panel_state;

	return panel_state->panel;
}

int drm_mode_vrefresh(const struct drm_display_mode *mode);
int display_send_mcu_cmd(struct display_state *state, u32 type, u32 val);
bool drm_mode_is_420(const struct drm_display_info *display,
		     struct drm_display_mode *mode);

#endif
