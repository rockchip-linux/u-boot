// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * serdes-bridge.c  --  display bridge for different serdes chips
 *
 * Copyright (c) 2023 Rockchip Electronics Co. Ltd.
 *
 * Author: luowei <lw@rock-chips.com>
 */

#include <serdes-display-core.h>

static void serdes_bridge_init(struct serdes *serdes)
{
	if (serdes->vpower_supply)
		regulator_set_enable(serdes->vpower_supply, true);

	if (dm_gpio_is_valid(&serdes->enable_gpio))
		dm_gpio_set_value(&serdes->enable_gpio, 1);

	mdelay(5);

	video_bridge_set_active(serdes->dev, true);

	if (serdes->chip_data->bridge_ops->init)
		serdes->chip_data->bridge_ops->init(serdes);

	serdes_i2c_set_sequence(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_pre_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev);

	//serdes_bridge_init(serdes);

	if (serdes->chip_data->bridge_ops->pre_enable)
		serdes->chip_data->bridge_ops->pre_enable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_post_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev);

	if (serdes->chip_data->bridge_ops->post_disable)
		serdes->chip_data->bridge_ops->post_disable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev);

	if (serdes->chip_data->serdes_type == TYPE_DES)
		serdes_bridge_init(serdes);

	if (serdes->chip_data->bridge_ops->enable)
		serdes->chip_data->bridge_ops->enable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev);

	if (serdes->chip_data->bridge_ops->disable)
		serdes->chip_data->bridge_ops->disable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_mode_set(struct rockchip_bridge *bridge,
				   const struct drm_display_mode *mode)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev);

	memcpy(&serdes->serdes_bridge->mode, mode,
	       sizeof(struct drm_display_mode));

	SERDES_DBG_MFD("%s: %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name);
}

