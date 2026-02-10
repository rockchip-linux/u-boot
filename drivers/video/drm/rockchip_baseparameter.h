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

#define ACM_GAIN_LUT_Y_LENGTH		(9)
#define ACM_GAIN_LUT_S_LENGTH		(13)
#define ACM_GAIN_LUT_H_LENGTH		(65)
#define ACM_GAIN_LUT_H_DOWN_LENGTH	(17)

#define ACM_GAIN_LUT_HY_LENGTH		(ACM_GAIN_LUT_H_DOWN_LENGTH * ACM_GAIN_LUT_Y_LENGTH)
#define ACM_GAIN_LUT_HY_TOTAL_LENGTH	(ACM_GAIN_LUT_HY_LENGTH * 3)
#define ACM_GAIN_LUT_HS_LENGTH		(ACM_GAIN_LUT_H_DOWN_LENGTH * ACM_GAIN_LUT_S_LENGTH)
#define ACM_GAIN_LUT_HS_TOTAL_LENGTH	(ACM_GAIN_LUT_HS_LENGTH * 3)
#define ACM_DELTA_LUT_H_LENGTH		(65)
#define ACM_DELTA_LUT_H_TOTAL_LENGTH	(ACM_DELTA_LUT_H_LENGTH * 3)

#define RESOLUTION_AUTO		BIT(0)
#define COLOR_AUTO		BIT(1)
#define HDCP1X_EN		BIT(2)
#define RESOLUTION_WHITE_EN	BIT(3)
#define ALLM_EN			BIT(4)

enum baseparameter_version {
	RK_BASEPARAMETER_V1_0 = 1,
	RK_BASEPARAMETER_V2_0,
	RK_BASEPARAMETER_V2_1,
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

enum bp_csc_mode {
	HIGH_QUALITY_MODE = 0,
	LOW_LATENCY_MODE = 1,
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
	u32 framebuffer_width;
	u32 framebuffer_height;
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

struct bp_dci_info {
	bool dci_enable;
	u16 dci_wgt_coef_low[33];
	u16 dci_wgt_coef_mid[33];
	u16 dci_wgt_coef_high[33];
	u16 dci_weight_low[32];
	u16 dci_weight_mid[32];
	u16 dci_weight_high[32];
};

struct bp_acm_info {
	bool acm_enable;
	s16 acm_table_delta_yby_h[65];
	s16 acm_table_delta_hby_h[65];
	s16 acm_table_delta_sby_h[65];
	s16 acm_table_gain_yby_y[585];
	s16 acm_table_gain_hby_y[585];
	s16 acm_table_gain_sby_y[585];
	s16 acm_table_gain_yby_s[845];
	s16 acm_table_gain_hby_s[845];
	s16 acm_table_gain_sby_s[845];
	u32 lum_gain;
	u32 hue_gain;
	u32 sat_gain;
};

struct bp_white_balance_info {
	u32 rgain;
	u32 ggain;
	u32 bgain;
};

struct bp_pq_factory_info {
	struct bp_bcsh_info bcsh[4];
	struct bp_white_balance_info white_balance[4];
	u8 cur_bcsh_index;
	u8 cur_white_balance_index;
	u8 cur_dci_index;
	u8 cur_acm_index;
	u8 cur_gamma_index;
	u8 cur_cubic_index;
	u32 crc;
};

struct bp_pq_sharp_info {
	bool sharp_enable;
	u32 sharp_peaking_gain;
	bool sharp_enable_shoot_ctrl;
	u32 sharp_shoot_ctrl_over;
	u32 sharp_shoot_ctrl_under;
	bool sharp_enable_coring_ctrl;
	u16 sharp_coring_ctrl_ratio[4];
	u16 sharp_coring_ctrl_zero[4];
	u16 sharp_coring_ctrl_thrd[4];
	bool sharp_enable_gain_ctrl;
	u16 sharp_gain_ctrl_pos[4];
	bool sharp_enable_limit_ctrl;
	u16 sharp_limit_ctrl_pos0[4];
	u16 sharp_limit_ctrl_pos1[4];
	u16 sharp_limit_ctrl_bnd_pos[4];
	u16 sharp_limit_ctrl_ratio[4];
	u8 cur_sharp_index;
	u32 sharp_peaking_gain_mode[4];
	u8 cur_sharp_peaking_gain_mode_index;
	u32 crc;
};

struct bp_aipq_info {
	bool ai_sd_enable;
	bool ai_sr_enable;
	u32 ai_sr_fix_model_idx;
	bool ai_sr_tuning_enable;
	u32 ai_sr_usm_gain_natural;
	bool ai_sr_usm_enable_ctrl_natural;
	u32 ai_sr_usm_ctrl_over_natural;
	u32 ai_sr_usm_ctrl_under_natural;
	u32 ai_sr_fusion_gain_natural;
	bool ai_sr_fusion_enable_ctrl_natural;
	u32 ai_sr_fusion_ctrl_over_natural;
	u32 ai_sr_fusion_ctrl_under_natural;
	u32 ai_sr_usm_gain_textual;
	bool ai_sr_usm_enable_ctrl_textual;
	u32 ai_sr_usm_ctrl_over_textual;
	u32 ai_sr_usm_ctrl_under_textual;
	u32 ai_sr_fusion_gain_textual;
	bool ai_sr_fusion_enable_ctrl_textual;
	u32 ai_sr_fusion_ctrl_over_textual;
	u32 ai_sr_fusion_ctrl_under_textual;
	u32 crc;
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
			struct bp_overscan_info overscan_info;	/* 12 bytes */
			struct bp_hwc_initial_info hwc_info;	/* 140 bytes */
			struct bp_bcsh_info bcsh_info;
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
			struct bp_overscan_info overscan_info;	/* 12 bytes */
			struct bp_hwc_initial_info hwc_info;	/* 140 bytes */
			struct bp_bcsh_info bcsh_info;
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

