/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_DISPLAY_H
#define _ROCKCHIP_DISPLAY_H

#include <bmp_layout.h>

/* Video mode flags */
/* bit compatible with the xorg definitions. */
#define DRM_MODE_FLAG_PHSYNC			(1 << 0)
#define DRM_MODE_FLAG_NHSYNC			(1 << 1)
#define DRM_MODE_FLAG_PVSYNC			(1 << 2)
#define DRM_MODE_FLAG_NVSYNC			(1 << 3)
#define DRM_MODE_FLAG_INTERLACE			(1 << 4)
#define DRM_MODE_FLAG_DBLSCAN			(1 << 5)
#define DRM_MODE_FLAG_CSYNC			(1 << 6)
#define DRM_MODE_FLAG_PCSYNC			(1 << 7)
#define DRM_MODE_FLAG_NCSYNC			(1 << 8)
#define DRM_MODE_FLAG_HSKEW			(1 << 9) /* hskew provided */
#define DRM_MODE_FLAG_BCAST			(1 << 10)
#define DRM_MODE_FLAG_PIXMUX			(1 << 11)
#define DRM_MODE_FLAG_DBLCLK			(1 << 12)
#define DRM_MODE_FLAG_CLKDIV2			(1 << 13)

#define DRM_MODE_CONNECTOR_Unknown	0
#define DRM_MODE_CONNECTOR_VGA		1
#define DRM_MODE_CONNECTOR_DVII		2
#define DRM_MODE_CONNECTOR_DVID		3
#define DRM_MODE_CONNECTOR_DVIA		4
#define DRM_MODE_CONNECTOR_Composite	5
#define DRM_MODE_CONNECTOR_SVIDEO	6
#define DRM_MODE_CONNECTOR_LVDS		7
#define DRM_MODE_CONNECTOR_Component	8
#define DRM_MODE_CONNECTOR_9PinDIN	9
#define DRM_MODE_CONNECTOR_DisplayPort	10
#define DRM_MODE_CONNECTOR_HDMIA	11
#define DRM_MODE_CONNECTOR_HDMIB	12
#define DRM_MODE_CONNECTOR_TV		13
#define DRM_MODE_CONNECTOR_eDP		14
#define DRM_MODE_CONNECTOR_VIRTUAL      15
#define DRM_MODE_CONNECTOR_DSI		16

struct drm_display_mode {
	/* Proposed mode values */
	int clock;		/* in kHz */
	int hdisplay;
	int hsync_start;
	int hsync_end;
	int htotal;
	int vdisplay;
	int vsync_start;
	int vsync_end;
	int vtotal;
	int vrefresh;
	int vscan;
	unsigned int flags;
};

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
	const struct rockchip_crtc *crtc;
	void *private;
	int node;
	int crtc_id;

	int format;
	u32 dma_addr;
	int ymirror;
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

struct panel {
	int node;

	int enable_flags;
};

struct connector_state {
	int node;
	const struct rockchip_connector *connector;

	void *private;

	struct fdt_gpio_state enable_gpio;
	struct drm_display_mode mode;
	int output_mode;
	int type;
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
	int node;
	const char *ulogo_name;
	const char *klogo_name;
	bmp_image_t *ubmp;
	bmp_image_t *kbmp;
	void *mem_base;
	int mem_size;
	struct logo_info logo;
	struct crtc_state crtc_state;
	struct connector_state conn_state;
	int enable;
	int is_init;
	int is_enable;
};

int drm_mode_vrefresh(const struct drm_display_mode *mode);

#endif