static bool serdes_bridge_detect(struct rockchip_bridge *bridge)
{
	bool ret = true;
	struct serdes *serdes = dev_get_priv(bridge->dev);

	if (serdes->chip_data->bridge_ops->detect)
		ret = serdes->chip_data->bridge_ops->detect(serdes);

	SERDES_DBG_MFD("%s: %s %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name, (ret == true) ? "detected" : "no detected");

	return ret;
}

struct rockchip_bridge_funcs serdes_bridge_ops = {
	.pre_enable = serdes_bridge_pre_enable,
	.post_disable = serdes_bridge_post_disable,
	.enable = serdes_bridge_enable,
	.disable = serdes_bridge_disable,
	.mode_set = serdes_bridge_mode_set,
	.detect = serdes_bridge_detect,
};

static int serdes_bridge_probe(struct udevice *dev)
{
	struct serdes *serdes = dev_get_priv(dev);
	struct serdes_bridge *serdes_bridge = NULL;
	struct serdes_pinctrl *serdes_pinctrl = NULL;
	struct rockchip_bridge *bridge = NULL;
	int ret;

	ret = i2c_set_chip_offset_len(dev, 2);
	if (ret)
		return ret;

	serdes->dev = dev;
	serdes->chip_data = (struct serdes_chip_data *)dev_get_driver_data(dev);
	serdes->type = serdes->chip_data->serdes_type;

	SERDES_DBG_MFD("serdes %s %s probe start\n",
		       serdes->dev->name, serdes->chip_data->name);

	ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
					   "vpower-supply", &serdes->vpower_supply);
	if (ret && ret != -ENOENT)
		SERDES_DBG_MFD("%s: Cannot get power supply: %d\n",
			       __func__, ret);

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &serdes->enable_gpio, GPIOD_IS_OUT);
	if (ret)
		SERDES_DBG_MFD("%s: failed to get enable gpio: %d\n",
			       __func__, ret);

	ret = gpio_request_by_name(dev, "lock-gpios", 0, &serdes->lock_gpio,
				   GPIOD_IS_IN);
	if (ret)
		SERDES_DBG_MFD("%s: failed to get lock gpio: %d\n",
			       __func__, ret);

	ret = gpio_request_by_name(dev, "err-gpios", 0, &serdes->err_gpio,
				   GPIOD_IS_IN);
	if (ret)
		SERDES_DBG_MFD("%s: failed to err gpio: %d\n",
			       __func__, ret);

	if (serdes->chip_data->serdes_type != TYPE_SER)
		SERDES_DBG_MFD("warning: this chip is not ser type\n");

	if (serdes->chip_data->serdes_type == TYPE_OTHER) {
		SERDES_DBG_MFD("TYPE_OTHER just need only init i2c\n");
		serdes_bridge_init(serdes);
		return 0;
	}

	if (!serdes->chip_data->bridge_ops) {
		SERDES_DBG_MFD("%s %s no bridge ops\n",
			       __func__, serdes->chip_data->name);
		return -1;
	}

	serdes_bridge = calloc(1, sizeof(*serdes_bridge));
	if (!serdes_bridge)
		return -ENOMEM;

	serdes->sel_mipi = dev_read_bool(dev, "sel-mipi");
	if (serdes->sel_mipi) {
		struct mipi_dsi_device *device = dev_get_platdata(dev);

		device->dev = dev;
		device->lanes = dev_read_u32_default(dev, "dsi,lanes", 4);
		device->format = dev_read_u32_default(dev, "dsi,format",
						      MIPI_DSI_FMT_RGB888);
		device->mode_flags = MIPI_DSI_MODE_VIDEO |
				     MIPI_DSI_MODE_VIDEO_BURST |
				     MIPI_DSI_MODE_VIDEO_HBP |
				     MIPI_DSI_MODE_LPM |
				     MIPI_DSI_MODE_EOT_PACKET;
		device->channel = dev_read_u32_default(dev, "reg", 0);
	}

	bridge = calloc(1, sizeof(*bridge));
	if (!bridge)
		return -ENOMEM;

	dev->driver_data = (ulong)bridge;
	bridge->dev = dev;
	bridge->funcs = &serdes_bridge_ops;

	serdes->serdes_bridge = serdes_bridge;
	serdes->serdes_bridge->bridge = bridge;

	serdes_pinctrl = calloc(1, sizeof(*serdes_pinctrl));
	if (!serdes_pinctrl)
		return -ENOMEM;

	serdes->serdes_pinctrl = serdes_pinctrl;

	ret = serdes_pinctrl_register(dev, serdes);
	if (ret)
		return ret;

	ret = serdes_get_init_seq(serdes);
	if (ret)
		return ret;

	if (serdes->chip_data->serdes_type == TYPE_SER)
		serdes_bridge_init(serdes);

	printf("%s %s %s successful\n",
	       __func__,
	       serdes->dev->name,
	       serdes->chip_data->name);

	return 0;
}

static const struct udevice_id serdes_of_match[] = {
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_ROHM_BU18TL82)
	{ .compatible = "rohm,bu18tl82", .data = (ulong)&serdes_bu18tl82_data },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_ROHM_BU18RL82)
	{ .compatible = "rohm,bu18rl82", .data = (ulong)&serdes_bu18rl82_data },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96745)
	{ .compatible = "maxim,max96745", .data = (ulong)&serdes_max96745_data },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96755)
	{ .compatible = "maxim,max96755", .data = (ulong)&serdes_max96755_data },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_ROCKCHIP_RKX111)
	{ .compatible = "rockchip,rkx111", .data = (ulong)&serdes_rkx111_data },
#endif
	{ }
};

U_BOOT_DRIVER(serdes_bridge) = {
	.name = "serdes-bridge",
	.id = UCLASS_VIDEO_BRIDGE,
	.of_match = serdes_of_match,
	.probe = serdes_bridge_probe,
	.priv_auto_alloc_size = sizeof(struct serdes),
	.platdata_auto_alloc_size = sizeof(struct mipi_dsi_device),
};
