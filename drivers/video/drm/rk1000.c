/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <edid.h>
#include <i2c.h>
#include <lcd.h>
#include <asm/arch/rkplat.h>
#include <asm/io.h>
#include <linux/fb.h>
#include <../board/rockchip/common/config.h>
#include "../rockchip_display.h"
#include "../rockchip_crtc.h"
#include "../rockchip_connector.h"
#include "../rockchip_panel.h"

#define MAX_I2C_RETRY  3
#define I2C_ADDRESS    0x40
#define TV_I2C_ADDRESS 0x42

#if defined(CONFIG_OF_LIBFDT)
struct fdt_gpio_state rk1000_rst_gpios;
#endif

static int drm_rk1000_write_reg(int i2c_addr, int reg, uchar *data, uint len)
{
	int	i;
	int	retval = -1;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (!i2c_write(i2c_addr, reg, 1, data, len)) {
			retval = 0;
			goto exit;
		}
		udelay(1000);
	}

exit:
	if (retval)
		printf("%s: failed to write register %#x\n", __func__, reg);
	return retval;
}

static int rk1000_parse_dt(const void* blob)
{
	int node;

	node = fdt_node_offset_by_compatible(blob, 0, "rockchip,rk1000-ctl");
	if (node < 0) {
		debug("can't find dts node for rk1000\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		debug("device rk1000 is disabled\n");
		return -1;
	}

	fdtdec_decode_gpio(blob, node, "reset-gpios", &rk1000_rst_gpios);

	return 0;
}

void drm_rk1000_selete_output(struct overscan *overscan,
			      struct drm_display_mode *mode)
{
	int ret, i, screen_size;
	struct base_screen_info *screen_info = NULL;
	struct base_disp_info base_parameter;
	const struct base_overscan *scan;
	const disk_partition_t *ptn_baseparameter;
	char baseparameter_buf[8 * RK_BLK_SIZE] __aligned(ARCH_DMA_MINALIGN);
	int max_scan = 100;
	int min_scan = 50;

	overscan->left_margin = max_scan;
	overscan->right_margin = max_scan;
	overscan->top_margin = max_scan;
	overscan->bottom_margin = max_scan;

	ptn_baseparameter = get_disk_partition("baseparameter");
	if (!ptn_baseparameter) {
		printf("%s; fail get baseparameter\n", __func__);
		return;
	}
	ret = StorageReadLba(ptn_baseparameter->start, baseparameter_buf, 8);
	if (ret < 0) {
		printf("%s; fail read baseparameter\n", __func__);
		return;
	}

	memcpy(&base_parameter, baseparameter_buf, sizeof(base_parameter));
	scan = &base_parameter.scan;

	screen_size = sizeof(base_parameter.screen_list) /
		sizeof(base_parameter.screen_list[0]);

	for (i = 0; i < screen_size; i++) {
		if (base_parameter.screen_list[i].type ==
		    DRM_MODE_CONNECTOR_TV) {
			screen_info = &base_parameter.screen_list[i];
			break;
		}
	}

	if (scan->leftscale < min_scan && scan->leftscale > 0)
		overscan->left_margin = min_scan;
	else if (scan->leftscale < max_scan)
		overscan->left_margin = scan->leftscale;

	if (scan->rightscale < min_scan && scan->rightscale > 0)
		overscan->right_margin = min_scan;
	else if (scan->rightscale < max_scan)
		overscan->right_margin = scan->rightscale;

	if (scan->topscale < min_scan && scan->topscale > 0)
		overscan->top_margin = min_scan;
	else if (scan->topscale < max_scan)
		overscan->top_margin = scan->topscale;

	if (scan->bottomscale < min_scan && scan->bottomscale > 0)
		overscan->bottom_margin = min_scan;
	else if (scan->bottomscale < max_scan)
		overscan->bottom_margin = scan->bottomscale;

	if (screen_info &&
	    (screen_info->mode.hdisplay == 720 &&
	     screen_info->mode.vdisplay == 576 &&
	     screen_info->mode.hsync_end == 738)) {
		mode->hdisplay = 720;
		mode->hsync_start = 732;
		mode->hsync_end = 738;
		mode->htotal = 864;
		mode->vdisplay = 576;
		mode->vsync_start = 582;
		mode->vsync_end = 588;
		mode->vtotal = 625;
		mode->clock = 27000;
	} else if (screen_info &&
		   (screen_info->mode.hdisplay == 720 &&
		    screen_info->mode.hsync_end == 742 &&
		    screen_info->mode.vdisplay == 480)) {
		mode->hdisplay = 720;
		mode->hsync_start = 736;
		mode->hsync_end = 742;
		mode->htotal = 858;
		mode->vdisplay = 480;
		mode->vsync_start = 486;
		mode->vsync_end = 492;
		mode->vtotal = 529;
		mode->clock = 27000;
	} else {
		mode->hdisplay = 720;
		mode->hsync_start = 732;
		mode->hsync_end = 738;
		mode->htotal = 864;
		mode->vdisplay = 576;
		mode->vsync_start = 582;
		mode->vsync_end = 588;
		mode->vtotal = 625;
		mode->clock = 27000;
	}

	mode->flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC;
	if (screen_info)
		printf("base_parameter.mode:%dx%d\n",
		       screen_info->mode.hdisplay,
		       screen_info->mode.vdisplay);
}

static int drm_rk1000_tve_init(struct display_state *state)
{
	int bus_num = -1;

#if defined(CONFIG_OF_LIBFDT)
	debug("rk fb parse dt start.\n");
	int ret = rk1000_parse_dt(gd->fdt_blob);
	if (ret < 0){
		printf("parse rk1000 dts error!\n");
		return -1;
	}
#endif

#ifdef CONFIG_RKCHIP_RK3368
	bus_num = 1;
#endif

#ifdef CONFIG_RKCHIP_RK3288
	bus_num = 4;
#endif
	/* init i2c */
	if (bus_num < 0)
		return -1;
	i2c_set_bus_num(bus_num);
	i2c_init(200*1000, 1);

	/* for rk3368 */
#ifdef  CONFIG_RKCHIP_RK3368
	grf_writel(0xffff1500, GRF_GPIO2C_IOMUX);
	cru_writel(cru_readl(0x16c) | 0x80008000, 0x16c);
#endif
	/* for rk3288 */
#ifdef	CONFIG_RKCHIP_RK3288
	grf_writel(0xffff1a40, GRF_SOC_CON7);
	grf_writel(grf_readl(GRF_GPIO7CL_IOMUX) | (1<<4) | (1<<8) | (1<<20) |
		   (1<<24), GRF_GPIO7CL_IOMUX);
	grf_writel(grf_readl(GRF_GPIO6B_IOMUX) | (1<<0) | (1<<16),
		   GRF_GPIO6B_IOMUX);
	writel(0x00071f1f, 0xff890008);
#endif
	gpio_direction_output((GPIO_BANK0 | GPIO_A1), 0);
	gpio_direction_output(rk1000_rst_gpios.gpio, !rk1000_rst_gpios.flags);
	mdelay(5);

	gpio_direction_output((GPIO_BANK0 | GPIO_A1), 1);
	gpio_direction_output(rk1000_rst_gpios.gpio, rk1000_rst_gpios.flags);

	return 0;
}

static void rk1000_tve_deinit(struct display_state *state)
{
	char data[1] = {0x07};

	/* rk1000 power down output dac */
	drm_rk1000_write_reg(0x42, 0x03, (uchar*)data, 1);
}

static int rk1000_tve_prepare(struct display_state *state)
{
	return 0;
}

static int rk1000_tve_unprepare(struct display_state *state)
{
	return 0;
}

static int rk1000_tve_enable(struct display_state *state)
{
	unsigned char tv_encoder_regs_pal[] = {0x06, 0x00, 0x00, 0x03, 0x00,
					       0x00};
	unsigned char tv_encoder_control_regs_pal[] = {0x41, 0x01};
	unsigned char tv_encoder_regs_ntsc[] = {0x00, 0x00, 0x00, 0x03, 0x00,
						0x00};
        unsigned char tv_encoder_control_regs_ntsc[] = {0x43, 0x01};
	char data[4] = {0x88, 0x00, 0x22, 0x00};
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;

	drm_rk1000_write_reg(0x40, 0, (uchar*)data, 4);

	/* rk1000 power down output dac */
	data[0] = 0x07;
	drm_rk1000_write_reg(0x42, 0x03, (uchar*)data, 1);

	if (mode->vdisplay == 576) {
		drm_rk1000_write_reg(0x42, 0, tv_encoder_regs_pal,
				     sizeof(tv_encoder_regs_pal));
		drm_rk1000_write_reg(0x40, 3, tv_encoder_control_regs_pal,
				     sizeof(tv_encoder_control_regs_pal));
	} else {
		drm_rk1000_write_reg(0x42, 0, tv_encoder_regs_ntsc,
				     sizeof(tv_encoder_regs_ntsc));
		drm_rk1000_write_reg(0x40, 3, tv_encoder_control_regs_ntsc,
				     sizeof(tv_encoder_control_regs_ntsc));
	}

	return 0;
}

static int rk1000_tve_disable(struct display_state *state)
{
	char data[1] = {0x07};

	/* rk1000 power down output dac */
	drm_rk1000_write_reg(0x42, 0x03, (uchar*)data, 1);

	return 0;
}

const struct rockchip_panel_funcs rockchip_rk1000_funcs = {
	.init		= drm_rk1000_tve_init,
	.deinit		= rk1000_tve_deinit,
	.prepare	= rk1000_tve_prepare,
	.unprepare	= rk1000_tve_unprepare,
	.enable		= rk1000_tve_enable,
	.disable	= rk1000_tve_disable,
};
