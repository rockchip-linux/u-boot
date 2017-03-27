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

#include "../rockchip_display.h"
#include "../rockchip_crtc.h"
#include "../rockchip_connector.h"
#include "rockchip_dw_hdmi_reg.h"
#include "rockchip_dw_hdmi.h"

#define RK3399_GRF_SOC_CON20	0x6250
#define RK3288_GRF_SOC_CON6	0x025c
#define EDID_BLOCK_LENGTH	0x80

struct dw_hdmi {
	unsigned char edid[EDID_BLOCK_LENGTH * 2];
	struct hdmi_dev *hdmi_dev;
};

struct dw_hdmi_plat_data {
	u32 vop_sel_bit;
	u32 grf_vop_sel_reg;
};

const struct dw_hdmi_plat_data rk3288_hdmi_drv_data = {
	.vop_sel_bit = 4,
	.grf_vop_sel_reg = RK3288_GRF_SOC_CON6,
};

const struct dw_hdmi_plat_data rk3399_hdmi_drv_data = {
	.vop_sel_bit = 6,
	.grf_vop_sel_reg = RK3399_GRF_SOC_CON20,
};

extern void dw_rk_hdmi_register(struct hdmi_dev *hdmi_dev);
static int rockchip_dw_hdmi_init(struct display_state *state)
{
	struct hdmi_dev *hdmi_dev;
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	const struct dw_hdmi_plat_data *pdata = connector->data;
	struct crtc_state *crtc_state = &state->crtc_state;
	struct dw_hdmi *hdmi;
	u32 val;

	hdmi_dev = malloc(sizeof(struct hdmi_dev));
	if (!hdmi_dev)
		return -ENOMEM;

	memset(hdmi_dev, 0, sizeof(struct hdmi_dev));
	hdmi_dev->regbase = (void *)RKIO_HDMI_PHYS;
	hdmi_dev->read_edid = hdmi_dev_read_edid;
#if CONFIG_RKCHIP_RK3288
	hdmi_dev->soctype = HDMI_SOC_RK3288;
	hdmi_dev->feature = SUPPORT_4K |
			    SUPPORT_DEEP_10BIT |
			    SUPPORT_TMDS_600M;
	strcpy(hdmi_dev->compatible, "rockchip,rk3288-dw-hdmi");
#elif CONFIG_RKCHIP_RK3399
	hdmi_dev->soctype = HDMI_SOC_RK3399;
	hdmi_dev->feature = SUPPORT_4K |
			    SUPPORT_4K_4096 |
			    /* SUPPORT_YCBCR_INPUT | */
			    SUPPORT_1080I |
			    SUPPORT_480I_576I |
			    SUPPORT_YUV420 |
			    SUPPORT_DEEP_10BIT;
        strcpy(hdmi_dev->compatible, "rockchip,rk3399-dw-hdmi");
#endif
	hdmi_dev->video.color_input = HDMI_COLOR_RGB_0_255;
	hdmi_dev->video.sink_hdmi = OUTPUT_HDMI;
	hdmi_dev->video.format_3d = HDMI_3D_NONE;
	hdmi_dev->video.color_output = HDMI_COLOR_RGB_0_255;
	hdmi_dev->video.color_output_depth = 8;

	dw_rk_hdmi_register(hdmi_dev);

	if (crtc_state->crtc_id)
		val = ((1 << pdata->vop_sel_bit) |
		       (1 << (16 + pdata->vop_sel_bit)));
	else
		val = ((0 << pdata->vop_sel_bit) |
		       (1 << (16 + pdata->vop_sel_bit)));
	grf_writel(val, pdata->grf_vop_sel_reg);

	conn_state->output_mode = ROCKCHIP_OUT_MODE_AAAA;
	conn_state->type = DRM_MODE_CONNECTOR_HDMIA;
	hdmi_dev_init(hdmi_dev);

	hdmi = malloc(sizeof(struct dw_hdmi));
	if (!hdmi)
		return -ENOMEM;
	memset(hdmi, 0, sizeof(struct dw_hdmi));
	hdmi->hdmi_dev = hdmi_dev;
	conn_state->private = hdmi;

	return 0;
}

