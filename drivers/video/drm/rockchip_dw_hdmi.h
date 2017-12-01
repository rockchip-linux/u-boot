/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _ROCKCHIP_DW_HDMI_REG_H_
#define _ROCKCHIP_DW_HDMI_REG_H_

enum output_format {
	output_rgb = 0,
	output_ycbcr444 = 1,
	output_ycbcr422 = 2,
	output_ycbcr420 = 3,
	output_ycbcr_high_subsampling = 4,/* (YCbCr444 > YCbCr422 > YCbCr420 > RGB) */
	output_ycbcr_low_subsampling = 5,	/* (RGB > YCbCr420 > YCbCr422 > YCbCr444) */
	invalid_output = 6,
};

enum  output_depth {
	automatic = 0,
	depth_24bit = 8,
	depth_30bit = 10,
};

struct over_scan {
	unsigned short maxvalue;
	unsigned short leftscale;
	unsigned short rightscale;
	unsigned short topscale;
	unsigned short bottomscale;
};

struct disp_info {
	struct drm_display_mode mode;
	struct over_scan scan;
	enum output_format  format;
	enum output_depth depth;
	unsigned int feature;
	unsigned int reserve[10];
};

struct file_base_paramer {
	struct disp_info main;
	struct disp_info aux;
};

/*
 * Rockchip connector callbacks.
 * If you want to know the details, please refer to rockchip_connector.h
 */
int rockchip_dw_hdmi_init(struct display_state *state);
void rockchip_dw_hdmi_deinit(struct display_state *state);
int rockchip_dw_hdmi_prepare(struct display_state *state);
int rockchip_dw_hdmi_enable(struct display_state *state);
int rockchip_dw_hdmi_disable(struct display_state *state);
int rockchip_dw_hdmi_get_timing(struct display_state *state);
int rockchip_dw_hdmi_detect(struct display_state *state);
int rockchip_dw_hdmi_get_edid(struct display_state *state);

#endif /* _ROCKCHIP_DW_HDMI_REG_H_ */
