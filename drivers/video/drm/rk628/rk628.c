// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2022 Rockchip Electronics Co., Ltd
 */

#include <clk.h>
#include "rk628.h"
#include "rk628_cru.h"
#include "rk628_dsi.h"
#include "rk628_gvi.h"
#include "rk628_hdmirx.h"
#include "rk628_hdmitx.h"
#include "rk628_lvds.h"
#include "rk628_post_process.h"
#include "rk628_rgb.h"

static int rk628_power_on(struct rk628 *rk628)
{
	if (rk628->power_supply)
		regulator_set_enable(rk628->power_supply, 1);

	if (dm_gpio_is_valid(&rk628->enable_gpio)) {
		dm_gpio_set_value(&rk628->enable_gpio, 1);
		mdelay(10);
	}

	if (dm_gpio_is_valid(&rk628->reset_gpio)) {
		dm_gpio_set_value(&rk628->reset_gpio, 0);
		mdelay(10);
		dm_gpio_set_value(&rk628->reset_gpio, 1);
		mdelay(10);
		dm_gpio_set_value(&rk628->reset_gpio, 0);
		mdelay(10);
	}

	/* selete int io function */
	rk628_i2c_write(rk628, GRF_GPIO3AB_SEL_CON, 0x30002000);

	return 0;
}

static int rk628_version_info(struct rk628 *rk628)
{
	int ret;
	char *version;

	ret = rk628_i2c_read(rk628, GRF_SOC_VERSION, &rk628->version);
	if (ret < 0) {
		printf("failed to access rk628 register: %d\n", ret);
		return ret;
	}

	switch (rk628->version) {
	case RK628D_VERSION:
		version = "D/E";
		break;
	case RK628F_VERSION:
		version = "F/H";
		break;
	default:
		version = "unknown";
		ret = -EINVAL;
	}

	printf("the IC version is: RK628-%s\n", version);

	return ret;
}

static int rk628_display_route_info_parse(struct rk628 *rk628)
{
	int ret = 0;
	ofnode np;
	u32 val;

	if (dev_read_bool(rk628->dev, "rk628-hdmi-in") ||
	    dev_read_bool(rk628->dev, "rk628,hdmi-in")) {
		rk628->input_mode = BIT(INPUT_MODE_HDMI);
	} else if (dev_read_bool(rk628->dev, "rk628-rgb-in") ||
		   dev_read_bool(rk628->dev, "rk628,rgb-in")) {
		rk628->input_mode = BIT(INPUT_MODE_RGB);
		ret = rk628_rgbrx_parse(rk628);
	} else if (dev_read_bool(rk628->dev, "rk628-bt1120-in") ||
		   dev_read_bool(rk628->dev, "rk628,bt1120-in")) {
		rk628->input_mode = BIT(INPUT_MODE_BT1120);
		ret = rk628_rgbrx_parse(rk628);
	} else {
		rk628->input_mode = BIT(INPUT_MODE_RGB);
	}

	if (ofnode_valid(dev_read_subnode(rk628->dev, "rk628-gvi-out")) ||
	    ofnode_valid(dev_read_subnode(rk628->dev, "rk628-gvi"))) {
		np = dev_read_subnode(rk628->dev, "rk628-gvi-out");
		if (!ofnode_valid(np))
			np = dev_read_subnode(rk628->dev, "rk628-gvi");

		rk628->output_mode |= BIT(OUTPUT_MODE_GVI);
		ret = rk628_gvi_parse(rk628, np);
	} else if (ofnode_valid(dev_read_subnode(rk628->dev, "rk628-lvds-out")) ||
		   ofnode_valid(dev_read_subnode(rk628->dev, "rk628-lvds"))) {
		np = dev_read_subnode(rk628->dev, "rk628-lvds-out");
		if (!ofnode_valid(np))
			np = dev_read_subnode(rk628->dev, "rk628-lvds");

		rk628->output_mode |= BIT(OUTPUT_MODE_LVDS);
		ret = rk628_lvds_parse(rk628, np);
	} else if (ofnode_valid(dev_read_subnode(rk628->dev, "rk628-dsi-out")) ||
		   ofnode_valid(dev_read_subnode(rk628->dev, "rk628-dsi"))) {
		np = dev_read_subnode(rk628->dev, "rk628-dsi-out");
		if (!ofnode_valid(np))
			np = dev_read_subnode(rk628->dev, "rk628-dsi");

		rk628->output_mode |= BIT(OUTPUT_MODE_DSI);
		ret = rk628_dsi_parse(rk628, np);
	} else if (dev_read_bool(rk628->dev, "rk628-csi-out") ||
		   dev_read_bool(rk628->dev, "rk628,csi-out")) {
		rk628->output_mode |= BIT(OUTPUT_MODE_CSI);
	}

	if (dev_read_bool(rk628->dev, "rk628-hdmi-out") ||
	    dev_read_bool(rk628->dev, "rk628,hdmi-out"))
		rk628->output_mode |= BIT(OUTPUT_MODE_HDMI);

	if (dev_read_bool(rk628->dev, "rk628-rgb-out") ||
	    dev_read_bool(rk628->dev, "rk628-rgb")) {
		np = dev_read_subnode(rk628->dev, "rk628-rgb-out");
		if (!ofnode_valid(np))
			np = dev_read_subnode(rk628->dev, "rk628-rgb");

		rk628->output_mode |= BIT(OUTPUT_MODE_RGB);
		ret = rk628_rgbtx_parse(rk628, np);
	} else if (dev_read_bool(rk628->dev, "rk628-bt1120-out") ||
		   dev_read_bool(rk628->dev, "rk628-bt1120")) {
		np = dev_read_subnode(rk628->dev, "rk628-bt1120-out");
		if (!ofnode_valid(np))
			np = dev_read_subnode(rk628->dev, "rk628-bt1120");

		rk628->output_mode |= BIT(OUTPUT_MODE_BT1120);
		ret = rk628_rgbtx_parse(rk628, np);
	}

	val = dev_read_u32_default(rk628->dev, "mode-sync-pol", 1);
	rk628->sync_pol = (!val ? MODE_FLAG_NSYNC : MODE_FLAG_PSYNC);

	return ret;
}

