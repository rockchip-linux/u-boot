/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm/device.h>
#include <linux/dw_hdmi.h>
#include <linux/hdmi.h>
#include <errno.h>
#include <asm/arch/rkplat.h>
#include "rockchip_dw_hdmi.h"
#include "../rockchip_display.h"
#include "../rockchip_crtc.h"
#include "../rockchip_connector.h"

#define HDMI_SEL_LCDC(x, bit)  ((((x) & 1) << bit) | (1 << (16 + bit)))
#define RK3288_GRF_SOC_CON6		0x025C
#define RK3288_HDMI_LCDC_SEL		BIT(4)
#define RK3399_GRF_SOC_CON20		0x6250
#define RK3399_HDMI_LCDC_SEL		BIT(6)

#define RK3328_IO_3V_DOMAIN              (7 << (9 + 16))
#define RK3328_IO_5V_DOMAIN              ((7 << 9) | (3 << (9 + 16)))
#define RK3328_GRF_SOC_CON4              0x0410

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

static const struct drm_display_mode resolution_white[] = {
	/* 0. vic:2 - 720x480@60Hz */
	{ DRM_MODE(27000, 720, 736,
		   798, 858, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 1. vic:3 - 720x480@60Hz */
	{ DRM_MODE(27000, 720, 736,
		   798, 858, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 2. vic:4 - 1280x720@60Hz */
	{ DRM_MODE(74250, 1280, 1390,
		   1430, 1650, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 3. vic:5 - 1920x1080i@60Hz */
	{ DRM_MODE(74250, 1920, 2008,
		   2052, 2200, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 4. vic:6 - 720(1440)x480i@60Hz */
	{ DRM_MODE(13500, 720, 739,
		   801, 858, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 5. vic:16 - 1920x1080@60Hz */
	{ DRM_MODE(148500, 1920, 2008,
		   2052, 2200, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 6. vic:17 - 720x576@50Hz */
	{ DRM_MODE(27000, 720, 732,
		   796, 864, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 7. vic:18 - 720x576@50Hz */
	{ DRM_MODE(27000, 720, 732,
		   796, 864, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 8. vic:19 - 1280x720@50Hz */
	{ DRM_MODE(74250, 1280, 1720,
		   1760, 1980, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 9. vic:20 - 1920x1080i@50Hz */
	{ DRM_MODE(74250, 1920, 2448,
		   2492, 2640, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 10. vic:21 - 720(1440)x576i@50Hz */
	{ DRM_MODE(13500, 720, 732,
		   795, 864, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 11. vic:31 - 1920x1080@50Hz */
	{ DRM_MODE(148500, 1920, 2448,
		   2492, 2640, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 12. vic:32 - 1920x1080@24Hz */
	{ DRM_MODE(74250, 1920, 2558,
		   2602, 2750, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 13. vic:33 - 1920x1080@25Hz */
	{ DRM_MODE(74250, 1920, 2448,
		   2492, 2640, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 14. vic:34 - 1920x1080@30Hz */
	{ DRM_MODE(74250, 1920, 2008,
		   2052, 2200, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 15. vic:39 - 1920x1080i@50Hz */
	{ DRM_MODE(72000, 1920, 1952,
		   2120, 2304, 1080, 1126, 1136, 1250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 16. vic:60 - 1280x720@24Hz */
	{ DRM_MODE(59400, 1280, 3040,
		   3080, 3300, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 17. vic:61 - 1280x720@25Hz */
	{ DRM_MODE(74250, 1280, 3700,
		   3740, 3960, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 18. vic:62 - 1280x720@30Hz */
	{ DRM_MODE(74250, 1280, 3040,
		   3080, 3300, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 19. vic:93 - 3840x2160p@24Hz 16:9 */
	{ DRM_MODE(297000, 3840, 5116,
		5204, 5500, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 20. vic:94 - 3840x2160p@25Hz 16:9 */
	{ DRM_MODE(297000, 3840, 4896,
		4984, 5280, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 21. vic:95 - 3840x2160p@30Hz 16:9 */
	{ DRM_MODE(297000, 3840, 4016,
		4104, 4400, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 22. vic:96 - 3840x2160p@50Hz 16:9 */
	{ DRM_MODE(594000, 3840, 4896,
		4984, 5280, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 23. vic:97 - 3840x2160p@60Hz 16:9 */
	{ DRM_MODE(594000, 3840, 4016,
		4104, 4400, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 24. vic:98 - 4096x2160p@24Hz 256:135 */
	{ DRM_MODE(297000, 4096, 5116,
		5204, 5500, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 25. vic:99 - 4096x2160p@25Hz 256:135 */
	{ DRM_MODE(297000, 4096, 5064,
		5152, 5280, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 26. vic:100 - 4096x2160p@30Hz 256:135 */
	{ DRM_MODE(297000, 4096, 4184,
		4272, 4400, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 27. vic:101 - 4096x2160p@50Hz 256:135 */
	{ DRM_MODE(594000, 4096, 5064,
		5152, 5280, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
	/* 28. vic:102 - 4096x2160p@60Hz 256:135 */
	{ DRM_MODE(594000, 4096, 4184,
		4272, 4400, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135, },
};

static int drm_mode_equal(struct drm_display_mode *mode1,
			  struct drm_display_mode *mode2)
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

int drm_rk_find_best_mode(struct hdmi_edid_data *edid_data)
{
	int i, j, white_len;
	struct file_base_paramer base_paramer;
	const disk_partition_t *ptn_baseparamer;
	char baseparamer_buf[8 * RK_BLK_SIZE] __attribute__((aligned(ARCH_DMA_MINALIGN)));
	struct drm_display_mode *base_mode = &base_paramer.main.mode;

	ptn_baseparamer = get_disk_partition("baseparamer");
	if (ptn_baseparamer) {
		if (StorageReadLba(ptn_baseparamer->start, baseparamer_buf, 8) < 0)
			printf("func: %s; LINE: %d\n", __func__, __LINE__);
		else
			memcpy(&base_paramer, baseparamer_buf, sizeof(base_paramer));
	}

	if (base_mode->hdisplay == 0 || base_mode->hdisplay == 0) {
		/* define init resolution here */
	} else {
		for (i = 0; i < edid_data->modes; i++) {
			if (drm_mode_equal(base_mode, &edid_data->mode_buf[i])) {
				edid_data->preferred_mode = &edid_data->mode_buf[i];
				break;
			}
		}
	}

	if (!sizeof(resolution_white))
		return true;

	white_len = sizeof(resolution_white) / sizeof(resolution_white[0]);
	for (j = white_len - 1; j >= 0; j--) {
		if (drm_mode_equal(edid_data->preferred_mode, &resolution_white[j]))
			return true;
	}

	for (j = (white_len - 1); j >= 0; j--) {
		for (i = 0; i < edid_data->modes; i++) {
			if (drm_mode_equal(&resolution_white[j], &edid_data->mode_buf[i])) {
				edid_data->preferred_mode = &edid_data->mode_buf[i];
				return true;
			}
		}
	}

	printf("func: %s return false\n", __func__);
	return false;
}

void inno_dw_hdmi_set_domain(int status)
{
	if (status)
		grf_writel(RK3328_IO_5V_DOMAIN, RK3328_GRF_SOC_CON4);
	else
		grf_writel(RK3328_IO_3V_DOMAIN, RK3328_GRF_SOC_CON4);
}

static const struct dw_hdmi_phy_ops inno_dw_hdmi_phy_ops = {
	.init = inno_dw_hdmi_phy_init,
	.disable = inno_dw_hdmi_phy_disable,
	.read_hpd = inno_dw_hdmi_phy_read_hpd,
};

const struct rockchip_connector_funcs rockchip_dw_hdmi_funcs = {
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
	.dev_type   = RK3288_HDMI,
};

const struct dw_hdmi_plat_data rk3328_hdmi_drv_data = {
        //.mode_valid = dw_hdmi_rockchip_mode_valid,
	.vop_sel_bit = 0,
	.grf_vop_sel_reg = 0,
	.phy_ops    = &inno_dw_hdmi_phy_ops,
	.phy_name   = "inno_dw_hdmi_phy2",
	.dev_type   = RK3328_HDMI,
};

const struct dw_hdmi_plat_data rk3399_hdmi_drv_data = {
	.vop_sel_bit = 6,
	.grf_vop_sel_reg = RK3399_GRF_SOC_CON20,
	.mpll_cfg   = rockchip_mpll_cfg,
	.cur_ctr    = rockchip_cur_ctr,
	.phy_config = rockchip_phy_config,
	.dev_type   = RK3399_HDMI,
};

static const struct rockchip_connector rk3399_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3399_hdmi_drv_data,
};

static const struct rockchip_connector rk3288_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3288_hdmi_drv_data,
};

static const struct rockchip_connector rk3328_dw_hdmi_data = {
	.funcs = &rockchip_dw_hdmi_funcs,
	.data = &rk3328_hdmi_drv_data,
};

static int rockchip_dw_hdmi_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id rockchip_dw_hdmi_ids[] = {
	{
	 .compatible = "rockchip,rk3399-dw-hdmi",
	 .data = (ulong)&rk3399_dw_hdmi_data,
	}, {
	 .compatible = "rockchip,rk3288-dw-hdmi",
	 .data = (ulong)&rk3288_dw_hdmi_data,
	}, {
	 .compatible = "rockchip,rk3328-dw-hdmi",
	 .data = (ulong)&rk3288_dw_hdmi_data,
	}, {}
};

U_BOOT_DRIVER(rockchip_dw_hdmi) = {
	.name = "rockchip_dw_hdmi",
	.id = UCLASS_DISPLAY,
	.of_match = rockchip_dw_hdmi_ids,
	.probe	= rockchip_dw_hdmi_probe,
};
