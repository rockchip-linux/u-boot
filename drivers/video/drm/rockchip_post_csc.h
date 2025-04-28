/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (C) Rockchip Electronics Co.Ltd
 * Author:
 *      Zhang Yubing <yubing.zhang@rock-chips.com>
 */

#ifndef _ROCKCHIP_POST_CSC_H
#define _ROCKCHIP_POST_CSC_H

#include <linux/kernel.h>
#include <drm/drm_color_mgmt.h>
#include <edid.h>

struct post_csc_convert_mode {
	enum drm_color_encoding color_encoding;
	bool is_input_yuv;
	bool is_output_yuv;
	bool is_input_full_range;
	bool is_output_full_range;
};

struct post_csc_coef {
	s32 csc_coef00;
	s32 csc_coef01;
	s32 csc_coef02;
	s32 csc_coef10;
	s32 csc_coef11;
	s32 csc_coef12;
	s32 csc_coef20;
	s32 csc_coef21;
	s32 csc_coef22;

	s32 csc_dc0;
	s32 csc_dc1;
	s32 csc_dc2;

	u32 range_type;
};

int rockchip_calc_post_csc(struct csc_info *csc_cfg, struct post_csc_coef *csc_simple_coef,
			   struct post_csc_convert_mode *convert_mode);

#endif