static bool rk628_display_route_check(struct rk628 *rk628)
{
	if (!hweight32(rk628->input_mode) || !hweight32(rk628->output_mode))
		return false;

	/*
	 * the RGB/BT1120 RX and RGB/BT1120 TX are the same shared IP
	 * and cannot be used as both input and output simultaneously.
	 */
	if ((rk628_input_is_rgb(rk628) || rk628_input_is_bt1120(rk628)) &&
	    (rk628_output_is_rgb(rk628) || rk628_output_is_bt1120(rk628)))
		return false;

	if (rk628->version == RK628F_VERSION)
		return true;

	/* rk628d only support rgb and hdmi output simultaneously */
	if (hweight32(rk628->output_mode) > 2)
		return false;

	if (hweight32(rk628->output_mode) == 2 &&
	    !(rk628_output_is_rgb(rk628) && rk628_output_is_hdmi(rk628)))
		return false;

	return true;
}

static inline size_t strlcat(char *dest, const char *src, size_t n)
{
	strcat(dest, src);
	return strlen(dest) + strlen(src);
}

static void rk628_current_display_route(struct rk628 *rk628, char *input_s,
					int input_s_len, char *output_s,
					int output_s_len)
{
	*input_s = '\0';
	*output_s = '\0';

	if (rk628_input_is_rgb(rk628))
		strlcat(input_s, "RGB", input_s_len);
	else if (rk628_input_is_bt1120(rk628))
		strlcat(input_s, "BT1120", input_s_len);
	else if (rk628_input_is_hdmi(rk628))
		strlcat(input_s, "HDMI", input_s_len);
	else
		strlcat(input_s, "unknown", input_s_len);

	if (rk628_output_is_rgb(rk628))
		strlcat(output_s, "RGB ", output_s_len);

	if (rk628_output_is_bt1120(rk628))
		strlcat(output_s, "BT1120 ", output_s_len);

	if (rk628_output_is_gvi(rk628))
		strlcat(output_s, "GVI ", output_s_len);

	if (rk628_output_is_lvds(rk628))
		strncat(output_s, "LVDS ", output_s_len);

	if (rk628_output_is_dsi(rk628))
		strlcat(output_s, "DSI ", output_s_len);

	if (rk628_output_is_csi(rk628))
		strlcat(output_s, "CSI ", output_s_len);

	if (rk628_output_is_hdmi(rk628))
		strlcat(output_s, "HDMI ", output_s_len);

	if (!strlen(output_s))
		strlcat(output_s, "unknown", output_s_len);
}

static void rk628_show_current_display_route(struct rk628 *rk628)
{
	char input_s[10], output_s[30];

	rk628_current_display_route(rk628, input_s, sizeof(input_s),
				    output_s, sizeof(output_s));

	printf("rk628 input_mode: %s, output_mode: %s\n", input_s, output_s);
}

