/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <boot_rkimg.h>
#include <asm/io.h>
#include <dm/device.h>
#include <linux/dw_hdmi.h>
#include <linux/hdmi.h>
#include <linux/media-bus-format.h>
#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "dw_hdmi.h"
#include "rockchip_dw_hdmi.h"

#define HDMI_SEL_LCDC(x, bit)  ((((x) & 1) << bit) | (1 << (16 + bit)))
#define RK3288_GRF_SOC_CON6		0x025C
#define RK3288_HDMI_LCDC_SEL		BIT(4)
#define RK3399_GRF_SOC_CON20		0x6250
#define RK3399_HDMI_LCDC_SEL		BIT(6)

#define RK3228_IO_3V_DOMAIN              ((7 << 4) | (7 << (4 + 16)))
#define RK3328_IO_3V_DOMAIN              (7 << (9 + 16))
#define RK3328_IO_5V_DOMAIN              ((7 << 9) | (3 << (9 + 16)))
#define RK3328_IO_CTRL_BY_HDMI           ((1 << 13) | (1 << (13 + 16)))
#define RK3328_IO_DDC_IN_MSK             ((3 << 10) | (3 << (10 + 16)))
#define RK3228_IO_DDC_IN_MSK             ((3 << 13) | (3 << (13 + 16)))
#define RK3228_GRF_SOC_CON2              0x0408
#define RK3228_GRF_SOC_CON6              0x0418
#define RK3328_GRF_SOC_CON2              0x0408
#define RK3328_GRF_SOC_CON3              0x040c
#define RK3328_GRF_SOC_CON4              0x0410

#define DRM_BASE_MODE(c, hd, hss, hse, ht, vd, vss, vse, vt, vs, f) \
	.clock = (c), \
	.hdisplay = (hd), .hsync_start = (hss), .hsync_end = (hse), \
	.htotal = (ht), .vdisplay = (vd), \
	.vsync_start = (vss), .vsync_end = (vse), .vtotal = (vt), \
	.vscan = (vs), .flags = (f)

static const struct dw_hdmi_mpll_config rockchip_mpll_cfg[] = {
	{
		30666000, {
			{ 0x00b3, 0x0000 },
			{ 0x2153, 0x0000 },
			{ 0x40f3, 0x0000 },
		},
	},  {
		36800000, {
			{ 0x00b3, 0x0000 },
			{ 0x2153, 0x0000 },
			{ 0x40a2, 0x0001 },
		},
	},  {
		46000000, {
			{ 0x00b3, 0x0000 },
			{ 0x2142, 0x0001 },
			{ 0x40a2, 0x0001 },
		},
	},  {
		61333000, {
			{ 0x0072, 0x0001 },
			{ 0x2142, 0x0001 },
			{ 0x40a2, 0x0001 },
		},
	},  {
		73600000, {
			{ 0x0072, 0x0001 },
			{ 0x2142, 0x0001 },
			{ 0x4061, 0x0002 },
		},
	},  {
		92000000, {
			{ 0x0072, 0x0001 },
			{ 0x2145, 0x0002 },
			{ 0x4061, 0x0002 },
		},
	},  {
		122666000, {
			{ 0x0051, 0x0002 },
			{ 0x2145, 0x0002 },
			{ 0x4061, 0x0002 },
		},
	},  {
		147200000, {
			{ 0x0051, 0x0002 },
			{ 0x2145, 0x0002 },
			{ 0x4064, 0x0003 },
		},
	},  {
		184000000, {
			{ 0x0051, 0x0002 },
			{ 0x214c, 0x0003 },
			{ 0x4064, 0x0003 },
		},
	},  {
		226666000, {
			{ 0x0040, 0x0003 },
			{ 0x214c, 0x0003 },
			{ 0x4064, 0x0003 },
		},
	},  {
		272000000, {
			{ 0x0040, 0x0003 },
			{ 0x214c, 0x0003 },
			{ 0x5a64, 0x0003 },
		},
	},  {
		340000000, {
			{ 0x0040, 0x0003 },
			{ 0x3b4c, 0x0003 },
			{ 0x5a64, 0x0003 },
		},
	},  {
		600000000, {
			{ 0x1a40, 0x0003 },
			{ 0x3b4c, 0x0003 },
			{ 0x5a64, 0x0003 },
		},
	},  {
		~0UL, {
			{ 0x0000, 0x0000 },
			{ 0x0000, 0x0000 },
			{ 0x0000, 0x0000 },
		},
	}
};

