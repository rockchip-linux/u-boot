/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/rkplat.h>
#include <asm/unaligned.h>
#include <asm/errno.h>
#include <config.h>
#include <common.h>
#include <errno.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <linux/list.h>
#include <linux/compat.h>
#include <linux/media-bus-format.h>
#include <malloc.h>
#include <resource.h>

#include "bmp_helper.h"
#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_phy.h"
#include "rockchip_panel.h"

DECLARE_GLOBAL_DATA_PTR;
static LIST_HEAD(rockchip_display_list);
static LIST_HEAD(logo_cache_list);

#define MEMORY_POOL_SIZE CONFIG_RK_LCD_SIZE
static unsigned long memory_start;
static unsigned long memory_end;

#ifdef CONFIG_RK_PWM_BL
extern int rk_pwm_bl_config(int brightness);
#endif

static void init_display_buffer(void)
{
	memory_start = gd->fb_base;
	memory_end = memory_start;
}

static void *get_display_buffer(int size)
{
	unsigned long roundup_memory = roundup(memory_end, PAGE_SIZE);
	void *buf;

	if (roundup_memory + size > memory_start + MEMORY_POOL_SIZE) {
		printf("failed to alloc %dbyte memory to display\n", size);
		return NULL;
	}
	buf = (void *)roundup_memory;

	memory_end = roundup_memory + size;

	return buf;
}

static unsigned long get_display_size(void)
{
	return memory_end - memory_start;
}

static bool can_direct_logo(int bpp)
{
	return bpp == 24 || bpp == 32;
}

static int get_panel_node(struct display_state *state, int conn_node)
{
	const void *blob = state->blob;
	int panel, ports, port, ep, remote, ph, nodedepth;

	panel = fdt_subnode_offset(blob, conn_node, "panel");
	if (panel > 0)
		return panel;

	ports = fdt_subnode_offset(blob, conn_node, "ports");
	if (ports < 0)
		return -ENODEV;

	fdt_for_each_subnode(blob, port, ports) {
		fdt_for_each_subnode(blob, ep, port) {
			ph = fdt_getprop_u32_default_node(blob, ep, 0,
							  "remote-endpoint", 0);
			if (!ph)
				continue;

			remote = fdt_node_offset_by_phandle(blob, ph);

			nodedepth = fdt_node_depth(blob, remote);
			if (nodedepth < 2)
				continue;

			panel = fdt_supernode_atdepth_offset(blob, remote,
							     nodedepth - 2,
							     NULL);
			break;
		}
	}

	return panel;
}

static int connector_phy_init(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	int conn_node = conn_state->node;
	const void *blob = state->blob;
	const struct rockchip_phy *phy;
	int phy_node, phandle;

	phandle = fdt_getprop_u32_default_node(blob, conn_node, 0,
					       "phys", -1);
	if (phandle < 0)
		return 0;

	phy_node = fdt_node_offset_by_phandle(blob, phandle);
	if (phy_node < 0) {
		printf("failed to find phy node\n");
		return phy_node;
	}

	phy = rockchip_get_phy(blob, phy_node);
	if (!phy) {
		printf("failed to find phy driver\n");
		return 0;
	}

	conn_state->phy_node = phy_node;

	if (!phy->funcs || !phy->funcs->init ||
	    phy->funcs->init(state)) {
		printf("failed to init phy driver\n");
		return -EINVAL;
	}

	conn_state->phy = phy;
	return 0;
}

