// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 * Author: Damon Ding <damon.ding@rock-chips.com>
 */

#include <asm/arch-rockchip/common.h>
#include <asm/unaligned.h>
#include <malloc.h>
#include <memalign.h>
#include <u-boot/crc.h>

#include "rockchip_baseparameter.h"

static LIST_HEAD(bp_disp_info_list);

static union baseparameter_info *bp_info;

enum baseparameter_version bp_version = RK_BASEPARAMETER_INVALID;

struct bp_disp_info_iter {
	struct list_head head;
	enum baseparameter_version bp_version;
	uintptr_t conn_state_ptr;
	int index;
};

static void drm_display_mode_from_bp_display_mode(struct drm_display_mode *mode,
						  struct bp_display_mode *bp_mode)
{
	mode->clock = bp_mode->clock;
	mode->hdisplay = bp_mode->hdisplay;
	mode->hsync_start = bp_mode->hsync_start;
	mode->hsync_end = bp_mode->hsync_end;
	mode->htotal = bp_mode->htotal;
	mode->vdisplay = bp_mode->vdisplay;
	mode->vsync_start = bp_mode->vsync_start;
	mode->vsync_end = bp_mode->vsync_end;
	mode->vtotal = bp_mode->vtotal;
	mode->vrefresh = bp_mode->vrefresh;
	mode->vscan = bp_mode->vscan;
	mode->flags = bp_mode->flags;
	mode->picture_aspect_ratio = bp_mode->picture_aspect_ratio;
}

static bool rockchip_baseparameter_version_v1(void)
{
	return bp_version == RK_BASEPARAMETER_V1_0;
}

static bool rockchip_baseparameter_version_v2(void)
{
	return bp_version == RK_BASEPARAMETER_V2_0 || bp_version == RK_BASEPARAMETER_V2_1;
}

static int rockchip_baseparameter_disp_info_v1(int type)
{
	int i = 0;

	for (i = 0; i < BP_V1_SCREEN_INFO_ARRAY_SIZE; i++) {
		if (bp_info->baseparameter_info_v1.main.screen_list[i].type == type) {
			printf("INFO: Screen info(MAIN) index[%d]: type[%d]\n", i, type);
			return i;
		}
	}

	printf("INFO: try to match disp info in AUX partition\n");

	for (i = 0; i < BP_V1_SCREEN_INFO_ARRAY_SIZE; i++) {
		if (bp_info->baseparameter_info_v1.aux.screen_list[i].type == type) {
			printf("INFO: Screen info(AUX) index[%d]: type[%d]\n", i, type);
			return i + BP_V1_SCREEN_INFO_ARRAY_SIZE;
		}
	}

	pr_err("ERROR: Disp info couldn't be found, screen info type[%d] mismatched\n", type);

	return -EINVAL;
}

static int rockchip_baseparameter_disp_info_v2(u32 type, u32 id)
{
	struct bp_disp_header *disp_header;
	int i = 0, offset = -1;
	u32 crc_val;
	void *baseparameter_addr = (void *)&bp_info->baseparameter_info_v2;

	for (i = 0; i < BP_V2_DISP_INFO_ARRAY_SIZE; i++) {
		disp_header = &bp_info->baseparameter_info_v2.disp_header[i];
		if (disp_header->connector_type == type && disp_header->connector_id == id) {
			printf("INFO: Disp info index[%d]: type[%d] id[%d]\n", i, type, id);
			offset = disp_header->offset;
			break;
		}
	}
	if (offset < 0)
		return -EINVAL;

	for (i = 0; i < BP_V2_DISP_INFO_ARRAY_SIZE; i++) {
		if (baseparameter_addr + offset == (void *)&bp_info->baseparameter_info_v2.disp_info[i])
			break;
	}
	if (i == BP_V2_DISP_INFO_ARRAY_SIZE)
		return -EINVAL;

	if (strncasecmp(bp_info->baseparameter_info_v2.disp_info[i].disp_head_flag, "DISP", 4))
		return -EINVAL;

	crc_val = crc32(0, (unsigned char *)&bp_info->baseparameter_info_v2.disp_info[i],
			sizeof(bp_info->baseparameter_info_v2.disp_info[i]) -
			sizeof(bp_info->baseparameter_info_v2.disp_info[i].crc));
	if (crc_val != bp_info->baseparameter_info_v2.disp_info[i].crc) {
		pr_err("ERROR: Connector type[%d] id[%d] CRC mismatched\n", type, id);
		return -EINVAL;
	}

	return i;
}

