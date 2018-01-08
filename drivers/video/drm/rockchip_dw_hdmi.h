/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _ROCKCHIP_DW_HDMI_REG_H_
#define _ROCKCHIP_DW_HDMI_REG_H_

#include <drm_modes.h>
#include "../rockchip_display.h"

enum output_format {
	output_rgb = 0,
	output_ycbcr444 = 1,
	output_ycbcr422 = 2,
	output_ycbcr420 = 3,
	/* (YCbCr444 > YCbCr422 > YCbCr420 > RGB) */
	output_ycbcr_high_subsampling = 4,
	/* (RGB > YCbCr420 > YCbCr422 > YCbCr444) */
	output_ycbcr_low_subsampling = 5,
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

enum drm_connector_status
inno_dw_hdmi_phy_read_hpd(struct dw_hdmi *hdmi,
			  void *data);
void inno_dw_hdmi_phy_disable(struct dw_hdmi *dw_hdmi,
			      void *data);
int inno_dw_hdmi_phy_init(struct dw_hdmi *dw_hdmi,
			  void *data);

const disk_partition_t *get_disk_partition(const char *name);
int StorageReadLba(u32 LBA, void *pbuf, u32 nSec);

#endif /* _ROCKCHIP_DW_HDMI_REG_H_ */
