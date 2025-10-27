/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef _ROCKCHIP_BASEPARAMETER_H_
#define _ROCKCHIP_BASEPARAMETER_H_

#include <linux/kernel.h>
#include <linux/types.h>

#include <common.h>
#include <drm_modes.h>
#include <edid.h>
#include <part.h>

#define BP_LUT_DATA_ARRAY_SIZE		(1024)
#define BP_CUBIC_LUT_DATA_ARRAY_SIZE	(4913)
#define BP_GAMMA_LUT_DATA_ARRAY_SIZE	(1024)

#define BP_V1_DISP_INFO_BLK_SIZE	(8 * 1024)
#define BP_V1_SCREEN_INFO_ARRAY_SIZE	(5)

#define BP_V2_DISP_INFO_ARRAY_SIZE	(8)
#define BP_V2_SCREEN_INFO_ARRAY_SIZE	(4)

#define ACM_GAIN_LUT_HY_LENGTH		(9 * 17)
#define ACM_GAIN_LUT_HY_TOTAL_LENGTH	(ACM_GAIN_LUT_HY_LENGTH * 3)
#define ACM_GAIN_LUT_HS_LENGTH		(13 * 17)
#define ACM_GAIN_LUT_HS_TOTAL_LENGTH	(ACM_GAIN_LUT_HS_LENGTH * 3)
#define ACM_DELTA_LUT_H_LENGTH		(65)
#define ACM_DELTA_LUT_H_TOTAL_LENGTH	(ACM_DELTA_LUT_H_LENGTH * 3)

#define RESOLUTION_AUTO		BIT(0)
#define COLOR_AUTO		BIT(1)
#define HDCP1X_EN		BIT(2)
#define RESOLUTION_WHITE_EN	BIT(3)
#define ALLM_EN			BIT(4)

enum baseparameter_version {
	RK_BASEPARAMETER_V1 = 1,
	RK_BASEPARAMETER_V2,
	RK_BASEPARAMETER_INVALID,
};

enum bp_output_format {
	RK_IF_FORMAT_RGB,	/* default RGB */
	RK_IF_FORMAT_YCBCR444,	/* YCBCR 444 */
	RK_IF_FORMAT_YCBCR422,	/* YCBCR 422 */
	RK_IF_FORMAT_YCBCR420,	/* YCBCR 420 */
	/* (YCbCr444 > YCbCr422 > YCbCr420 > RGB) */
	RK_IF_FORMAT_YCBCR_HQ,	/* Highest subsampled YUV */
	/* (YCbCr420 > YCbCr422 > YCbCr444 > RGB) */
	RK_IF_FORMAT_YCBCR_LQ,	/* Lowest subsampled YUV */
	RK_IF_FORMAT_MAX,
};

enum bp_output_depth {
	AUTOMATIC = 0,
	DEPTH_24BIT = 8,
	DEPTH_30BIT = 10,
};

struct bp_display_mode {
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
	u32 flags;
	int picture_aspect_ratio;
};

struct bp_bcsh_info {
	u16 brightness;
	u16 contrast;
	u16 saturation;
	u16 hue;
};

struct bp_overscan_info {
	u32 maxvalue;
	u16 leftscale;
	u16 rightscale;
	u16 topscale;
	u16 bottomscale;
};

struct bp_hwc_initial_info {
	char device[128];
	unsigned int framebuffer_width;
	unsigned int framebuffer_height;
	float fps;
};

struct bp_lut_data {
	u16 size;
	u16 lred[BP_LUT_DATA_ARRAY_SIZE];
	u16 lgreen[BP_LUT_DATA_ARRAY_SIZE];
	u16 lblue[BP_LUT_DATA_ARRAY_SIZE];
};

struct bp_csc_info {
	u16 hue;
	u16 saturation;
	u16 contrast;
	u16 brightness;
	u16 r_gain;
	u16 g_gain;
	u16 b_gain;
	u16 r_offset;
	u16 g_offset;
	u16 b_offset;
	u16 csc_enable;
};

struct bp_cubic_lut_data {
	u16 size;
	u16 lred[BP_CUBIC_LUT_DATA_ARRAY_SIZE];
	u16 lgreen[BP_CUBIC_LUT_DATA_ARRAY_SIZE];
	u16 lblue[BP_CUBIC_LUT_DATA_ARRAY_SIZE];
};

struct bp_framebuffer_info {
	u32 framebuffer_width;
	u32 framebuffer_height;
	u32 fps;
};