int rockchip_baseparameter_disp_info_init(uintptr_t conn_state_ptr, u32 type, u32 id)
{
	struct bp_disp_info_iter *disp_info_iter;
	int index;

	if (bp_version == RK_BASEPARAMETER_INVALID)
		return -EINVAL;

	list_for_each_entry(disp_info_iter, &bp_disp_info_list, head) {
		if (disp_info_iter->conn_state_ptr == conn_state_ptr)
			return 0;
	}

	if (rockchip_baseparameter_version_v1())
		index = rockchip_baseparameter_disp_info_v1(type);
	else if (rockchip_baseparameter_version_v2())
		index = rockchip_baseparameter_disp_info_v2(type, id);
	if (index < 0)
		return -EINVAL;

	disp_info_iter = malloc(sizeof(struct bp_disp_info_iter));
	disp_info_iter->bp_version = bp_version;
	disp_info_iter->conn_state_ptr = conn_state_ptr;
	disp_info_iter->index = index;
	list_add_tail(&disp_info_iter->head, &bp_disp_info_list);

	return 0;
}

static int rockchip_baseparameter_disp_info_get(uintptr_t conn_state_ptr)
{
	struct bp_disp_info_iter *disp_info_iter;

	list_for_each_entry(disp_info_iter, &bp_disp_info_list, head) {
		if (disp_info_iter->conn_state_ptr == conn_state_ptr)
			return disp_info_iter->index;
	}

	return -EINVAL;
}

static int rockchip_baseparameter_screen_info_v1(uintptr_t conn_state_ptr, u32 type, u32 id,
						 struct bp_screen_info *screen_info)
{
	int index;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return -EINVAL;

	if (index < BP_V1_SCREEN_INFO_ARRAY_SIZE) {
		screen_info->type = bp_info->baseparameter_info_v1.main.screen_list[index].type;
		screen_info->mode = bp_info->baseparameter_info_v1.main.screen_list[index].mode;
		screen_info->format = bp_info->baseparameter_info_v1.main.screen_list[index].format;
		screen_info->depth = bp_info->baseparameter_info_v1.main.screen_list[index].depth;
		screen_info->feature = bp_info->baseparameter_info_v1.main.screen_list[index].feature;
	} else {
		index -= BP_V1_SCREEN_INFO_ARRAY_SIZE;
		screen_info->type = bp_info->baseparameter_info_v1.aux.screen_list[index].type;
		screen_info->mode = bp_info->baseparameter_info_v1.aux.screen_list[index].mode;
		screen_info->format = bp_info->baseparameter_info_v1.aux.screen_list[index].format;
		screen_info->depth = bp_info->baseparameter_info_v1.aux.screen_list[index].depth;
		screen_info->feature = bp_info->baseparameter_info_v1.aux.screen_list[index].feature;
	}

	return 0;
}

static int rockchip_baseparameter_screen_info_v2(uintptr_t conn_state_ptr, u32 type, u32 id,
						 struct bp_screen_info *screen_info)
{
	int index;
	int i;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return -EINVAL;

	for (i = 0; i < BP_V2_SCREEN_INFO_ARRAY_SIZE; i++) {
		if (bp_info->baseparameter_info_v2.disp_info[index].screen_info[i].type == type ||
		    bp_info->baseparameter_info_v2.disp_info[index].screen_info[i].id == id)
			break;
	}
	if (i == BP_V2_SCREEN_INFO_ARRAY_SIZE) {
		pr_err("ERROR: Screen info type[%d] or id[%d] mismatched\n", type, id);
		return -EINVAL;
	}

	screen_info->type = bp_info->baseparameter_info_v2.disp_info[index].screen_info[i].type;
	screen_info->mode = bp_info->baseparameter_info_v2.disp_info[index].screen_info[i].mode;
	screen_info->format = bp_info->baseparameter_info_v2.disp_info[index].screen_info[i].format;
	screen_info->depth = bp_info->baseparameter_info_v2.disp_info[index].screen_info[i].depth;
	screen_info->feature = bp_info->baseparameter_info_v2.disp_info[index].screen_info[i].feature;

	return 0;
}

