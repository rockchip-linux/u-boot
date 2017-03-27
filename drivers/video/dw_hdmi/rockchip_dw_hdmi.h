/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_HDMI_H_
#define _ROCKCHIP_HDMI_H_

void hdmi_dev_init(struct hdmi_dev *hdmi_dev);
void hdmi_dev_hdcp_start(struct hdmi_dev *hdmi_dev);
int hdmi_dev_control_output(struct hdmi_dev *hdmi_dev, int enable);
int hdmi_dev_insert(struct hdmi_dev *hdmi_dev);
int hdmi_dev_detect_hotplug(struct hdmi_dev *hdmi_dev);
int hdmi_dev_config_video(struct hdmi_dev *hdmi_dev,
			  struct hdmi_video *vpara);
int hdmi_dev_read_edid(struct hdmi_dev *hdmi_dev,
		       int block, unsigned char *buff);

#endif