struct bp_acm_data {
	s16 delta_lut_h[ACM_DELTA_LUT_H_TOTAL_LENGTH];
	s16 gain_lut_hy[ACM_GAIN_LUT_HY_TOTAL_LENGTH];
	s16 gain_lut_hs[ACM_GAIN_LUT_HS_TOTAL_LENGTH];
	u16 y_gain;
	u16 h_gain;
	u16 s_gain;
	u16 acm_enable;
};

struct bp_gamma_lut_data {
	u16 size;
	u16 lred[BP_GAMMA_LUT_DATA_ARRAY_SIZE];
	u16 lgreen[BP_GAMMA_LUT_DATA_ARRAY_SIZE];
	u16 lblue[BP_GAMMA_LUT_DATA_ARRAY_SIZE];
};

struct bp_disp_header {
	u32 connector_type;
	u32 connector_id;
	/* the memory offset based &baseparameter_info of disp info for specific connector */
	u32 offset;
};

struct bp_screen_info {
	u32 type;
	u32 id;
	struct bp_display_mode mode;	/* 52 bytes */
	enum bp_output_format format;	/* 4 bytes */
	enum bp_output_depth depth;	/* 4 bytes */
	u32 feature;
};

union baseparameter_info {
	struct {
		struct {
			struct{
				u32 type;
				struct bp_display_mode mode;	/* 52 bytes */
				enum bp_output_format format;	/* 4 bytes */
				enum bp_output_depth depth;	/* 4 bytes */
				u32 feature;
			} screen_list[BP_V1_SCREEN_INFO_ARRAY_SIZE];
			struct bp_overscan_info scan;		/* 12 bytes */
			struct bp_hwc_initial_info hwc_info;	/* 140 bytes */
			struct bp_bcsh_info bcsh;
			char reserve[512];
			struct bp_lut_data mlutdata;		/* (6k + 2) bytes */
		} main;
		struct {
			struct{
				u32 type;
				struct bp_display_mode mode;	/* 52 bytes */
				enum bp_output_format format;	/* 4 bytes */
				enum bp_output_depth depth;	/* 4 bytes */
				u32 feature;
			} screen_list[BP_V1_SCREEN_INFO_ARRAY_SIZE];
			struct bp_overscan_info scan;		/* 12 bytes */
			struct bp_hwc_initial_info hwc_info;	/* 140 bytes */
			struct bp_bcsh_info bcsh;
			char reserve[512];
			struct bp_lut_data mlutdata;		/* (6k + 2) bytes */
		} aux;
	} baseparameter_info_v1;

	struct {
		char head_flag[4];
		u16 major_version;
		u16 minor_version;
		struct bp_disp_header disp_header[BP_V2_DISP_INFO_ARRAY_SIZE];
		struct {
			char disp_head_flag[6];
			struct {
				u32 type;
				u32 id;
				struct bp_display_mode mode;
				enum bp_output_format format;
				enum bp_output_depth depth;
				u32 feature;
			} screen_info[BP_V2_SCREEN_INFO_ARRAY_SIZE];
			struct bp_bcsh_info bcsh_info;
			struct bp_overscan_info overscan_info;
			struct bp_gamma_lut_data gamma_lut_data;
			struct bp_cubic_lut_data cubic_lut_data;
			struct bp_framebuffer_info framebuffer_info;
			u32 reserved[244];
			u32 crc;
		} disp_info[BP_V2_DISP_INFO_ARRAY_SIZE];
	} baseparameter_info_v2;
};

void rockchip_baseparameter_select_mode(struct hdmi_edid_data *edid_data,
					struct bp_screen_info *screen_info);
int rockchip_baseparameter_disp_info_init(uintptr_t conn_state_ptr, u32 type, u32 id);
int rockchip_baseparameter_screen_info_get(uintptr_t conn_state_ptr, u32 type, u32 id,
					   struct bp_screen_info *screen_info);
struct bp_gamma_lut_data *rockchip_baseparameter_gamma_lut_data_get(uintptr_t conn_state_ptr);
struct bp_cubic_lut_data *rockchip_baseparameter_cubic_lut_data_get(uintptr_t conn_state_ptr);
struct bp_bcsh_info *rockchip_baseparameter_bcsh_info_get(uintptr_t conn_state_ptr);
struct bp_acm_data *rockchip_baseparameter_acm_data_get(uintptr_t conn_state_ptr);
struct bp_csc_info *rockchip_baseparameter_csc_info_get(uintptr_t conn_state_ptr);
struct bp_overscan_info *rockchip_baseparameter_overscan_info_get(uintptr_t conn_state_ptr);
int rockchip_baseparameter_init(void);

#endif /* _ROCKCHIP_BASEPARAMETER_H_ */