int rockchip_baseparameter_screen_info_get(uintptr_t conn_state_ptr, u32 type, u32 id,
					   struct bp_screen_info *screen_info)
{
	int ret;

	if (!screen_info)
		return -EINVAL;

	if (rockchip_baseparameter_version_v1()) {
		ret = rockchip_baseparameter_screen_info_v1(conn_state_ptr, type, id, screen_info);
		if (ret)
			pr_warn("WARN: Failed to find screen info in v1 baseparameter\n");
	} else if (rockchip_baseparameter_version_v2()) {
		ret = rockchip_baseparameter_screen_info_v2(conn_state_ptr, type, id, screen_info);
		if (ret)
			pr_warn("WARN: Failed to find screen info in v2 baseparameter\n");
	} else {
		pr_warn("WARN: Unsupported baseparameter version[%d] for screen info\n",
			bp_version);
		ret = -EINVAL;
	}

	return ret;
}

static int rockchip_baseparameter_csc_info_v2(uintptr_t conn_state_ptr,
					      struct bp_csc_info *csc_info)
{
	int index;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return -EINVAL;

	if (bp_version < RK_BASEPARAMETER_V2_1) {
		printf("INFO: Cureent version[%d]. Only v2.1 and later versions can support csc info\n",
		       bp_version);
		return -EINVAL;
	}

	csc_info->hue = bp_info->baseparameter_info_v2.pq_tuning_info.csc_info.csc_hue;
	csc_info->saturation = bp_info->baseparameter_info_v2.pq_tuning_info.csc_info.csc_saturation;
	csc_info->contrast = bp_info->baseparameter_info_v2.pq_tuning_info.csc_info.csc_contrast;
	csc_info->r_gain = bp_info->baseparameter_info_v2.pq_tuning_info.csc_info.csc_r_gain;
	csc_info->g_gain = bp_info->baseparameter_info_v2.pq_tuning_info.csc_info.csc_g_gain;
	csc_info->b_gain = bp_info->baseparameter_info_v2.pq_tuning_info.csc_info.csc_b_gain;
	csc_info->r_offset = 0;
	csc_info->g_offset = 0;
	csc_info->b_offset = 0;
	csc_info->csc_enable = bp_info->baseparameter_info_v2.pq_tuning_info.csc_info.csc_enable;

	return 0;
}

int rockchip_baseparameter_csc_info_get(uintptr_t conn_state_ptr, struct bp_csc_info *csc_info)
{
	int ret;

	if (!csc_info)
		return -EINVAL;

	if (rockchip_baseparameter_version_v2()) {
		ret = rockchip_baseparameter_csc_info_v2(conn_state_ptr, csc_info);
		if (ret)
			pr_warn("WARN: Failed to parse csc info in v2 baseparameter\n");
	} else {
		pr_warn("WARN: Unsupported baseparameter version[%d] for csc info\n",
			bp_version);
		ret = -EINVAL;
	}

	return ret;
}

static struct bp_gamma_lut_data *rockchip_baseparameter_gamma_lut_data_v2(uintptr_t conn_state_ptr)
{
	int index;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return NULL;

	if (!bp_info->baseparameter_info_v2.disp_info[index].gamma_lut_data.size)
		return NULL;

	return &bp_info->baseparameter_info_v2.disp_info[index].gamma_lut_data;
}

struct bp_gamma_lut_data *rockchip_baseparameter_gamma_lut_data_get(uintptr_t conn_state_ptr)
{
	struct bp_gamma_lut_data *lut_data = NULL;

	if (rockchip_baseparameter_version_v2()) {
		lut_data = rockchip_baseparameter_gamma_lut_data_v2(conn_state_ptr);
		if (!lut_data)
			pr_warn("WARN: Failed to find gamma lut data in v2 baseparameter\n");
	} else {
		pr_warn("WARN: Unsupported baseparameter version[%d] for gamma lut data\n",
			bp_version);
	}

	return lut_data;
}

static struct bp_cubic_lut_data *rockchip_baseparameter_cubic_lut_data_v2(uintptr_t conn_state_ptr)
{
	int index;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return NULL;

	if (!bp_info->baseparameter_info_v2.disp_info[index].cubic_lut_data.size)
		return NULL;

	return &bp_info->baseparameter_info_v2.disp_info[index].cubic_lut_data;
}