static const struct dw_hdmi_mpll_config rockchip_mpll_cfg_420[] = {
	{
		30666000, {
			{ 0x00b7, 0x0000 },
			{ 0x2157, 0x0000 },
			{ 0x40f7, 0x0000 },
		},
	},  {
		92000000, {
			{ 0x00b7, 0x0000 },
			{ 0x2143, 0x0001 },
			{ 0x40a3, 0x0001 },
		},
	},  {
		184000000, {
			{ 0x0073, 0x0001 },
			{ 0x2146, 0x0002 },
			{ 0x4062, 0x0002 },
		},
	},  {
		340000000, {
			{ 0x0052, 0x0003 },
			{ 0x214d, 0x0003 },
			{ 0x4065, 0x0003 },
		},
	},  {
		600000000, {
			{ 0x0041, 0x0003 },
			{ 0x3b4d, 0x0003 },
			{ 0x5a65, 0x0003 },
		},
	},  {
		~0UL, {
			{ 0x0000, 0x0000 },
			{ 0x0000, 0x0000 },
			{ 0x0000, 0x0000 },
		},
	}
};

static const struct dw_hdmi_curr_ctrl rockchip_cur_ctr[] = {
	/*      pixelclk    bpp8    bpp10   bpp12 */
	{
		600000000, { 0x0000, 0x0000, 0x0000 },
	},  {
		~0UL,      { 0x0000, 0x0000, 0x0000},
	}
};

static const struct dw_hdmi_phy_config rockchip_phy_config[] = {
	/*pixelclk   symbol   term   vlev*/
	{ 74250000,  0x8009, 0x0004, 0x0272},
	{ 165000000, 0x802b, 0x0004, 0x0209},
	{ 297000000, 0x8039, 0x0005, 0x028d},
	{ 594000000, 0x8039, 0x0000, 0x019d},
	{ ~0UL,	     0x0000, 0x0000, 0x0000}
};

