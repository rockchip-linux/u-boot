/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <dm.h>
#include <common.h>
#include <power/fuel_gauge.h>

static int fg_test_bat_is_exit(struct udevice *dev)
{
	return 1;
}

static int fg_test_get_soc(struct udevice *dev)
{
	return 50;
}

static int fg_test_get_voltage(struct udevice *dev)
{
	return 3600;
}

static int fg_test_get_current(struct udevice *dev)
{
	return 1000;
}

static bool fg_test_get_chrg_online(struct udevice *dev)
{
	return 1;
}

static int fg_test_charger_capability(struct udevice *dev)
{
	return FG_CAP_CHARGER | FG_CAP_FUEL_GAUGE;
}

static struct dm_fuel_gauge_ops fg_test_ops = {
	.bat_is_exist = fg_test_bat_is_exit,
	.get_soc = fg_test_get_soc,
	.get_voltage = fg_test_get_voltage,
	.get_current = fg_test_get_current,
	.get_chrg_online = fg_test_get_chrg_online,
	.capability = fg_test_charger_capability,
};

static const struct udevice_id fg_test_ids[] = {
	{ .compatible = "fg,test" },
	{ },
};

static int fg_test_probe(struct udevice *dev)
{
	return 0;
}

U_BOOT_DRIVER(fg_test) = {
	.name = "fg_test",
	.id = UCLASS_FG,
	.probe = fg_test_probe,
	.ops = &fg_test_ops,
	.of_match = fg_test_ids,
};