static void rockchip_dw_hdmi_deinit(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_hdmi *hdmi = conn_state->private;
	struct hdmi_dev *hdmi_dev;

	if (hdmi) {
		hdmi_dev = hdmi->hdmi_dev;
		if (hdmi_dev)
			free(hdmi_dev);
		free(hdmi);
	}
}

static int rockchip_dw_hdmi_prepare(struct display_state *state)
{
	return 0;
}

static int rockchip_dw_hdmi_enable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_hdmi *hdmi = conn_state->private;
	struct hdmi_dev *hdmi_dev;

	if (!hdmi)
		return -EFAULT;

	hdmi_dev = hdmi->hdmi_dev;
	if (!hdmi_dev)
		return -EFAULT;

	hdmi_dev_config_video(hdmi_dev, &hdmi_dev->video);
	hdmi_dev_hdcp_start(hdmi_dev);
	hdmi_dev_control_output(hdmi_dev, HDMI_AV_UNMUTE);

	return 0;
}

static int rockchip_dw_hdmi_disable(struct display_state *state)
{
	return 0;
}

struct display_state *g_state;
struct drm_display_mode *g_mode;

static int rockchip_dw_hdmi_get_timing(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	const struct video_mode *videomode = NULL;
	struct hdmi_dev	*hdmi_dev;
	struct dw_hdmi *hdmi = conn_state->private;

	if (!hdmi)
		return -EFAULT;

	hdmi_dev = hdmi->hdmi_dev;
	if (!hdmi_dev)
		return -EFAULT;

	hdmi_dev_insert(hdmi_dev);
	videomode = hdmi_edid_get_timing(hdmi_dev);

	mode->hdisplay = videomode->xres;
	mode->hsync_start = mode->hdisplay + videomode->right_margin;
	mode->hsync_end = mode->hsync_start + videomode->hsync_len;
	mode->htotal = mode->hsync_end + videomode->left_margin;
	mode->vdisplay = videomode->yres;
	mode->vsync_start = mode->vdisplay + videomode->lower_margin;
	mode->vsync_end = mode->vsync_start + videomode->vsync_len;
	mode->vtotal = mode->vsync_end + videomode->upper_margin;
	mode->clock = videomode->pixclock / 1000;

	g_state = state;
	g_mode = mode;
	return 0;
}

static int rockchip_dw_hdmi_detect(struct display_state *state)
{
	int ret;
	struct connector_state *conn_state = &state->conn_state;
	struct hdmi_dev *hdmi_dev;
	struct dw_hdmi *hdmi = conn_state->private;

	if (!hdmi)
		return -EFAULT;

	hdmi_dev = hdmi->hdmi_dev;
	if (!hdmi_dev)
		return -EFAULT;

	ret = hdmi_dev_detect_hotplug(hdmi_dev);
	return ret;
}

static int rockchip_dw_hdmi_get_edid(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_hdmi *hdmi = conn_state->private;
	struct hdmi_dev *hdmi_dev;
	int i;

	if (!hdmi)
		return -EFAULT;

	hdmi_dev = hdmi->hdmi_dev;
	if (!hdmi_dev)
		return -EFAULT;

	for (i = 0; i < 3; i++) {
		hdmi_dev->read_edid(hdmi_dev, i, hdmi->edid);
	}
	memcpy(&conn_state->edid, &hdmi->edid, sizeof(hdmi->edid));
	return 0;
}

const struct rockchip_connector_funcs rockchip_dw_hdmi_funcs = {
	.init = rockchip_dw_hdmi_init,
	.deinit = rockchip_dw_hdmi_deinit,
	.prepare = rockchip_dw_hdmi_prepare,
	.enable = rockchip_dw_hdmi_enable,
	.disable = rockchip_dw_hdmi_disable,
	.get_timing = rockchip_dw_hdmi_get_timing,
	.detect = rockchip_dw_hdmi_detect,
	.get_edid = rockchip_dw_hdmi_get_edid,
};