static const struct base_drm_display_mode resolution_white[] = {
	/* 0. vic:2 - 720x480@60Hz */
	{ DRM_BASE_MODE(27000, 720, 736,
			798, 858, 480, 489, 495, 525, 0,
			DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 1. vic:3 - 720x480@60Hz */
	{ DRM_BASE_MODE(27000, 720, 736,
			798, 858, 480, 489, 495, 525, 0,
			DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 2. vic:4 - 1280x720@60Hz */
	{ DRM_BASE_MODE(74250, 1280, 1390,
			1430, 1650, 720, 725, 730, 750, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 3. vic:5 - 1920x1080i@60Hz */
	{ DRM_BASE_MODE(74250, 1920, 2008,
			2052, 2200, 1080, 1084, 1094, 1125, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 4. vic:6 - 720(1440)x480i@60Hz */
	{ DRM_BASE_MODE(13500, 720, 739,
			801, 858, 480, 488, 494, 525, 0,
			DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 5. vic:16 - 1920x1080@60Hz */
	{ DRM_BASE_MODE(148500, 1920, 2008,
			2052, 2200, 1080, 1084, 1089, 1125, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 6. vic:17 - 720x576@50Hz */
	{ DRM_BASE_MODE(27000, 720, 732,
			796, 864, 576, 581, 586, 625, 0,
			DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 7. vic:18 - 720x576@50Hz */
	{ DRM_BASE_MODE(27000, 720, 732,
			796, 864, 576, 581, 586, 625, 0,
			DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 8. vic:19 - 1280x720@50Hz */
	{ DRM_BASE_MODE(74250, 1280, 1720,
			1760, 1980, 720, 725, 730, 750, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 9. vic:20 - 1920x1080i@50Hz */
	{ DRM_BASE_MODE(74250, 1920, 2448,
			2492, 2640, 1080, 1084, 1094, 1125, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 10. vic:21 - 720(1440)x576i@50Hz */
	{ DRM_BASE_MODE(13500, 720, 732,
			795, 864, 576, 580, 586, 625, 0,
			DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 11. vic:31 - 1920x1080@50Hz */
	{ DRM_BASE_MODE(148500, 1920, 2448,
			2492, 2640, 1080, 1084, 1089, 1125, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 12. vic:32 - 1920x1080@24Hz */
	{ DRM_BASE_MODE(74250, 1920, 2558,
			2602, 2750, 1080, 1084, 1089, 1125, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 13. vic:33 - 1920x1080@25Hz */
	{ DRM_BASE_MODE(74250, 1920, 2448,
			2492, 2640, 1080, 1084, 1089, 1125, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 14. vic:34 - 1920x1080@30Hz */
	{ DRM_BASE_MODE(74250, 1920, 2008,
			2052, 2200, 1080, 1084, 1089, 1125, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 15. vic:39 - 1920x1080i@50Hz */
	{ DRM_BASE_MODE(72000, 1920, 1952,
			2120, 2304, 1080, 1126, 1136, 1250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 16. vic:60 - 1280x720@24Hz */
	{ DRM_BASE_MODE(59400, 1280, 3040,
			3080, 3300, 720, 725, 730, 750, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 17. vic:61 - 1280x720@25Hz */
	{ DRM_BASE_MODE(74250, 1280, 3700,
			3740, 3960, 720, 725, 730, 750, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 18. vic:62 - 1280x720@30Hz */
	{ DRM_BASE_MODE(74250, 1280, 3040,
			3080, 3300, 720, 725, 730, 750, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 19. vic:93 - 3840x2160p@24Hz 16:9 */
	{ DRM_BASE_MODE(297000, 3840, 5116,
			5204, 5500, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 20. vic:94 - 3840x2160p@25Hz 16:9 */
	{ DRM_BASE_MODE(297000, 3840, 4896,
			4984, 5280, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 21. vic:95 - 3840x2160p@30Hz 16:9 */
	{ DRM_BASE_MODE(297000, 3840, 4016,
			4104, 4400, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 22. vic:96 - 3840x2160p@50Hz 16:9 */
	{ DRM_BASE_MODE(594000, 3840, 4896,
			4984, 5280, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 23. vic:97 - 3840x2160p@60Hz 16:9 */
	{ DRM_BASE_MODE(594000, 3840, 4016,
			4104, 4400, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 24. vic:98 - 4096x2160p@24Hz 256:135 */
	{ DRM_BASE_MODE(297000, 4096, 5116,
			5204, 5500, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 25. vic:99 - 4096x2160p@25Hz 256:135 */
	{ DRM_BASE_MODE(297000, 4096, 5064,
			5152, 5280, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 26. vic:100 - 4096x2160p@30Hz 256:135 */
	{ DRM_BASE_MODE(297000, 4096, 4184,
			4272, 4400, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 27. vic:101 - 4096x2160p@50Hz 256:135 */
	{ DRM_BASE_MODE(594000, 4096, 5064,
			5152, 5280, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 28. vic:102 - 4096x2160p@60Hz 256:135 */
	{ DRM_BASE_MODE(594000, 4096, 4184,
			4272, 4400, 2160, 2168, 2178, 2250, 0,
			DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
};

static bool drm_mode_equal(const struct base_drm_display_mode *mode1,
			   const struct drm_display_mode *mode2)
{
	unsigned int flags_mask =
		DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_PHSYNC |
		DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC |
		DRM_MODE_FLAG_NVSYNC;

	if (mode1->clock == mode2->clock &&
	    mode1->hdisplay == mode2->hdisplay &&
	    mode1->hsync_start == mode2->hsync_start &&
	    mode1->hsync_end == mode2->hsync_end &&
	    mode1->htotal == mode2->htotal &&
	    mode1->vdisplay == mode2->vdisplay &&
	    mode1->vsync_start == mode2->vsync_start &&
	    mode1->vsync_end == mode2->vsync_end &&
	    mode1->vtotal == mode2->vtotal &&
	    (mode1->flags & flags_mask) == (mode2->flags & flags_mask)) {
		return true;
	}

	return false;
}

/**
 * drm_mode_sort - sort mode list
 * @edid_data: modes structures to sort
 *
 * Sort @edid_data by favorability, moving good modes to the head of the list.
 */
void drm_mode_sort(struct hdmi_edid_data *edid_data)
{
	struct drm_display_mode *a, *b;
	struct drm_display_mode c;
	int diff, i, j;

	for (i = 0; i < (edid_data->modes - 1); i++) {
		a = &edid_data->mode_buf[i];
		for (j = i + 1; j < edid_data->modes; j++) {
			b = &edid_data->mode_buf[j];
			diff = ((b->type & DRM_MODE_TYPE_PREFERRED) != 0) -
				((a->type & DRM_MODE_TYPE_PREFERRED) != 0);
			if (diff) {
				if (diff > 0) {
					c = *a;
					*a = *b;
					*b = c;
				}
				continue;
			}

			diff = b->hdisplay * b->vdisplay
				- a->hdisplay * a->vdisplay;
			if (diff) {
				if (diff > 0) {
					c = *a;
					*a = *b;
					*b = c;
				}
				continue;
			}

			diff = b->vrefresh - a->vrefresh;
			if (diff) {
				if (diff > 0) {
					c = *a;
					*a = *b;
					*b = c;
				}
				continue;
			}

			diff = b->clock - a->clock;
			if (diff > 0) {
				c = *a;
				*a = *b;
				*b = c;
			}
		}
	}
	edid_data->preferred_mode = &edid_data->mode_buf[0];
}

/**
 * drm_mode_prune_invalid - remove invalid modes from mode list
 * @edid_data: structure store mode list
 * Returns:
 * Number of valid modes.
 */
int drm_mode_prune_invalid(struct hdmi_edid_data *edid_data)
{
	int i, j;
	int num = edid_data->modes;
	int len = sizeof(struct drm_display_mode);
	struct drm_display_mode *mode_buf = edid_data->mode_buf;

	for (i = 0; i < num; i++) {
		if (mode_buf[i].invalid) {
			/* If mode is invalid, delete it. */
			for (j = i; j < num - 1; j++)
				memcpy(&mode_buf[j], &mode_buf[j + 1], len);

			num--;
			i--;
		}
	}
	/* Clear redundant modes of mode_buf. */
	memset(&mode_buf[num], 0, len * (edid_data->modes - num));

	edid_data->modes = num;
	return num;
}

/**
 * drm_rk_filter_whitelist - mark modes out of white list from mode list
 * @edid_data: structure store mode list
 */
void drm_rk_filter_whitelist(struct hdmi_edid_data *edid_data)
{
	int i, j, white_len;

	if (sizeof(resolution_white)) {
		white_len = sizeof(resolution_white) /
			sizeof(resolution_white[0]);
		for (i = 0; i < edid_data->modes; i++) {
			for (j = 0; j < white_len; j++) {
				if (drm_mode_equal(&resolution_white[j],
						   &edid_data->mode_buf[i]))
					break;
			}

			if (j == white_len)
				edid_data->mode_buf[i].invalid = true;
		}
	}
}

void drm_rk_select_mode(struct hdmi_edid_data *edid_data,
			struct base_screen_info *screen_info)
{
	int i;
	const struct base_drm_display_mode *base_mode;

	if (!screen_info) {
		/* define init resolution here */
	} else {
		base_mode = &screen_info->mode;
		for (i = 0; i < edid_data->modes; i++) {
			if (drm_mode_equal(base_mode,
					   &edid_data->mode_buf[i])) {
				edid_data->preferred_mode =
					&edid_data->mode_buf[i];
				break;
			}
		}
	}
}

static unsigned int drm_rk_select_color(struct hdmi_edid_data *edid_data,
					struct base_screen_info *screen_info,
					enum dw_hdmi_devtype dev_type)
{
	struct drm_display_info *info = &edid_data->display_info;
	struct drm_display_mode *mode = edid_data->preferred_mode;
	int max_tmds_clock = info->max_tmds_clock;
	bool support_dc = false;
	bool mode_420 = drm_mode_is_420(info, mode);
	unsigned int color_depth = 8;
	unsigned int base_color = DRM_HDMI_OUTPUT_YCBCR444;
	unsigned int color_format = DRM_HDMI_OUTPUT_DEFAULT_RGB;
	unsigned long tmdsclock, pixclock = mode->clock;

	if (screen_info)
		base_color = screen_info->format;

	switch (base_color) {
	case DRM_HDMI_OUTPUT_YCBCR_HQ:
		if (info->color_formats & DRM_COLOR_FORMAT_YCRCB444)
			color_format = DRM_HDMI_OUTPUT_YCBCR444;
		else if (info->color_formats & DRM_COLOR_FORMAT_YCRCB422)
			color_format = DRM_HDMI_OUTPUT_YCBCR422;
		else if (mode_420)
			color_format = DRM_HDMI_OUTPUT_YCBCR420;
		break;
	case DRM_HDMI_OUTPUT_YCBCR_LQ:
		if (mode_420)
			color_format = DRM_HDMI_OUTPUT_YCBCR420;
		else if (info->color_formats & DRM_COLOR_FORMAT_YCRCB422)
			color_format = DRM_HDMI_OUTPUT_YCBCR422;
		else if (info->color_formats & DRM_COLOR_FORMAT_YCRCB444)
			color_format = DRM_HDMI_OUTPUT_YCBCR444;
		break;
	case DRM_HDMI_OUTPUT_YCBCR420:
		if (mode_420)
			color_format = DRM_HDMI_OUTPUT_YCBCR420;
		break;
	case DRM_HDMI_OUTPUT_YCBCR422:
		if (info->color_formats & DRM_COLOR_FORMAT_YCRCB422)
			color_format = DRM_HDMI_OUTPUT_YCBCR422;
		break;
	case DRM_HDMI_OUTPUT_YCBCR444:
		if (info->color_formats & DRM_COLOR_FORMAT_YCRCB444)
			color_format = DRM_HDMI_OUTPUT_YCBCR444;
		break;
	case DRM_HDMI_OUTPUT_DEFAULT_RGB:
	default:
		break;
	}

	if (color_format == DRM_HDMI_OUTPUT_DEFAULT_RGB &&
	    info->edid_hdmi_dc_modes & DRM_EDID_HDMI_DC_30)
		support_dc = true;
	if (color_format == DRM_HDMI_OUTPUT_YCBCR444 &&
	    (info->edid_hdmi_dc_modes &
	     (DRM_EDID_HDMI_DC_Y444 | DRM_EDID_HDMI_DC_30)))
		support_dc = true;
	if (color_format == DRM_HDMI_OUTPUT_YCBCR422)
		support_dc = true;
	if (color_format == DRM_HDMI_OUTPUT_YCBCR420 &&
	    info->hdmi.y420_dc_modes & DRM_EDID_YCBCR420_DC_30)
		support_dc = true;

	if (mode->flags & DRM_MODE_FLAG_DBLCLK)
		pixclock *= 2;

	if (screen_info && screen_info->depth == 10)
		color_depth = screen_info->depth;

	if (color_format == DRM_HDMI_OUTPUT_YCBCR422 || color_depth == 8)
		tmdsclock = pixclock;
	else
		tmdsclock = pixclock * color_depth / 8;

	if (color_format == DRM_HDMI_OUTPUT_YCBCR420)
		tmdsclock /= 2;

	if (!max_tmds_clock)
		max_tmds_clock = 340000;

	switch (dev_type) {
	case RK3368_HDMI:
		max_tmds_clock = min(max_tmds_clock, 340000);
		break;
	case RK3328_HDMI:
	case RK3228_HDMI:
		max_tmds_clock = min(max_tmds_clock, 371250);
		break;
	default:
		max_tmds_clock = min(max_tmds_clock, 594000);
		break;
	}

	if (tmdsclock > max_tmds_clock) {
		if (max_tmds_clock >= 594000) {
			color_depth = 8;
		} else if (max_tmds_clock > 340000) {
			if (drm_mode_is_420(info, mode))
				color_format = DRM_HDMI_OUTPUT_YCBCR420;
		} else {
			color_depth = 8;
			if (drm_mode_is_420(info, mode))
				color_format = DRM_HDMI_OUTPUT_YCBCR420;
		}
	}

	if (color_depth > 8 && support_dc) {
		if (dev_type == RK3288_HDMI)
			return MEDIA_BUS_FMT_RGB101010_1X30;
		switch (color_format) {
		case DRM_HDMI_OUTPUT_YCBCR444:
			return MEDIA_BUS_FMT_YUV10_1X30;
		case DRM_HDMI_OUTPUT_YCBCR422:
			return MEDIA_BUS_FMT_UYVY10_1X20;
		case DRM_HDMI_OUTPUT_YCBCR420:
			return MEDIA_BUS_FMT_UYYVYY10_0_5X30;
		default:
			return MEDIA_BUS_FMT_RGB101010_1X30;
		}
	} else {
		if (dev_type == RK3288_HDMI)
			return MEDIA_BUS_FMT_RGB888_1X24;
		switch (color_format) {
		case DRM_HDMI_OUTPUT_YCBCR444:
			return MEDIA_BUS_FMT_YUV8_1X24;
		case DRM_HDMI_OUTPUT_YCBCR422:
			return MEDIA_BUS_FMT_UYVY8_1X16;
		case DRM_HDMI_OUTPUT_YCBCR420:
			return MEDIA_BUS_FMT_UYYVYY8_0_5X24;
		default:
			return MEDIA_BUS_FMT_RGB888_1X24;
		}
	}
}

void drm_rk_selete_output(struct hdmi_edid_data *edid_data,
			  unsigned int *bus_format,
			  struct overscan *overscan,
			  enum dw_hdmi_devtype dev_type)
{
	int ret, i, screen_size;
	struct base_disp_info base_parameter;
	const struct base_overscan *scan;
	struct base_screen_info *screen_info = NULL;
	int max_scan = 100;
	int min_scan = 51;
	struct blk_desc *dev_desc;
	disk_partition_t part_info;
	char baseparameter_buf[8 * RK_BLK_SIZE] __aligned(ARCH_DMA_MINALIGN);

	overscan->left_margin = max_scan;
	overscan->right_margin = max_scan;
	overscan->top_margin = max_scan;
	overscan->bottom_margin = max_scan;

	if (dev_type == RK3288_HDMI)
		*bus_format = MEDIA_BUS_FMT_RGB888_1X24;
	else
		*bus_format = MEDIA_BUS_FMT_YUV8_1X24;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return;
	}

	if (part_get_info_by_name(dev_desc, "baseparameter", &part_info) < 0) {
		printf("Could not find baseparameter partition\n");
		return;
	}

	ret = blk_dread(dev_desc, part_info.start, 1,
			(void *)baseparameter_buf);
	if (ret < 0) {
		printf("read baseparameter failed\n");
		return;
	}

	memcpy(&base_parameter, baseparameter_buf, sizeof(base_parameter));
	scan = &base_parameter.scan;

	if (scan->leftscale < min_scan && scan->leftscale > 0)
		overscan->left_margin = min_scan;
	else if (scan->leftscale < max_scan && scan->leftscale > 0)
		overscan->left_margin = scan->leftscale;

	if (scan->rightscale < min_scan && scan->rightscale > 0)
		overscan->right_margin = min_scan;
	else if (scan->rightscale < max_scan && scan->rightscale > 0)
		overscan->right_margin = scan->rightscale;

	if (scan->topscale < min_scan && scan->topscale > 0)
		overscan->top_margin = min_scan;
	else if (scan->topscale < max_scan && scan->topscale > 0)
		overscan->top_margin = scan->topscale;

	if (scan->bottomscale < min_scan && scan->bottomscale > 0)
		overscan->bottom_margin = min_scan;
	else if (scan->bottomscale < max_scan && scan->bottomscale > 0)
		overscan->bottom_margin = scan->bottomscale;

	screen_size = sizeof(base_parameter.screen_list) /
		sizeof(base_parameter.screen_list[0]);

	for (i = 0; i < screen_size; i++) {
		if (base_parameter.screen_list[i].type ==
		    DRM_MODE_CONNECTOR_HDMIA) {
			screen_info = &base_parameter.screen_list[i];
			break;
		}
	}

	if (screen_info)
		printf("base_parameter.mode:%dx%d\n",
		       screen_info->mode.hdisplay,
		       screen_info->mode.vdisplay);
	drm_rk_select_mode(edid_data, screen_info);

	*bus_format = drm_rk_select_color(edid_data, screen_info,
					  dev_type);
}

void inno_dw_hdmi_set_domain(void *grf, int status)
{
	if (status)
		writel(RK3328_IO_5V_DOMAIN, grf + RK3328_GRF_SOC_CON4);
	else
		writel(RK3328_IO_3V_DOMAIN, grf + RK3328_GRF_SOC_CON4);
}

void dw_hdmi_set_iomux(void *grf, int dev_type)
{
	switch (dev_type) {
	case RK3328_HDMI:
		writel(RK3328_IO_DDC_IN_MSK, grf + RK3328_GRF_SOC_CON2);
		writel(RK3328_IO_CTRL_BY_HDMI, grf + RK3328_GRF_SOC_CON3);
		break;
	case RK3228_HDMI:
		writel(RK3228_IO_3V_DOMAIN, grf + RK3228_GRF_SOC_CON6);
		writel(RK3228_IO_DDC_IN_MSK, grf + RK3228_GRF_SOC_CON2);
		break;
	default:
		break;
	}
}

static const struct dw_hdmi_phy_ops inno_dw_hdmi_phy_ops = {
	.init = inno_dw_hdmi_phy_init,
	.disable = inno_dw_hdmi_phy_disable,
	.read_hpd = inno_dw_hdmi_phy_read_hpd,
	.mode_valid = inno_dw_hdmi_mode_valid,
};

static const struct rockchip_connector_funcs rockchip_dw_hdmi_funcs = {
	.init = rockchip_dw_hdmi_init,
	.deinit = rockchip_dw_hdmi_deinit,
	.prepare = rockchip_dw_hdmi_prepare,
	.enable = rockchip_dw_hdmi_enable,
	.disable = rockchip_dw_hdmi_disable,
	.get_timing = rockchip_dw_hdmi_get_timing,
	.detect = rockchip_dw_hdmi_detect,
	.get_edid = rockchip_dw_hdmi_get_edid,
};

const struct dw_hdmi_plat_data rk3288_hdmi_drv_data = {
	.vop_sel_bit = 4,
	.grf_vop_sel_reg = RK3288_GRF_SOC_CON6,
	.mpll_cfg   = rockchip_mpll_cfg,
	.cur_ctr    = rockchip_cur_ctr,
	.phy_config = rockchip_phy_config,
	.dev_type   = RK3288_HDMI,
};

const struct dw_hdmi_plat_data rk3328_hdmi_drv_data = {
	.vop_sel_bit = 0,
	.grf_vop_sel_reg = 0,
	.phy_ops    = &inno_dw_hdmi_phy_ops,
	.phy_name   = "inno_dw_hdmi_phy2",
	.dev_type   = RK3328_HDMI,
};

const struct dw_hdmi_plat_data rk3228_hdmi_drv_data = {
	.vop_sel_bit = 0,
	.grf_vop_sel_reg = 0,
	.phy_ops    = &inno_dw_hdmi_phy_ops,
	.phy_name   = "inno_dw_hdmi_phy",
	.dev_type   = RK3228_HDMI,
};

const struct dw_hdmi_plat_data rk3368_hdmi_drv_data = {
	.mpll_cfg   = rockchip_mpll_cfg,
	.cur_ctr    = rockchip_cur_ctr,
	.phy_config = rockchip_phy_config,
	.mpll_cfg_420 = rockchip_mpll_cfg_420,
	.dev_type   = RK3368_HDMI,
};

const struct dw_hdmi_plat_data rk3399_hdmi_drv_data = {
	.vop_sel_bit = 6,
	.grf_vop_sel_reg = RK3399_GRF_SOC_CON20,
	.mpll_cfg   = rockchip_mpll_cfg,
	.cur_ctr    = rockchip_cur_ctr,
	.phy_config = rockchip_phy_config,
	.mpll_cfg_420 = rockchip_mpll_cfg_420,
	.dev_type   = RK3399_HDMI,
};

static int rockchip_dw_hdmi_probe(struct udevice *dev)
{
	return 0;
}

static const struct rockchip_connector rk3399_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3399_hdmi_drv_data,
};

static const struct rockchip_connector rk3368_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3368_hdmi_drv_data,
};

static const struct rockchip_connector rk3288_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3288_hdmi_drv_data,
};

static const struct rockchip_connector rk3328_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3328_hdmi_drv_data,
};

static const struct rockchip_connector rk3228_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3228_hdmi_drv_data,
};

static const struct udevice_id rockchip_dw_hdmi_ids[] = {
	{
	 .compatible = "rockchip,rk3399-dw-hdmi",
	 .data = (ulong)&rk3399_dw_hdmi_data,
	}, {
	 .compatible = "rockchip,rk3368-dw-hdmi",
	 .data = (ulong)&rk3368_dw_hdmi_data,
	}, {
	 .compatible = "rockchip,rk3288-dw-hdmi",
	 .data = (ulong)&rk3288_dw_hdmi_data,
	}, {
	 .compatible = "rockchip,rk3328-dw-hdmi",
	 .data = (ulong)&rk3328_dw_hdmi_data,
	}, {
	 .compatible = "rockchip,rk3128-inno-hdmi",
	 .data = (ulong)&rk3228_dw_hdmi_data,
	}, {
	 .compatible = "rockchip,rk3228-dw-hdmi",
	 .data = (ulong)&rk3228_dw_hdmi_data,
	}, {}
};

U_BOOT_DRIVER(rockchip_dw_hdmi) = {
	.name = "rockchip_dw_hdmi",
	.id = UCLASS_DISPLAY,
	.of_match = rockchip_dw_hdmi_ids,
	.probe	= rockchip_dw_hdmi_probe,
};