static void rk628_pwr_consumption_init(struct rk628 *rk628)
{
	/* set pin as int function to allow output interrupt */
	rk628_i2c_write(rk628, GRF_GPIO3AB_SEL_CON, 0x30002000);

	/*
	 * set unuse pin as GPIO input and
	 * pull down to reduce power consumption
	 */
	rk628_i2c_write(rk628, GRF_GPIO2AB_SEL_CON, 0xffff0000);
	rk628_i2c_write(rk628, GRF_GPIO2C_SEL_CON, 0xffff0000);
	rk628_i2c_write(rk628, GRF_GPIO3AB_SEL_CON, 0x10b0000);
	rk628_i2c_write(rk628, GRF_GPIO2C_P_CON, 0x3f0015);
	rk628_i2c_write(rk628, GRF_GPIO3A_P_CON, 0xcc0044);

	if (!rk628_output_is_hdmi(rk628)) {
		u32 mask = SW_OUTPUT_MODE_MASK;
		u32 val = SW_OUTPUT_MODE(OUTPUT_MODE_HDMI);

		if (rk628->version == RK628F_VERSION) {
			mask = SW_HDMITX_EN_MASK;
			val = SW_HDMITX_EN(1);
		}

		/* disable clock/data channel and band gap when hdmitx not work */
		rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0, mask, val);
		rk628_i2c_write(rk628, HDMI_PHY_SYS_CTL, 0x17);
		rk628_i2c_write(rk628, HDMI_PHY_CHG_PWR, 0x0);
		rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0, mask, 0);
	}
}

static void rk628_display_enable(struct rk628 *rk628)
{
	if (rk628_input_is_rgb(rk628))
		rk628_rgb_rx_enable(rk628);

	/*
	 * bt1120 needs to configure the timing register, but hdmitx will modify
	 * the timing as needed, so the bt1120 enable process is moved to the
	 * configuration of post_process (function rk628_post_process_enable in
	 * rk628_post_process.c)
	 */

	if (rk628_output_is_rgb(rk628))
		rk628_rgb_tx_enable(rk628);

	if (rk628_input_is_hdmi(rk628))
		rk628_hdmirx_enable(rk628);

	if (rk628_output_is_dsi(rk628)) {
		rk628_mipi_dsi_pre_enable(rk628);
		rk628_mipi_dsi_enable(rk628);
	}


	if (rk628_output_is_bt1120(rk628))
		rk628_bt1120_tx_enable(rk628);

	if (!rk628_output_is_hdmi(rk628)) {
		rk628_post_process_init(rk628);
		rk628_post_process_enable(rk628);
	}

	if (rk628_output_is_lvds(rk628))
		rk628_lvds_enable(rk628);

	if (rk628_output_is_gvi(rk628))
		rk628_gvi_enable(rk628);
}

static void
of_parse_rk628_display_timing( ofnode np, struct rk628_videomode *vm)
{
	u32 val = 0;

	ofnode_read_u32(np, "clock-frequency", &vm->pixelclock);
	ofnode_read_u32(np, "hactive", &vm->hactive);
	ofnode_read_u32(np, "hfront-porch", &vm->hfront_porch);
	ofnode_read_u32(np, "hback-porch", &vm->hback_porch);
	ofnode_read_u32(np, "hsync-len", &vm->hsync_len);

	ofnode_read_u32(np, "vactive", &vm->vactive);
	ofnode_read_u32(np, "vfront-porch", &vm->vfront_porch);
	ofnode_read_u32(np, "vback-porch", &vm->vback_porch);
	ofnode_read_u32(np, "vsync-len", &vm->vsync_len);

	vm->flags = 0;
	ofnode_read_u32(np, "hsync-active", &val);
	vm->flags |= val ? DRM_MODE_FLAG_PHSYNC : DRM_MODE_FLAG_NHSYNC;

	ofnode_read_u32(np, "vsync-active", &val);
	vm->flags |= val ? DRM_MODE_FLAG_PVSYNC : DRM_MODE_FLAG_NVSYNC;
}

static void
rk628_display_mode_from_videomode(const struct rk628_videomode *vm,
				  struct drm_display_mode *dmode)
{
	dmode->hdisplay = vm->hactive;
	dmode->hsync_start = dmode->hdisplay + vm->hfront_porch;
	dmode->hsync_end = dmode->hsync_start + vm->hsync_len;
	dmode->htotal = dmode->hsync_end + vm->hback_porch;

