/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * Author: Andy Yan <andy.yan@rock-chips.com>
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __ROCKCHIP_H__
#define __ROCKCHIP_H__
#include <linux/fb.h>
#include <linux/rk_screen.h>

#define FB_DEFAULT_ORDER			0
#define FB0_WIN2_FB1_WIN1_FB2_WIN0		12
#define FB0_WIN1_FB1_WIN2_FB2_WIN0		21
#define FB0_WIN2_FB1_WIN0_FB2_WIN1		102
#define FB0_WIN0_FB1_WIN2_FB2_WIN1		120
#define FB0_WIN0_FB1_WIN1_FB2_WIN2		210
#define FB0_WIN1_FB1_WIN0_FB2_WIN2		201
#define FB0_WIN0_FB1_WIN1_FB2_WIN2_FB3_WIN3	3210

#define OUT_P888				0
#define OUT_P666				1
#define OUT_P565				2
#define OUT_S888x				4
#define OUT_CCIR656				6
#define OUT_S888				8
#define OUT_S888DUMY				12
#define OUT_YUV_420				14
#define OUT_RGB_AAA				15
#define OUT_P16BPP4				24
#define OUT_D888_P666				0x21
#define OUT_D888_P565				0x22

#define SCREEN_NULL				0
#define SCREEN_RGB				1
#define SCREEN_LVDS				2
#define SCREEN_DUAL_LVDS			3
#define SCREEN_MCU				4
#define SCREEN_TVOUT				5
#define SCREEN_HDMI				6
#define SCREEN_MIPI				7
#define SCREEN_DUAL_MIPI			8
#define SCREEN_EDP				9
#define SCREEN_TVOUT_TEST       10

#define LVDS_8BIT_1				0
#define LVDS_8BIT_2				1
#define LVDS_8BIT_3				2
#define LVDS_6BIT				3

#define NO_MIRROR				0
#define X_MIRROR				1
#define Y_MIRROR				2
#define X_Y_MIRROR				3

#define PRMRY					1
#define EXTEND					2

struct rockchip_fb {
	int node;
	int lcdc_node;
	int lcdc_id;
	struct list_head pwrlist_head;
};


#if defined(CONFIG_RK_HDMI)
void rk_hdmi_probe(vidinfo_t *panel);
#endif
extern void rk_fb_vidinfo_to_screen(vidinfo_t *vid, struct rk_screen *screen);

void lcd_standby(int enable);
void rk_lcdc_standby(int enable);
void lcd_pandispaly(struct fb_dsp_info *info);

int rk_fb_pwr_enable(struct rockchip_fb *fb);
int rk_fb_pwr_disable(struct rockchip_fb *fb);

#endif /* __ROCKCHIP_H__ */
