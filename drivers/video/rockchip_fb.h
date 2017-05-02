/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
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
#define OUT_P101010				15
#define OUT_YUV_420_10BIT			16
#define OUT_YUV_422				12
#define OUT_YUV_422_10BIT			17
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
#define SCREEN_DP		13

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

#define NO_DUAL		0
#define ONE_DUAL	1
#define DUAL		2
#define DUAL_LCD	3

#define	TTL_DEFAULT_MODE	0
#define	TTL_HVSYNC_MODE		1
#define	TTL_DEN_MODE		2

enum _vop_overlay_mode {
	VOP_RGB_DOMAIN,
	VOP_YUV_DOMAIN
};

enum _vop_output_mode {
	COLOR_RGB,
	COLOR_YCBCR,
	COLOR_YCBCR_BT709,
	COLOR_YCBCR_BT2020,
	COLOR_YCBCR_BT601F
};

#define IS_YUV_COLOR(x)                ((x) >= COLOR_YCBCR)

struct rockchip_fb {
	int node;
	int lcdc_node;
	int lcdc_id;
	struct list_head pwrlist_head;
};

struct rk_lcdc_win_area {
	u16 format;
	u16 xpos;		/*start point in panel  --->LCDC_WINx_DSP_ST*/
	u16 ypos;
	u16 xsize;		/* display window width/height  -->LCDC_WINx_DSP_INFO*/
	u16 ysize;
	u16 xact;		/*origin display window size -->LCDC_WINx_ACT_INFO*/
	u16 yact;
	u16 dsp_stx;
	u16 dsp_sty;
	u8 fbdc_en;
	u16 y_vir_stride;
};

struct rk_lcdc_win {
	u8 state;
	u8 fmt_10;
	u8 rb_swap;

	u32 scale_yrgb_x;
	u32 scale_yrgb_y;
	u32 scale_cbcr_x;
	u32 scale_cbcr_y;

	u8 win_lb_mode;

	u8 bic_coe_el;
	u8 yrgb_hor_scl_mode;//h 01:scale up ;10:down
	u8 yrgb_ver_scl_mode;//v 01:scale up ;10:down
	u8 yrgb_hsd_mode;//h scale down mode
	u8 yrgb_vsu_mode;//v scale up mode
	u8 yrgb_vsd_mode;//v scale down mode
	u8 cbr_hor_scl_mode;
	u8 cbr_ver_scl_mode;
	u8 cbr_hsd_mode;
	u8 cbr_vsu_mode;
	u8 cbr_vsd_mode;
	u8 vsd_yrgb_gt4;
	u8 vsd_yrgb_gt2;
	u8 vsd_cbr_gt4;
	u8 vsd_cbr_gt2;

	u8  mirror_en;
	u8 xmirror;
	u8 ymirror;
	u8 csc_mode;
	struct rk_lcdc_win_area area[1];
};

#if defined(CONFIG_RK_HDMI)
void rk_hdmi_probe(vidinfo_t *panel);
#endif
extern void rk_fb_vidinfo_to_screen(vidinfo_t *vid, struct rk_screen *screen);
extern int rk_fb_vidinfo_to_win(struct fb_dsp_info *fb_info, struct rk_lcdc_win *win);
void lcd_standby(int enable);
void rk_lcdc_standby(int enable);
void lcd_pandispaly(struct fb_dsp_info *info);

int rk_fb_pwr_enable(struct rockchip_fb *fb);
int rk_fb_pwr_disable(struct rockchip_fb *fb);

int rk_pwm_bl_config(int brightness);

#endif /* __ROCKCHIP_H__ */
