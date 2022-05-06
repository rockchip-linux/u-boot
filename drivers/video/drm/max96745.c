// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2022 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <max96745.h>
#include <video_bridge.h>

#include "rockchip_bridge.h"
#include "rockchip_display.h"
#include "rockchip_panel.h"

static void max96745_bridge_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;

	dm_i2c_reg_clrset(dev->parent, 0x0100, VID_TX_EN,
			  FIELD_PREP(VID_TX_EN, 1));
}

static void max96745_bridge_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;

	dm_i2c_reg_clrset(dev->parent, 0x0100, VID_TX_EN,
			  FIELD_PREP(VID_TX_EN, 0));
}

static const struct rockchip_bridge_funcs max96745_bridge_funcs = {
	.enable = max96745_bridge_enable,
	.disable = max96745_bridge_disable,
};

static int max96745_bridge_probe(struct udevice *dev)
{
	struct rockchip_bridge *bridge;

	dm_i2c_reg_write(dev->parent, 0x7019, 0x00);
	dm_i2c_reg_write(dev->parent, 0x70a0, 0x04);
	dm_i2c_reg_write(dev->parent, 0x7074, 0x14);
	dm_i2c_reg_write(dev->parent, 0x7070, 0x04);
	dm_i2c_reg_write(dev->parent, 0x7000, 0x01);

	bridge = calloc(1, sizeof(*bridge));
	if (!bridge)
		return -ENOMEM;

	dev->driver_data = (ulong)bridge;
	bridge->dev = dev;
	bridge->funcs = &max96745_bridge_funcs;

	return 0;
}

static const struct udevice_id max96745_bridge_of_match[] = {
	{ .compatible = "maxim,max96745-bridge", },
	{ }
};

U_BOOT_DRIVER(max96745_bridge) = {
	.name = "max96745_bridge",
	.id = UCLASS_VIDEO_BRIDGE,
	.of_match = max96745_bridge_of_match,
	.probe = max96745_bridge_probe,
};