struct bp_cubic_lut_data *rockchip_baseparameter_cubic_lut_data_get(uintptr_t conn_state_ptr)
{
	struct bp_cubic_lut_data *lut_data = NULL;

	if (rockchip_baseparameter_version_v2()) {
		lut_data = rockchip_baseparameter_cubic_lut_data_v2(conn_state_ptr);
		if (!lut_data)
			pr_warn("WARN: Failed to find cubic lut data in v2 baseparameter\n");
	} else {
		pr_warn("WARN: Unsupported baseparameter version[%d] for cubic lut data\n",
			bp_version);
	}

	return lut_data;
}

static struct bp_bcsh_info *rockchip_baseparameter_bcsh_info_v2(uintptr_t conn_state_ptr)
{
	int index;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return NULL;

	return &bp_info->baseparameter_info_v2.disp_info[index].bcsh_info;
}

struct bp_bcsh_info *rockchip_baseparameter_bcsh_info_get(uintptr_t conn_state_ptr)
{
	struct bp_bcsh_info *bcsh_info = NULL;

	if (rockchip_baseparameter_version_v2()) {
		bcsh_info = rockchip_baseparameter_bcsh_info_v2(conn_state_ptr);
		if (!bcsh_info)
			pr_warn("WARN: Failed to find bcsh info in v2 baseparameter\n");
	} else {
		pr_warn("WARN: Unsupported baseparameter version[%d] for bcsh info\n", bp_version);
	}

	return bcsh_info;
}

static void rockchip_baseparameter_acm_info_to_acm_data(const struct bp_acm_info *swpq_acm,
							struct bp_acm_data *hwpq_acm)
{
	// Gaussian kernel for downsampling
	const int gaussian_kernel[5] = {1, 4, 6, 4, 1}; // sum to 16
	const int sample_ratio = 4;                     // 64/16 = 4
	int sum_y, sum_h, sum_s;
	int dst_idx_y, dst_idx_h, dst_idx_s;
	int src_idx_h;
	int i, y, h, k, s;

	// Check for null pointers
	if (!swpq_acm || !hwpq_acm) {
		return;
	}

	// Convert delta_lut_h array: concate each array
	for (i = 0; i < ACM_DELTA_LUT_H_LENGTH; i++) {
		hwpq_acm->delta_lut_h[0 * ACM_DELTA_LUT_H_LENGTH + i] = swpq_acm->acm_table_delta_yby_h[i];
		hwpq_acm->delta_lut_h[1 * ACM_DELTA_LUT_H_LENGTH + i] = swpq_acm->acm_table_delta_hby_h[i];
		hwpq_acm->delta_lut_h[2 * ACM_DELTA_LUT_H_LENGTH + i] = swpq_acm->acm_table_delta_sby_h[i];
	}

	// Convert gain_lut_hy array with Gaussian downsampling (window size 5) with wrap boundary
	// Original size: 65x9, Downsampled size: 9x17 (transpose also)
	for (y = 0; y < ACM_GAIN_LUT_Y_LENGTH; y++) {
		for (h = 0; h < ACM_GAIN_LUT_H_DOWN_LENGTH - 1; h++) {
			sum_y = 0;
			sum_h = 0;
			sum_s = 0;
			for (k = 0; k < 5; k++) {
				src_idx_h = (h * sample_ratio + k - 2 + 65 - 1) % (65 - 1);
				sum_y += swpq_acm->acm_table_gain_yby_y[src_idx_h * ACM_GAIN_LUT_Y_LENGTH + y] * gaussian_kernel[k];
				sum_h += swpq_acm->acm_table_gain_hby_y[src_idx_h * ACM_GAIN_LUT_Y_LENGTH + y] * gaussian_kernel[k];
				sum_s += swpq_acm->acm_table_gain_sby_y[src_idx_h * ACM_GAIN_LUT_Y_LENGTH + y] * gaussian_kernel[k];
			}

			dst_idx_y = 0 * ACM_GAIN_LUT_HY_LENGTH + y * ACM_GAIN_LUT_H_DOWN_LENGTH + h;
			dst_idx_h = 1 * ACM_GAIN_LUT_HY_LENGTH + y * ACM_GAIN_LUT_H_DOWN_LENGTH + h;
			dst_idx_s = 2 * ACM_GAIN_LUT_HY_LENGTH + y * ACM_GAIN_LUT_H_DOWN_LENGTH + h;
			hwpq_acm->gain_lut_hy[dst_idx_y] = (sum_y + 8 + (sum_y >> 31)) >> 4;
			hwpq_acm->gain_lut_hy[dst_idx_h] = (sum_h + 8 + (sum_h >> 31)) >> 4;
			hwpq_acm->gain_lut_hy[dst_idx_s] = (sum_s + 8 + (sum_s >> 31)) >> 4;
		}

		// set the last value same to the first value on H-axis
		dst_idx_y = 0 * ACM_GAIN_LUT_HY_LENGTH + y * ACM_GAIN_LUT_H_DOWN_LENGTH;
		dst_idx_h = 1 * ACM_GAIN_LUT_HY_LENGTH + y * ACM_GAIN_LUT_H_DOWN_LENGTH;
		dst_idx_s = 2 * ACM_GAIN_LUT_HY_LENGTH + y * ACM_GAIN_LUT_H_DOWN_LENGTH;
		hwpq_acm->gain_lut_hy[dst_idx_y + ACM_GAIN_LUT_H_DOWN_LENGTH - 1] = hwpq_acm->gain_lut_hy[dst_idx_y];
		hwpq_acm->gain_lut_hy[dst_idx_h + ACM_GAIN_LUT_H_DOWN_LENGTH - 1] = hwpq_acm->gain_lut_hy[dst_idx_h];
		hwpq_acm->gain_lut_hy[dst_idx_s + ACM_GAIN_LUT_H_DOWN_LENGTH - 1] = hwpq_acm->gain_lut_hy[dst_idx_s];
	}

