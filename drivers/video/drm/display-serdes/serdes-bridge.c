// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * serdes-bridge.c  --  display bridge for different serdes chips
 *
 * Copyright (c) 2023 Rockchip Electronics Co. Ltd.
 *
 * Author: luowei <lw@rock-chips.com>
 */

#include "core.h"

static void serdes_bridge_init(struct serdes *serdes)
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

static void serdes_bridge_pre_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct serdes_bridge *serdes_bridge = serdes->serdes_bridge;

	if (serdes->chip_data->bridge_ops->pre_enable)
		serdes->chip_data->bridge_ops->pre_enable(serdes);

	if (serdes_bridge->split_mode)
		rockchip_panel_prepare(serdes_bridge->panel_split);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_post_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct serdes_bridge *serdes_bridge = serdes->serdes_bridge;

	if (serdes_bridge->split_mode)
		rockchip_panel_unprepare(serdes_bridge->panel_split);

	if (serdes->chip_data->bridge_ops->post_disable)
		serdes->chip_data->bridge_ops->post_disable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct serdes_bridge *serdes_bridge = serdes->serdes_bridge;

	if (serdes->chip_data->serdes_type == TYPE_DES)
		serdes_bridge_init(serdes);

	serdes_pinctrl_register(serdes->dev);

	if (serdes->chip_data->bridge_ops->enable)
		serdes->chip_data->bridge_ops->enable(serdes);

	if (serdes_bridge->split_mode)
		rockchip_panel_enable(serdes_bridge->panel_split);

	SERDES_DBG_MFD("%s: %s %s\n", __func__,
		       serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct serdes_bridge *serdes_bridge = serdes->serdes_bridge;

	if (serdes_bridge->split_mode)
		rockchip_panel_disable(serdes_bridge->panel_split);

	if (serdes->chip_data->bridge_ops->disable)
		serdes->chip_data->bridge_ops->disable(serdes);

	SERDES_DBG_MFD("%s: %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name);
}

static void serdes_bridge_mode_set(struct rockchip_bridge *bridge,
				   const struct drm_display_mode *mode)
{
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	memcpy(&serdes->serdes_bridge->mode, mode,
	       sizeof(struct drm_display_mode));

	SERDES_DBG_MFD("%s: %s %s\n", __func__, serdes->dev->name,
		       serdes->chip_data->name);
}

static bool serdes_bridge_detect(struct rockchip_bridge *bridge)
{
	bool ret = true;
	struct udevice *dev = bridge->dev;
	struct serdes *serdes = dev_get_priv(dev->parent);

	if (serdes->mcu_enable) {
		printf("serdes %s detect link status in MCU\n",
		       serdes->dev->name);
		return ret;
	}

	if (serdes->chip_data->bridge_ops->detect)
		ret = serdes->chip_data->bridge_ops->detect(serdes, SER_LINKA);

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

static int serdes_bridge_parse_dt(struct udevice *dev)
{
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct serdes_bridge *serdes_bridge = dev_get_priv(dev);

	serdes->sel_mipi = dev_read_bool(dev->parent, "sel-mipi");
	if (serdes->sel_mipi) {
		serdes_bridge->lanes =
			dev_read_u32_default(dev, "dsi,lanes", 4);
		serdes_bridge->format =
			dev_read_u32_default(dev, "dsi,format",
					     MIPI_DSI_FMT_RGB888);
		serdes_bridge->flags =
			dev_read_u32_default(dev, "dsi,flags",
					     MIPI_DSI_MODE_VIDEO |
					     MIPI_DSI_MODE_VIDEO_SYNC_PULSE);
		serdes_bridge->channel = dev_read_u32_default(dev, "reg", 0);
	}

	return 0;
}

static int serdes_bridge_probe(struct udevice *dev)
{
	int ret;
	struct rockchip_bridge *bridge;
	struct serdes *serdes = dev_get_priv(dev->parent);
	struct mipi_dsi_device *device = dev_get_platdata(dev);
	struct serdes_bridge *serdes_bridge = dev_get_priv(dev);

	if (!serdes->chip_data->bridge_ops) {
		SERDES_DBG_MFD("%s %s no bridge ops\n",
			       __func__, serdes->chip_data->name);
		return 0;
	}

	ret = serdes_bridge_parse_dt(dev);
	if (ret)
		printf("%s %s parse dt failed, ret=%d\n",
		       __func__, serdes->dev->name, ret);

	if (serdes->sel_mipi) {
		device->dev = dev;
		device->lanes = serdes_bridge->lanes;
		device->format = serdes_bridge->format;
		device->mode_flags = serdes_bridge->flags;
		device->channel = serdes_bridge->channel;
	}

	bridge = calloc(1, sizeof(*bridge));
	if (!bridge)
		return -ENOMEM;

	dev->driver_data = (ulong)bridge;
	bridge->dev = dev;
	bridge->funcs = &serdes_bridge_ops;

	serdes_bridge->bridge = bridge;
	serdes->serdes_bridge = serdes_bridge;

	serdes_get_split_bridge_or_panel(serdes->serdes_bridge);

	SERDES_DBG_MFD("%s: %s %s bridge=%p name=%s device=%p\n",
		       __func__, serdes->dev->name,
		       serdes->chip_data->name,
		       bridge, bridge->dev->name, device);

	return 0;
}

static const struct udevice_id serdes_of_match[] = {
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_ROHM_BU18TL82)
	{ .compatible = "rohm,bu18tl82-bridge",  },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_ROHM_BU18RL82)
	{ .compatible = "rohm,bu18rl82-bridge", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96745)
	{ .compatible = "maxim,max96745-bridge", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96749)
	{ .compatible = "maxim,max96749-bridge", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96755)
	{ .compatible = "maxim,max96755-bridge", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_MAXIM_MAX96755)
	{ .compatible = "maxim,max96789-bridge", },
#endif
#if IS_ENABLED(CONFIG_SERDES_DISPLAY_CHIP_ROCKCHIP_RKX111)
	{ .compatible = "rockchip,rkx111-bridge", },
#endif
	{ }
};

U_BOOT_DRIVER(serdes_bridge) = {
	.name = "serdes-bridge",
	.id = UCLASS_VIDEO_BRIDGE,
	.of_match = serdes_of_match,
	.probe = serdes_bridge_probe,
	.priv_auto_alloc_size = sizeof(struct serdes_bridge),
	.platdata_auto_alloc_size = sizeof(struct mipi_dsi_device),
};