		/* Added in v2.1 */
		struct {
			struct {
				bool csc_enable;
				enum bp_csc_mode mode;
				u32 csc_brightness;
				u32 csc_contrast;
				u32 csc_saturation;
				u32 csc_hue;
				u32 csc_r_gain;
				u32 csc_g_gain;
				u32 csc_b_gain;
			} csc_info;
			struct bp_dci_info dci_info;
			struct bp_acm_info acm_info;
			struct bp_gamma_lut_data gamma_lut_data;
			u32 crc;
		} pq_tuning_info;
		struct bp_pq_factory_info pq_factory_info;
		struct bp_pq_sharp_info pq_sharp_info;
		struct bp_aipq_info aipq_info;
	} baseparameter_info_v2;
};

void rockchip_baseparameter_select_mode(struct hdmi_edid_data *edid_data,
					struct bp_screen_info *screen_info);
int rockchip_baseparameter_disp_info_init(uintptr_t conn_state_ptr, u32 type, u32 id);
int rockchip_baseparameter_screen_info_get(uintptr_t conn_state_ptr, u32 type, u32 id,
					   struct bp_screen_info *screen_info);
int rockchip_baseparameter_csc_info_get(uintptr_t conn_state_ptr, struct bp_csc_info *csc_info);
int rockchip_baseparameter_acm_data_get(uintptr_t conn_state_ptr, struct bp_acm_data *acm_data);
struct bp_gamma_lut_data *rockchip_baseparameter_gamma_lut_data_get(uintptr_t conn_state_ptr);
struct bp_cubic_lut_data *rockchip_baseparameter_cubic_lut_data_get(uintptr_t conn_state_ptr);
struct bp_bcsh_info *rockchip_baseparameter_bcsh_info_get(uintptr_t conn_state_ptr);
struct bp_overscan_info *rockchip_baseparameter_overscan_info_get(uintptr_t conn_state_ptr);
int rockchip_baseparameter_init(void);

#endif /* _ROCKCHIP_BASEPARAMETER_H_ */