	// Convert gain_lut_hs array with Gaussian downsampling (window size 5) with wrap boundary
	// Original size: 65x13, Downsampled size: 13x17 (transpose also)
	for (s = 0; s < ACM_GAIN_LUT_S_LENGTH; s++) {
		for (h = 0; h < ACM_GAIN_LUT_H_DOWN_LENGTH - 1; h++) {
			sum_y = 0;
			sum_h = 0;
			sum_s = 0;
			for (k = 0; k < 5; k++) {
				src_idx_h = (h * sample_ratio + k - 2 + 65 - 1) % (65 - 1);
				sum_y += swpq_acm->acm_table_gain_yby_s[src_idx_h * ACM_GAIN_LUT_S_LENGTH + s] * gaussian_kernel[k];
				sum_h += swpq_acm->acm_table_gain_hby_s[src_idx_h * ACM_GAIN_LUT_S_LENGTH + s] * gaussian_kernel[k];
				sum_s += swpq_acm->acm_table_gain_sby_s[src_idx_h * ACM_GAIN_LUT_S_LENGTH + s] * gaussian_kernel[k];
			}

			dst_idx_y = 0 * ACM_GAIN_LUT_HS_LENGTH + s * ACM_GAIN_LUT_H_DOWN_LENGTH + h;
			dst_idx_h = 1 * ACM_GAIN_LUT_HS_LENGTH + s * ACM_GAIN_LUT_H_DOWN_LENGTH + h;
			dst_idx_s = 2 * ACM_GAIN_LUT_HS_LENGTH + s * ACM_GAIN_LUT_H_DOWN_LENGTH + h;
			hwpq_acm->gain_lut_hs[dst_idx_y] = (sum_y + 8 + (sum_y >> 31)) >> 4;
			hwpq_acm->gain_lut_hs[dst_idx_h] = (sum_h + 8 + (sum_h >> 31)) >> 4;
			hwpq_acm->gain_lut_hs[dst_idx_s] = (sum_s + 8 + (sum_s >> 31)) >> 4;
		}

		// set the last value same to the first value on H-axis
		dst_idx_y = 0 * ACM_GAIN_LUT_HS_LENGTH + s * ACM_GAIN_LUT_H_DOWN_LENGTH;
		dst_idx_h = 1 * ACM_GAIN_LUT_HS_LENGTH + s * ACM_GAIN_LUT_H_DOWN_LENGTH;
		dst_idx_s = 2 * ACM_GAIN_LUT_HS_LENGTH + s * ACM_GAIN_LUT_H_DOWN_LENGTH;
		hwpq_acm->gain_lut_hs[dst_idx_y + ACM_GAIN_LUT_H_DOWN_LENGTH - 1] = hwpq_acm->gain_lut_hs[dst_idx_y];
		hwpq_acm->gain_lut_hs[dst_idx_h + ACM_GAIN_LUT_H_DOWN_LENGTH - 1] = hwpq_acm->gain_lut_hs[dst_idx_h];
		hwpq_acm->gain_lut_hs[dst_idx_s + ACM_GAIN_LUT_H_DOWN_LENGTH - 1] = hwpq_acm->gain_lut_hs[dst_idx_s];
	}