static int connector_panel_init(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct panel_state *panel_state = &state->panel_state;
	const void *blob = state->blob;
	int conn_node = conn_state->node;
	const struct rockchip_panel *panel;
	int panel_node, dsp_lut_node;
	int ret, len;

	panel_node = get_panel_node(state, conn_node);
	if (panel_node < 0) {
		printf("failed to find panel node\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, panel_node)) {
		printf("panel is disabled\n");
		return -ENODEV;
	}

	panel_state->node = panel_node;

	panel = rockchip_get_panel(blob, panel_node);
	if (!panel) {
		printf("failed to find panel driver\n");
		return 0;
	}

	panel_state->panel = panel;

	ret = rockchip_panel_init(state);
	if (ret) {
		printf("failed to init panel driver\n");
		return ret;
	}

	dsp_lut_node = fdt_subnode_offset(blob, panel_node, "dsp-lut");
	fdt_getprop(blob, dsp_lut_node, "gamma-lut", &len);
	if (len > 0) {
		conn_state->gamma.size  = len / sizeof(u32);
		conn_state->gamma.lut = malloc(len);
		if (!conn_state->gamma.lut) {
			printf("malloc gamma lut failed\n");
			return -ENOMEM;
		}
		if (fdtdec_get_int_array(blob, dsp_lut_node, "gamma-lut",
					 conn_state->gamma.lut,
					 conn_state->gamma.size)) {
			printf("Cannot decode gamma_lut\n");
			conn_state->gamma.lut = NULL;
			return -EINVAL;
		}
		panel_state->dsp_lut_node = dsp_lut_node;
	}

	return 0;
}

int drm_mode_vrefresh(const struct drm_display_mode *mode)
{
	int refresh = 0;
	unsigned int calc_val;

	if (mode->vrefresh > 0) {
		refresh = mode->vrefresh;
	} else if (mode->htotal > 0 && mode->vtotal > 0) {
		int vtotal;

		vtotal = mode->vtotal;
		/* work out vrefresh the value will be x1000 */
		calc_val = (mode->clock * 1000);
		calc_val /= mode->htotal;
		refresh = (calc_val + vtotal / 2) / vtotal;

		if (mode->flags & DRM_MODE_FLAG_INTERLACE)
			refresh *= 2;
		if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
			refresh /= 2;
		if (mode->vscan > 1)
			refresh /= mode->vscan;
	}
	return refresh;
}

static int display_get_timing_from_dts(int panel, const void *blob,
				       struct drm_display_mode *mode)
{
	int timing, phandle, native_mode;
	int hactive, vactive, pixelclock;
	int hfront_porch, hback_porch, hsync_len;
	int vfront_porch, vback_porch, vsync_len;
	int val, flags = 0;

	timing = fdt_subnode_offset(blob, panel, "display-timings");
	if (timing < 0)
		return -ENODEV;

	native_mode = fdt_subnode_offset(blob, timing, "timing");
	if (native_mode < 0) {
		phandle = fdt_getprop_u32_default_node(blob, timing, 0,
						       "native-mode", -1);
		native_mode = fdt_node_offset_by_phandle_node(blob, timing, phandle);
		if (native_mode <= 0) {
			printf("failed to get display timings from DT\n");
			return -ENXIO;
		}
	}

#define FDT_GET_INT(val, name) \
	val = fdtdec_get_int(blob, native_mode, name, -1); \
	if (val < 0) { \
		printf("Can't get %s\n", name); \
		return -ENXIO; \
	}

	FDT_GET_INT(hactive, "hactive");
	FDT_GET_INT(vactive, "vactive");
	FDT_GET_INT(pixelclock, "clock-frequency");
	FDT_GET_INT(hsync_len, "hsync-len");
	FDT_GET_INT(hfront_porch, "hfront-porch");
	FDT_GET_INT(hback_porch, "hback-porch");
	FDT_GET_INT(vsync_len, "vsync-len");
	FDT_GET_INT(vfront_porch, "vfront-porch");
	FDT_GET_INT(vback_porch, "vback-porch");
	FDT_GET_INT(val, "hsync-active");
	flags |= val ? DRM_MODE_FLAG_PHSYNC : DRM_MODE_FLAG_NHSYNC;
	FDT_GET_INT(val, "vsync-active");
	flags |= val ? DRM_MODE_FLAG_PVSYNC : DRM_MODE_FLAG_NVSYNC;

	mode->hdisplay = hactive;
	mode->hsync_start = mode->hdisplay + hfront_porch;
	mode->hsync_end = mode->hsync_start + hsync_len;
	mode->htotal = mode->hsync_end + hback_porch;

	mode->vdisplay = vactive;
	mode->vsync_start = mode->vdisplay + vfront_porch;
	mode->vsync_end = mode->vsync_start + vsync_len;
	mode->vtotal = mode->vsync_end + vback_porch;

	mode->clock = pixelclock / 1000;
	mode->flags = flags;

	return 0;
}

/**
 * drm_mode_set_crtcinfo - set CRTC modesetting timing parameters
 * @p: mode
 * @adjust_flags: a combination of adjustment flags
 *
 * Setup the CRTC modesetting timing parameters for @p, adjusting if necessary.
 *
 * - The CRTC_INTERLACE_HALVE_V flag can be used to halve vertical timings of
 *   interlaced modes.
 * - The CRTC_STEREO_DOUBLE flag can be used to compute the timings for
 *   buffers containing two eyes (only adjust the timings when needed, eg. for
 *   "frame packing" or "side by side full").
 * - The CRTC_NO_DBLSCAN and CRTC_NO_VSCAN flags request that adjustment *not*
 *   be performed for doublescan and vscan > 1 modes respectively.
 */
void drm_mode_set_crtcinfo(struct drm_display_mode *p, int adjust_flags)
{
	if ((p == NULL) || ((p->type & DRM_MODE_TYPE_CRTC_C) == DRM_MODE_TYPE_BUILTIN))
		return;

	if (p->flags & DRM_MODE_FLAG_DBLCLK)
		p->crtc_clock = 2 * p->clock;
	else
		p->crtc_clock = p->clock;
	p->crtc_hdisplay = p->hdisplay;
	p->crtc_hsync_start = p->hsync_start;
	p->crtc_hsync_end = p->hsync_end;
	p->crtc_htotal = p->htotal;
	p->crtc_hskew = p->hskew;
	p->crtc_vdisplay = p->vdisplay;
	p->crtc_vsync_start = p->vsync_start;
	p->crtc_vsync_end = p->vsync_end;
	p->crtc_vtotal = p->vtotal;

	if (p->flags & DRM_MODE_FLAG_INTERLACE) {
		if (adjust_flags & CRTC_INTERLACE_HALVE_V) {
			p->crtc_vdisplay /= 2;
			p->crtc_vsync_start /= 2;
			p->crtc_vsync_end /= 2;
			p->crtc_vtotal /= 2;
		}
	}

	if (!(adjust_flags & CRTC_NO_DBLSCAN)) {
		if (p->flags & DRM_MODE_FLAG_DBLSCAN) {
			p->crtc_vdisplay *= 2;
			p->crtc_vsync_start *= 2;
			p->crtc_vsync_end *= 2;
			p->crtc_vtotal *= 2;
		}
	}

	if (!(adjust_flags & CRTC_NO_VSCAN)) {
		if (p->vscan > 1) {
			p->crtc_vdisplay *= p->vscan;
			p->crtc_vsync_start *= p->vscan;
			p->crtc_vsync_end *= p->vscan;
			p->crtc_vtotal *= p->vscan;
		}
	}

	if (adjust_flags & CRTC_STEREO_DOUBLE) {
		unsigned int layout = p->flags & DRM_MODE_FLAG_3D_MASK;

		switch (layout) {
		case DRM_MODE_FLAG_3D_FRAME_PACKING:
			p->crtc_clock *= 2;
			p->crtc_vdisplay += p->crtc_vtotal;
			p->crtc_vsync_start += p->crtc_vtotal;
			p->crtc_vsync_end += p->crtc_vtotal;
			p->crtc_vtotal += p->crtc_vtotal;
			break;
		}
	}

	p->crtc_vblank_start = min(p->crtc_vsync_start, p->crtc_vdisplay);
	p->crtc_vblank_end = max(p->crtc_vsync_end, p->crtc_vtotal);
	p->crtc_hblank_start = min(p->crtc_hsync_start, p->crtc_hdisplay);
	p->crtc_hblank_end = max(p->crtc_hsync_end, p->crtc_htotal);
}

/**
 * drm_mode_is_420_only - if a given videomode can be only supported in YCBCR420
 * output format
 *
 * @connector: drm connector under action.
 * @mode: video mode to be tested.
 *
 * Returns:
 * true if the mode can be supported in YCBCR420 format
 * false if not.
 */
bool drm_mode_is_420_only(const struct drm_display_info *display,
			  const struct drm_display_mode *mode)
{
	u8 vic = drm_match_cea_mode(mode);

	return test_bit(vic, display->hdmi.y420_vdb_modes);
}

/**
 * drm_mode_is_420_also - if a given videomode can be supported in YCBCR420
 * output format also (along with RGB/YCBCR444/422)
 *
 * @display: display under action.
 * @mode: video mode to be tested.
 *
 * Returns:
 * true if the mode can be support YCBCR420 format
 * false if not.
 */
bool drm_mode_is_420_also(const struct drm_display_info *display,
			  const struct drm_display_mode *mode)
{
	u8 vic = drm_match_cea_mode(mode);

	return test_bit(vic, display->hdmi.y420_cmdb_modes);
}

/**
 * drm_mode_is_420 - if a given videomode can be supported in YCBCR420
 * output format
 *
 * @display: display under action.
 * @mode: video mode to be tested.
 *
 * Returns:
 * true if the mode can be supported in YCBCR420 format
 * false if not.
 */
bool drm_mode_is_420(const struct drm_display_info *display,
		     const struct drm_display_mode *mode)
{
	return drm_mode_is_420_only(display, mode) ||
		drm_mode_is_420_also(display, mode);
}

static int display_get_timing(struct display_state *state)
{
	const struct rockchip_connector *conn = state->conn_state.connector;
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector_funcs *conn_funcs = conn->funcs;
	struct drm_display_mode *mode = &conn_state->mode;
	const struct drm_display_mode *m;
	const void *blob = state->blob;
	int conn_node = conn_state->node;
	int panel;

	panel = get_panel_node(state, conn_node);
	if (panel < 0) {
		printf("failed to find panel node\n");
		return -ENODEV;
	}

	if (!display_get_timing_from_dts(panel, blob, mode)) {
		printf("Using display timing dts\n");
		goto done;
	}

	m = rockchip_get_display_mode_from_panel(state);
	if (m) {
		printf("Using display timing from compatible panel driver\n");
		memcpy(mode, m, sizeof(*m));
		goto done;
	}

	rockchip_panel_prepare(state);

	if (conn_funcs->get_edid && !conn_funcs->get_edid(state)) {
		int panel_bits_per_colourp;

		if (!edid_get_timing((void *)&conn_state->edid,
				     sizeof(conn_state->edid), mode,
				     &panel_bits_per_colourp)) {
			printf("Using display timing from edid\n");
			edid_print_info((void *)&conn_state->edid);
			goto done;
		}
	}

	printf("failed to find display timing\n");
	return -ENODEV;
done:
	printf("Detailed mode clock %u kHz, flags[%x]\n"
	       "    H: %04d %04d %04d %04d\n"
	       "    V: %04d %04d %04d %04d\n"
	       "bus_format: %x\n",
	       mode->clock, mode->flags,
	       mode->hdisplay, mode->hsync_start,
	       mode->hsync_end, mode->htotal,
	       mode->vdisplay, mode->vsync_start,
	       mode->vsync_end, mode->vtotal,
	       conn_state->bus_format);

	return 0;
}

static int display_init(struct display_state *state)
{
	const struct rockchip_connector *conn = state->conn_state.connector;
	const struct rockchip_connector_funcs *conn_funcs = conn->funcs;
	struct rockchip_crtc *crtc = state->crtc_state.crtc;
	const struct rockchip_crtc_funcs *crtc_funcs = crtc->funcs;
	const struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	int ret = 0;

	if (state->is_init)
		return 0;

	if (!conn_funcs || !crtc_funcs) {
		printf("failed to find connector or crtc functions\n");
		return -ENXIO;
	}

	if (conn_funcs->init) {
		ret = conn_funcs->init(state);
		if (ret)
			goto deinit_panel;
	}
	/*
	 * support hotplug, but not connect;
	 */

#ifdef CONFIG_ROCKCHIP_DRM_TVE
	if (crtc->hdmi_hpd && conn_state->type == DRM_MODE_CONNECTOR_TV) {
		printf("hdmi plugin ,skip tve\n");
		goto deinit;
	}
#endif
	if (conn_funcs->detect) {
		ret = conn_funcs->detect(state);

#ifdef CONFIG_ROCKCHIP_DRM_TVE
		if (conn_state->type == DRM_MODE_CONNECTOR_HDMIA)
			crtc->hdmi_hpd = ret;
#endif
		if (!ret)
			goto deinit;
	}

	if (conn_funcs->get_timing) {
		ret = conn_funcs->get_timing(state);
		if (ret)
			goto deinit;
	} else {
		ret = display_get_timing(state);
		if (ret)
			goto deinit;
	}
	drm_mode_set_crtcinfo(mode, CRTC_INTERLACE_HALVE_V);

	if (crtc_funcs->init) {
		ret = crtc_funcs->init(state);
		if (ret)
			goto deinit;
	}

	state->is_init = 1;

	return 0;

deinit:
	if (conn_funcs->deinit)
		conn_funcs->deinit(state);
deinit_panel:
	rockchip_panel_deinit(state);
	return ret;
}

static int display_set_plane(struct display_state *state)
{
	const struct rockchip_crtc *crtc = state->crtc_state.crtc;
	const struct rockchip_crtc_funcs *crtc_funcs = crtc->funcs;
	int ret;

	if (!state->is_init)
		return -EINVAL;

	if (crtc_funcs->set_plane) {
		ret = crtc_funcs->set_plane(state);
		if (ret)
			return ret;
	}

	return 0;
}

static int display_enable(struct display_state *state)
{
	const struct rockchip_connector *conn = state->conn_state.connector;
	const struct rockchip_crtc *crtc = state->crtc_state.crtc;
	const struct rockchip_connector_funcs *conn_funcs = conn->funcs;
	const struct rockchip_crtc_funcs *crtc_funcs = crtc->funcs;
	int ret = 0;

	display_init(state);

	if (!state->is_init)
		return -EINVAL;

	if (state->is_enable)
		return 0;

	if (crtc_funcs->prepare) {
		ret = crtc_funcs->prepare(state);
		if (ret)
			return ret;
	}

	if (conn_funcs->prepare) {
		ret = conn_funcs->prepare(state);
		if (ret)
			goto unprepare_crtc;
	}

	rockchip_panel_prepare(state);

	if (crtc_funcs->enable) {
		ret = crtc_funcs->enable(state);
		if (ret)
			goto unprepare_conn;
	}

	if (conn_funcs->enable) {
		ret = conn_funcs->enable(state);
		if (ret)
			goto disable_crtc;
	}

	rockchip_panel_enable(state);

	state->is_enable = true;

	return 0;
unprepare_crtc:
	if (crtc_funcs->unprepare)
		crtc_funcs->unprepare(state);
unprepare_conn:
	if (conn_funcs->unprepare)
		conn_funcs->unprepare(state);
disable_crtc:
	if (crtc_funcs->disable)
		crtc_funcs->disable(state);
	return ret;
}

static int display_disable(struct display_state *state)
{
	const struct rockchip_connector *conn = state->conn_state.connector;
	const struct rockchip_crtc *crtc = state->crtc_state.crtc;
	const struct rockchip_connector_funcs *conn_funcs = conn->funcs;
	const struct rockchip_crtc_funcs *crtc_funcs = crtc->funcs;

	if (!state->is_init)
		return 0;

	if (!state->is_enable)
		return 0;

	rockchip_panel_disable(state);

	if (crtc_funcs->disable)
		crtc_funcs->disable(state);

	if (conn_funcs->disable)
		conn_funcs->disable(state);

	rockchip_panel_unprepare(state);

	if (conn_funcs->unprepare)
		conn_funcs->unprepare(state);

	state->is_enable = 0;
	state->is_init = 0;

	return 0;
}

static int display_logo(struct display_state *state)
{
	struct crtc_state *crtc_state = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct logo_info *logo = &state->logo;
	int hdisplay, vdisplay;

	display_init(state);
	if (!state->is_init)
		return -ENODEV;

	switch (logo->bpp) {
	case 16:
		crtc_state->format = ROCKCHIP_FMT_RGB565;
		break;
	case 24:
		crtc_state->format = ROCKCHIP_FMT_RGB888;
		break;
	case 32:
		crtc_state->format = ROCKCHIP_FMT_ARGB8888;
		break;
	default:
		printf("can't support bmp bits[%d]\n", logo->bpp);
		return -EINVAL;
	}
	crtc_state->rb_swap = logo->bpp != 32;
	hdisplay = conn_state->mode.hdisplay;
	vdisplay = conn_state->mode.vdisplay;
	crtc_state->src_w = logo->width;
	crtc_state->src_h = logo->height;
	crtc_state->src_x = 0;
	crtc_state->src_y = 0;
	crtc_state->ymirror = logo->ymirror;

	crtc_state->dma_addr = logo->mem + logo->offset;
	crtc_state->xvir = ALIGN(crtc_state->src_w * logo->bpp, 32) >> 5;

	if (logo->mode == ROCKCHIP_DISPLAY_FULLSCREEN) {
		crtc_state->crtc_x = 0;
		crtc_state->crtc_y = 0;
		crtc_state->crtc_w = hdisplay;
		crtc_state->crtc_h = vdisplay;
	} else {
		if (crtc_state->src_w >= hdisplay) {
			crtc_state->crtc_x = 0;
			crtc_state->crtc_w = hdisplay;
		} else {
			crtc_state->crtc_x = (hdisplay - crtc_state->src_w) / 2;
			crtc_state->crtc_w = crtc_state->src_w;
		}

		if (crtc_state->src_h >= vdisplay) {
			crtc_state->crtc_y = 0;
			crtc_state->crtc_h = vdisplay;
		} else {
			crtc_state->crtc_y = (vdisplay - crtc_state->src_h) / 2;
			crtc_state->crtc_h = crtc_state->src_h;
		}
	}

	display_set_plane(state);
	display_enable(state);

	return 0;
}

static int get_crtc_id(const void *blob, int connect)
{
	int phandle, remote;
	int val;

	phandle = fdt_getprop_u32_default_node(blob, connect, 0,
					       "remote-endpoint", -1);
	if (phandle < 0)
		goto err;
	remote = fdt_node_offset_by_phandle(blob, phandle);

	val = fdtdec_get_int(blob, remote, "reg", -1);
	if (val < 0)
		goto err;

	return val;
err:
	printf("Can't get crtc id, default set to id = 0\n");
	return 0;
}

static int find_crtc_node(const void *blob, int node)
{
	int nodedepth = fdt_node_depth(blob, node);

	if (nodedepth < 2)
		return -EINVAL;

	return fdt_supernode_atdepth_offset(blob, node,
					    nodedepth - 2, NULL);
}

static int find_connector_node(const void *blob, int node)
{
	int phandle, remote;
	int nodedepth;

	phandle = fdt_getprop_u32_default_node(blob, node, 0,
					       "remote-endpoint", -1);
	remote = fdt_node_offset_by_phandle(blob, phandle);
	nodedepth = fdt_node_depth(blob, remote);

	return fdt_supernode_atdepth_offset(blob, remote,
					    nodedepth - 3, NULL);
}

struct rockchip_logo_cache *find_or_alloc_logo_cache(const char *bmp)
{
	struct rockchip_logo_cache *tmp, *logo_cache = NULL;

	list_for_each_entry(tmp, &logo_cache_list, head) {
		if (!strcmp(tmp->name, bmp)) {
			logo_cache = tmp;
			break;
		}
	}

	if (!logo_cache) {
		logo_cache = malloc(sizeof(*logo_cache));
		if (!logo_cache) {
			printf("failed to alloc memory for logo cache\n");
			return NULL;
		}
		memset(logo_cache, 0, sizeof(*logo_cache));
		strcpy(logo_cache->name, bmp);
		INIT_LIST_HEAD(&logo_cache->head);
		list_add_tail(&logo_cache->head, &logo_cache_list);
	}

	return logo_cache;
}

static int load_bmp_logo(struct logo_info *logo, const char *bmp_name)
{
	struct rockchip_logo_cache *logo_cache;
	struct bmp_header *header;
	void *dst = NULL, *pdst;
	int size;

	if (!logo || !bmp_name)
		return -EINVAL;
	logo_cache = find_or_alloc_logo_cache(bmp_name);
	if (!logo_cache)
		return -ENOMEM;

	if (logo_cache->logo.mem) {
		memcpy(logo, &logo_cache->logo, sizeof(*logo));
		return 0;
	}

	header = get_bmp_header(bmp_name);
	if (!header)
		return -EINVAL;

	logo->bpp = get_unaligned_le16(&header->bit_count);
	logo->width = get_unaligned_le32(&header->width);
	logo->height = get_unaligned_le32(&header->height);
	size = get_unaligned_le32(&header->file_size);
	if (!can_direct_logo(logo->bpp)) {
		if (size > CONFIG_RK_BOOT_BUFFER_SIZE) {
			printf("failed to use boot buf as temp bmp buffer\n");
			return -ENOMEM;
		}
		pdst = (void *)gd->arch.rk_boot_buf_addr;

	} else {
		pdst = get_display_buffer(size);
		dst = pdst;
	}

	if (load_bmp_content(bmp_name, pdst, size)) {
		printf("failed to load bmp %s\n", bmp_name);
		return 0;
	}

	if (!can_direct_logo(logo->bpp)) {
		int dst_size;
		/*
		 * TODO: force use 16bpp if bpp less than 16;
		 */
		logo->bpp = (logo->bpp <= 16) ? 16 : logo->bpp;
		dst_size = logo->width * logo->height * logo->bpp >> 3;

		dst = get_display_buffer(dst_size);
		if (!dst)
			return -ENOMEM;
		if (bmpdecoder(pdst, dst, logo->bpp)) {
			printf("failed to decode bmp %s\n", bmp_name);
			return 0;
		}
		logo->offset = 0;
		logo->ymirror = 0;
	} else {
		logo->offset = get_unaligned_le32(&header->data_offset);
		logo->ymirror = 1;
	}
	logo->mem = (u32)(unsigned long)dst;

	memcpy(&logo_cache->logo, logo, sizeof(*logo));

	return 0;
}

void rockchip_show_bmp(const char *bmp)
{
	struct display_state *s;

	if (!bmp) {
		list_for_each_entry(s, &rockchip_display_list, head)
			display_disable(s);
		return;
	}

	list_for_each_entry(s, &rockchip_display_list, head) {
		s->logo.mode = s->charge_logo_mode;
		if (load_bmp_logo(&s->logo, bmp))
			continue;
		display_logo(s);
	}
}

void rockchip_show_logo(void)
{
	struct display_state *s;

	list_for_each_entry(s, &rockchip_display_list, head) {
		s->logo.mode = s->logo_mode;
		if (load_bmp_logo(&s->logo, s->ulogo_name))
			printf("failed to display uboot logo\n");
		else
			display_logo(s);
		if (load_bmp_logo(&s->logo, s->klogo_name))
			printf("failed to display kernel logo\n");
	}
}

int rockchip_display_init(void)
{
	const void *blob = gd->fdt_blob;
	int route, child, phandle, connect, crtc_node, conn_node;
	const struct rockchip_connector *conn;
	const struct rockchip_crtc *crtc;
	struct display_state *s;
	const char *name;

	route = fdt_path_offset(blob, "/display-subsystem/route");
	if (route < 0) {
		printf("Can't find display display route node\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, route))
		return -ENODEV;

	init_display_buffer();

	fdt_for_each_subnode(blob, child, route) {
		if (!fdt_device_is_available(blob, child))
			continue;

		phandle = fdt_getprop_u32_default_node(blob, child, 0,
						       "connect", -1);
		if (phandle < 0) {
			printf("Warn: %s: can't find connect node's handle\n",
			       fdt_get_name(blob, child, NULL));
			continue;
		}

		connect = fdt_node_offset_by_phandle(blob, phandle);
		if (connect < 0) {
			printf("Warn: %s: can't find connect node\n",
			       fdt_get_name(blob, child, NULL));
			continue;
		}

		crtc_node = find_crtc_node(blob, connect);
		if (!fdt_device_is_available(blob, crtc_node)) {
			printf("Warn: %s: crtc node is not available\n",
			       fdt_get_name(blob, child, NULL));
			continue;
		}
		crtc = rockchip_get_crtc(blob, crtc_node);
		if (!crtc) {
			printf("Warn: %s: can't find crtc driver\n",
			       fdt_get_name(blob, child, NULL));
			continue;
		}

		conn_node = find_connector_node(blob, connect);
		if (!fdt_device_is_available(blob, conn_node)) {
			printf("Warn: %s: connector node is not available\n",
			       fdt_get_name(blob, child, NULL));
			continue;
		}

		conn = rockchip_get_connector(blob, conn_node);
		if (!conn) {
			printf("Warn: %s: can't find connector driver\n",
			       fdt_get_name(blob, child, NULL));
			continue;
		}

		s = malloc(sizeof(*s));
		if (!s)
			goto err_free;

		memset(s, 0, sizeof(*s));

		INIT_LIST_HEAD(&s->head);
		fdt_get_string(blob, child, "logo,uboot", &s->ulogo_name);
		fdt_get_string(blob, child, "logo,kernel", &s->klogo_name);
		fdt_get_string(blob, child, "logo,mode", &name);
		if (!strcmp(name, "fullscreen"))
			s->logo_mode = ROCKCHIP_DISPLAY_FULLSCREEN;
		else
			s->logo_mode = ROCKCHIP_DISPLAY_CENTER;
		fdt_get_string(blob, child, "charge_logo,mode", &name);
		if (!strcmp(name, "fullscreen"))
			s->charge_logo_mode = ROCKCHIP_DISPLAY_FULLSCREEN;
		else
			s->charge_logo_mode = ROCKCHIP_DISPLAY_CENTER;

		s->blob = blob;
		s->conn_state.node = conn_node;
		s->conn_state.connector = conn;
		s->conn_state.overscan.left_margin = 100;
		s->conn_state.overscan.right_margin = 100;
		s->conn_state.overscan.top_margin = 100;
		s->conn_state.overscan.bottom_margin = 100;
		s->crtc_state.node = crtc_node;
		s->crtc_state.crtc = crtc;
		s->crtc_state.crtc_id = get_crtc_id(blob, connect);
		s->node = child;

		connector_phy_init(s);
		connector_panel_init(s);
		list_add_tail(&s->head, &rockchip_display_list);
	}

	return 0;

err_free:
	list_for_each_entry(s, &rockchip_display_list, head) {
		list_del(&s->head);
		free(s);
	}
	return -ENODEV;
}

void rockchip_display_fixup(void *blob)
{
	const struct rockchip_connector_funcs *conn_funcs;
	const struct rockchip_crtc_funcs *crtc_funcs;
	const struct rockchip_connector *conn;
	const struct rockchip_crtc *crtc;
	struct display_state *s;
	u32 offset;
	int node;
	char path[100];
	int ret;

	if (!get_display_size())
		return;

	node = fdt_update_reserved_memory(blob, "rockchip,drm-logo",
					       (u64)memory_start,
					       (u64)get_display_size());
	if (node < 0) {
		printf("failed to add drm-loader-logo memory\n");
		return;
	}

	list_for_each_entry(s, &rockchip_display_list, head) {
		conn = s->conn_state.connector;
		if (!conn)
			continue;
		conn_funcs = conn->funcs;
		if (!conn_funcs) {
			printf("failed to get exist connector\n");
			continue;
		}

		crtc = s->crtc_state.crtc;
		if (!crtc)
			continue;

		crtc_funcs = crtc->funcs;
		if (!crtc_funcs) {
			printf("failed to get exist crtc\n");
			continue;
		}

		if (crtc_funcs->fixup_dts)
			crtc_funcs->fixup_dts(s, blob);

		if (conn_funcs->fixup_dts)
			conn_funcs->fixup_dts(s, blob);

		ret = fdt_get_path(s->blob, s->node, path, sizeof(path));
		if (ret < 0) {
			printf("failed to get route path[%s], ret=%d\n",
			       path, ret);
			continue;
		}

#define FDT_SET_U32(name, val) \
		do_fixup_by_path_u32(blob, path, name, val, 1);

		offset = s->logo.offset + s->logo.mem - memory_start;
		FDT_SET_U32("logo,offset", offset);
		FDT_SET_U32("logo,width", s->logo.width);
		FDT_SET_U32("logo,height", s->logo.height);
		FDT_SET_U32("logo,bpp", s->logo.bpp);
		FDT_SET_U32("logo,ymirror", s->logo.ymirror);
		FDT_SET_U32("video,hdisplay", s->conn_state.mode.hdisplay);
		FDT_SET_U32("video,vdisplay", s->conn_state.mode.vdisplay);
		FDT_SET_U32("video,crtc_hsync_end", s->conn_state.mode.crtc_hsync_end);
		FDT_SET_U32("video,crtc_vsync_end", s->conn_state.mode.crtc_vsync_end);
		FDT_SET_U32("video,vrefresh",
			    drm_mode_vrefresh(&s->conn_state.mode));
		FDT_SET_U32("video,flags", s->conn_state.mode.flags);
		FDT_SET_U32("overscan,left_margin", s->conn_state.overscan.left_margin);
		FDT_SET_U32("overscan,right_margin", s->conn_state.overscan.right_margin);
		FDT_SET_U32("overscan,top_margin", s->conn_state.overscan.top_margin);
		FDT_SET_U32("overscan,bottom_margin", s->conn_state.overscan.bottom_margin);
#undef FDT_SET_U32
	}
}