	dmode->vdisplay = vm->vactive;
	dmode->vsync_start = dmode->vdisplay + vm->vfront_porch;
	dmode->vsync_end = dmode->vsync_start + vm->vsync_len;
	dmode->vtotal = dmode->vsync_end + vm->vback_porch;

	dmode->clock = vm->pixelclock / 1000;
	dmode->flags = vm->flags;
}

static int rk628_get_video_mode(struct rk628 *rk628)
{
	ofnode timings_np, src_np, dst_np;
	struct rk628_videomode vm;

	timings_np = ofnode_find_subnode(dev_ofnode(rk628->dev), "display-timings");
	if (!ofnode_valid(timings_np))
		return -EINVAL;

	src_np = ofnode_find_subnode(timings_np, "src-timing");
	if (!ofnode_valid(src_np)) {
		printf("rk628 failed to found src timing\n");
		return -EINVAL;
	}

	of_parse_rk628_display_timing(src_np, &vm);
	rk628_display_mode_from_videomode(&vm, &rk628->src_mode);
	printf("rk628 src mode: %d %d %d %d %d %d %d %d %d 0x%x\n",
		rk628->src_mode.clock, rk628->src_mode.hdisplay, rk628->src_mode.hsync_start,
		rk628->src_mode.hsync_end, rk628->src_mode.htotal, rk628->src_mode.vdisplay,
		rk628->src_mode.vsync_start, rk628->src_mode.vsync_end, rk628->src_mode.vtotal,
		rk628->src_mode.flags);

	dst_np = ofnode_find_subnode(timings_np, "dst-timing");
	if (!ofnode_valid(dst_np)) {
		printf("rk628 failed to found dst timing\n");
		return -EINVAL;
	}

	of_parse_rk628_display_timing(dst_np, &vm);
	rk628_display_mode_from_videomode(&vm, &rk628->dst_mode);
	printf("rk628 dst mode: %d %d %d %d %d %d %d %d %d 0x%x\n",
		rk628->dst_mode.clock, rk628->dst_mode.hdisplay, rk628->dst_mode.hsync_start,
		rk628->dst_mode.hsync_end, rk628->dst_mode.htotal, rk628->dst_mode.vdisplay,
		rk628->dst_mode.vsync_start, rk628->dst_mode.vsync_end, rk628->dst_mode.vtotal,
		rk628->dst_mode.flags);

	return 0;
}

static int rk628_display_timings_get(struct rk628 *rk628)
{
	int ret;

	ret = rk628_get_video_mode(rk628);

	return ret;

}

static int rk628_probe(struct udevice *dev)
{
	struct rk628 *rk628 = dev_get_priv(dev);
	int ret;

	ret = i2c_set_chip_offset_len(dev, 4);
	if (ret)
		return ret;

	rk628->dev = dev;

	ret = rk628_display_route_info_parse(rk628);
	if (ret) {
		printf("display route parse err\n");
		return ret;
	}

	if (!rk628_output_is_csi(rk628) && !rk628_output_is_hdmi(rk628)) {
		ret = rk628_display_timings_get(rk628);
		if (ret) {
			printf("rk628 display timings err\n");
			return ret;
		}
	}

	ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
					   "power-supply",
					   &rk628->power_supply);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "Cannot get power supply: %d\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &rk628->enable_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "%s: failed to get enable GPIO: %d\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0,
				   &rk628->reset_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "%s: failed to get reset GPIO: %d\n", __func__, ret);
		return ret;
	}

	/*
	 * Process 'assigned-{clocks/clock-parents/clock-rates}'
	 * properties for ref clock from soc
	 */
	ret = clk_set_defaults(dev);
	if (ret)
		dev_err(dev, "%s clk_set_defaults failed %d\n", __func__, ret);

	rk628_power_on(rk628);
	rk628_version_info(rk628);

	rk628_show_current_display_route(rk628);

	if (!rk628_display_route_check(rk628)) {
		printf("rk628 display route check err\n");
		return -EINVAL;
	}

	rk628_pwr_consumption_init(rk628);
	rk628_cru_init(rk628);
	rk628_display_enable(rk628);

	return 0;
}

static const struct udevice_id rk628_of_match[] = {
	{ .compatible = "rockchip,rk628" },
	{}
};

U_BOOT_DRIVER(rk628) = {
	.name = "rk628",
	.id = UCLASS_I2C_GENERIC,
	.of_match = rk628_of_match,
	.bind = dm_scan_fdt_dev,
	.probe = rk628_probe,
	.priv_auto_alloc_size = sizeof(struct rk628),
};