	// Copy remain values
	hwpq_acm->acm_enable = swpq_acm->acm_enable ? 1 : 0;
	hwpq_acm->y_gain = swpq_acm->lum_gain;
	hwpq_acm->h_gain = swpq_acm->hue_gain;
	hwpq_acm->s_gain = swpq_acm->sat_gain;
}

static int rockchip_baseparameter_acm_data_v2(uintptr_t conn_state_ptr, struct bp_acm_data *acm_data)
{
	int index;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return -EINVAL;

	if (bp_version < RK_BASEPARAMETER_V2_1) {
		printf("INFO: Cureent version[%d]. Only v2.1 and later versions can support acm info\n",
		       bp_version);
		return -EINVAL;
	}

	rockchip_baseparameter_acm_info_to_acm_data(&bp_info->baseparameter_info_v2.pq_tuning_info.acm_info,
						    acm_data);

	return 0;
}

int rockchip_baseparameter_acm_data_get(uintptr_t conn_state_ptr, struct bp_acm_data *acm_data)
{
	int ret;

	if (!acm_data)
		return -EINVAL;

	if (rockchip_baseparameter_version_v2()) {
		ret = rockchip_baseparameter_acm_data_v2(conn_state_ptr, acm_data);
		if (ret)
			pr_warn("WARN: Failed to parse acm data in v2 baseparameter\n");
	} else {
		pr_warn("WARN: Unsupported baseparameter version[%d] for acm data\n",
			bp_version);
		ret = -EINVAL;
	}

	return ret;
}

static struct bp_overscan_info *rockchip_baseparameter_overscan_info_v2(uintptr_t conn_state_ptr)
{
	int index;

	index = rockchip_baseparameter_disp_info_get(conn_state_ptr);
	if (index < 0)
		return NULL;

	return &bp_info->baseparameter_info_v2.disp_info[index].overscan_info;
}

struct bp_overscan_info *rockchip_baseparameter_overscan_info_get(uintptr_t conn_state_ptr)
{
	struct bp_overscan_info *overscan_info = NULL;

	if (rockchip_baseparameter_version_v2()) {
		overscan_info = rockchip_baseparameter_overscan_info_v2(conn_state_ptr);
		if (!overscan_info)
			pr_warn("WARN: Failed to find overscan info in v2 baseparameter\n");
	} else {
		pr_warn("WARN: Unsupported baseparameter version[%d] for overscan info\n",
			bp_version);
	}

	return overscan_info;
}

void rockchip_baseparameter_select_mode(struct hdmi_edid_data *edid_data,
					struct bp_screen_info *screen_info)
{
	int i;
	struct drm_display_mode mode;

	if (!screen_info) {
		/* define init resolution here */
	} else {
		memset(&mode, 0, sizeof(struct drm_display_mode));

		drm_display_mode_from_bp_display_mode(&mode, &screen_info->mode);
		for (i = 0; i < edid_data->modes; i++) {
			if (drm_mode_match(&mode, &edid_data->mode_buf[i],
					   DRM_MODE_MATCH_TIMINGS |
					   DRM_MODE_MATCH_CLOCK |
					   DRM_MODE_MATCH_FLAGS)) {
				edid_data->preferred_mode = &edid_data->mode_buf[i];

				if (edid_data->mode_buf[i].picture_aspect_ratio)
					break;
			}
		}
	}
}

static int rockchip_baseparameter_get_v1(struct blk_desc *dev_desc,
					 struct disk_partition *part_info)
{
	lbaint_t block_num;
	ulong blks_read;
	u8 *baseparameter_buf;
	int ret;

	block_num = BLOCK_CNT(sizeof(bp_info->baseparameter_info_v1.main), dev_desc);
	baseparameter_buf = memalign(ARCH_DMA_MINALIGN, block_num * dev_desc->blksz);
	if (!baseparameter_buf) {
		pr_err("ERROR: Failed to alloc memory for baseparameter buffer\n");
		return -ENOMEM;
	}

