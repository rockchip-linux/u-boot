/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _ROCKCHIP_DW_HDMI_REG_H_
#define _ROCKCHIP_DW_HDMI_REG_H_

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
void inno_dw_hdmi_mode_valid(struct dw_hdmi *hdmi, void *data);

const disk_partition_t *get_disk_partition(const char *name);
int StorageReadLba(u32 LBA, void *pbuf, u32 nSec);

#endif /* _ROCKCHIP_DW_HDMI_REG_H_ */
