// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * serdes-bridge_split.c  --  display bridge_split for different serdes chips
 *
 * Copyright (c) 2023 Rockchip Electronics Co. Ltd.
 *
 * Author: luowei <lw@rock-chips.com>
 */

#include "core.h"

static void serdes_bridge_split_init(struct serdes *serdes)
{
	if (serdes->vpower_supply)
		regulator_set_enable(serdes->vpower_supply, true);

	if (dm_gpio_is_valid(&serdes->enable_gpio))
		dm_gpio_set_value(&serdes->enable_gpio, 1);

	mdelay(5);

	//video_bridge_set_active(serdes->dev, true);

	if (serdes->chip_data->bridge_ops->init)
		serdes->chip_data->bridge_ops->init(serdes);

	serdes_i2c_set_sequence(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_split_pre_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	if (serdes->chip_data->bridge_ops->pre_enable)
		serdes->chip_data->bridge_ops->pre_enable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_split_post_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	if (serdes->chip_data->bridge_ops->post_disable)
		serdes->chip_data->bridge_ops->post_disable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_split_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	if (serdes->chip_data->serdes_type == TYPE_DES)
		serdes_bridge_split_init(serdes);

	serdes_pinctrl_register(serdes->dev);

	if (serdes->chip_data->bridge_ops->enable)
		serdes->chip_data->bridge_ops->enable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_split_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	if (serdes->chip_data->bridge_ops->disable)
		serdes->chip_data->bridge_ops->disable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_split_mode_set(struct rockchip_bridge *bridge,
					 const struct drm_display_mode *mode)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	memcpy(&serdes->serdes_bridge_split->mode, mode,
	       sizeof(struct drm_display_mode));

	SERDES_DBG_MFD("%s: %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name);
}

static bool serdes_bridge_split_detect(struct rockchip_bridge *bridge)
{
	bool ret = true;
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	if (serdes->chip_data->bridge_ops->detect)
		ret = serdes->chip_data->bridge_ops->detect(serdes, SER_LINKB);

	SERDES_DBG_MFD("%s: %s %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name, ret ? "detected" : "no detected");

	return ret;
}

struct rockchip_bridge_funcs serdes_bridge_split_ops = {
	.pre_enable = serdes_bridge_split_pre_enable,
	.post_disable = serdes_bridge_split_post_disable,
	.enable = serdes_bridge_split_enable,
	.disable = serdes_bridge_split_disable,
	.mode_set = serdes_bridge_split_mode_set,
	.detect = serdes_bridge_split_detect,
};

static int serdes_bridge_split_parse_dt(struct udevice *dev)
{
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct serdes_bridge_split *serdes_bridge_split = dev_get_priv(dev);

	serdes->sel_mipi = dev_read_bool(dev->parent, "sel-mipi");
	if (serdes->sel_mipi) {
		serdes_bridge_split->lanes =
			dev_read_u32_default(dev, "dsi,lanes", 4);
		serdes_bridge_split->format =
			dev_read_u32_default(dev, "dsi,format",
					     MIPI_DSI_FMT_RGB888);
		serdes_bridge_split->flags =
			dev_read_u32_default(dev, "dsi,flags",
					     MIPI_DSI_MODE_VIDEO |
					     MIPI_DSI_MODE_VIDEO_SYNC_PULSE);
		serdes_bridge_split->channel =
			dev_read_u32_default(dev, "reg", 0);
	}

	return 0;
}

static int serdes_bridge_split_probe(struct udevice *dev)
{
	int ret;
	struct rockchip_bridge *bridge;
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct mipi_dsi_device *device = dev_get_plat(dev);
	struct serdes_bridge_split *serdes_bridge_split = dev_get_priv(dev);

	ret = serdes_bridge_split_parse_dt(dev);
	if (ret)
		printf("%s %s parse dt failed, ret=%d\n",
		       __func__, serdes->dev->name, ret);

	if (serdes->sel_mipi) {
		device->dev = dev;
		device->lanes = serdes_bridge_split->lanes;
		device->format = serdes_bridge_split->format;
		device->mode_flags = serdes_bridge_split->flags;
		device->channel = serdes_bridge_split->channel;
	}

	bridge = calloc(1, sizeof(*bridge));
	if (!bridge)
		return -ENOMEM;

	dev->driver_data = (ulong)bridge;
	bridge->dev = dev;
	bridge->funcs = &serdes_bridge_split_ops;

	serdes_bridge_split->bridge = bridge;
	serdes->serdes_bridge_split = serdes_bridge_split;

	SERDES_DBG_MFD("%s: %s %s bridge=%p name=%s device=%p\n",
		       __func__, serdes->dev->name,
		       serdes->chip_data->name,
		       bridge, bridge->dev->name, device);

	return 0;
}

static const struct udevice_id serdes_of_match[] = {
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96745)
	{ .compatible = "maxim,max96745-bridge-split", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96749)
	{ .compatible = "maxim,max96749-bridge-split", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96755)
	{ .compatible = "maxim,max96755-bridge-split", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96789)
	{ .compatible = "maxim,max96789-bridge-split", },
#endif
	{ }
};

U_BOOT_DRIVER(serdes_bridge_split) = {
	.name = "serdes-bridge-split",
	.id = UCLASS_VIDEO_BRIDGE,
	.of_match = serdes_of_match,
	.probe = serdes_bridge_split_probe,
	.priv_auto = sizeof(struct serdes_bridge_split),
	.plat_auto = sizeof(struct mipi_dsi_device),
};