	/* Parse MAIN partition */
	blks_read = blk_dread(dev_desc, part_info->start, block_num, (void *)baseparameter_buf);
	ret = blks_read == block_num ? 0 : -EINVAL;
	if (ret) {
		pr_err("ERROR: Failed to read baseparameter MAIN partition\n");
		goto out;
	}
	memcpy(&bp_info->baseparameter_info_v1.main, baseparameter_buf,
	       sizeof(bp_info->baseparameter_info_v1.main));

	memset(baseparameter_buf, 0, block_num * RK_BLK_SIZE);

	/* Parse AUX partition */
	block_num = BLOCK_CNT(sizeof(bp_info->baseparameter_info_v1.aux), dev_desc);
	blks_read = blk_dread(dev_desc,
			      part_info->start + BLOCK_CNT(BP_V1_DISP_INFO_BLK_SIZE, dev_desc),
			      block_num, (void *)baseparameter_buf);
	ret = blks_read == block_num ? 0 : -EINVAL;
	if (ret) {
		pr_err("ERROR: Failed to read baseparameter AUX partition\n");
		goto out;
	}
	memcpy(&bp_info->baseparameter_info_v1.aux, baseparameter_buf,
	       sizeof(bp_info->baseparameter_info_v1.aux));

out:
	free(baseparameter_buf);
	return ret;
}

static int rockchip_baseparameter_get_v2(struct blk_desc *dev_desc,
					 struct disk_partition *part_info)
{
	lbaint_t block_num;
	ulong blks_read;
	u8 *baseparameter_buf;
	int ret;

	block_num = BLOCK_CNT(sizeof(bp_info->baseparameter_info_v2), dev_desc);
	baseparameter_buf = memalign(ARCH_DMA_MINALIGN, block_num * dev_desc->blksz);
	if (!baseparameter_buf) {
		pr_err("ERROR: Failed to alloc memory for baseparameter buffer\n");
		return -ENOMEM;
	}

	blks_read = blk_dread(dev_desc, part_info->start, block_num, (void *)baseparameter_buf);
	ret = blks_read == block_num ? 0 : -EINVAL;
	if (ret) {
		pr_err("ERROR: Failed to read baseparameter\n");
		goto out;
	}

	memcpy(&bp_info->baseparameter_info_v2, baseparameter_buf,
	       sizeof(bp_info->baseparameter_info_v2));
	if (bp_info->baseparameter_info_v2.major_version != 2 ||
	    strncasecmp(bp_info->baseparameter_info_v2.head_flag, "BASP", 4)) {
		memset(&bp_info->baseparameter_info_v2, 0, sizeof(bp_info->baseparameter_info_v2));
		ret = -EOPNOTSUPP;
	}

out:
	free(baseparameter_buf);
	return ret;
}

static int rockchip_baseparameter_get(struct blk_desc *dev_desc, struct disk_partition *part_info)
{
	int ret = 0;

	ret = rockchip_baseparameter_get_v2(dev_desc, part_info);
	if (!ret) {
		if (bp_info->baseparameter_info_v2.minor_version == 0)
			bp_version = RK_BASEPARAMETER_V2_0;
		else if (bp_info->baseparameter_info_v2.minor_version == 1)
			bp_version = RK_BASEPARAMETER_V2_1;
	} else if (ret == -EOPNOTSUPP) {
		ret = rockchip_baseparameter_get_v1(dev_desc, part_info);
		if (!ret)
			bp_version = RK_BASEPARAMETER_V1_0;
	}

	return ret;
}

int rockchip_baseparameter_init(void)
{
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
	int ret;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		pr_err("ERROR: Failed to find boot device for baseparameter pasing\n");
		return -ENODEV;
	}

	if (part_get_info_by_name(dev_desc, "baseparameter", &part_info) < 0) {
		pr_err("ERROR: Failed to find baseparameter partition\n");
		return -ENOENT;
	}

	bp_info = calloc(1, sizeof(union baseparameter_info));
	if (!bp_info) {
		pr_err("ERROR: Failed to alloc memory for baseparameter\n");
		return -ENOMEM;
	}

	ret = rockchip_baseparameter_get(dev_desc, &part_info);
	if (ret) {
		pr_err("ERROR: Failed to get baseparameter\n");
		return ret;
	}

	return 0;
}
