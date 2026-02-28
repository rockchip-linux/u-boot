// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2026 Rockchip Electronics Co., Ltd
 *
 * USB Power Delivery protocol stack.
 */

#include <command.h>
#include <errno.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <power/power_delivery/power_delivery.h>

DECLARE_GLOBAL_DATA_PTR;

int power_delivery_get_data(struct udevice *dev, struct power_delivery_data *pd_data)
{
	const struct dm_power_delivery_ops *ops = dev_get_driver_ops(dev);

	if (!ops || !ops->get_current || !ops->get_voltage || !ops->get_online)
		return -ENOSYS;

	pd_data->voltage = ops->get_voltage(dev);
	pd_data->current = ops->get_current(dev);
	pd_data->online = ops->get_online(dev);

	return 0;
}

UCLASS_DRIVER(power_delivery) = {
	.id		= UCLASS_PD,
	.name		= "power_delivery",
};

static int do_pd_info(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct power_delivery_data power_data;
	struct udevice *dev;
	int ret;

	ret = uclass_get_device(UCLASS_PD, 0, &dev);
	if (ret) {
		printf("Can't get PD device!\n");
		return ret;
	}

	ret = power_delivery_get_data(dev, &power_data);
	if (ret) {
		printf("Can't get PD data!\n");
		return ret;
	}

	if (power_data.online) {
		if (power_data.current)
			printf("PD Voltage=%dmV, Current=%dmA\n",
			       power_data.voltage / 1000,
			       power_data.current / 1000);
		else
			printf("PD is not supported!\n");
	} else {
		printf("PD is not connected!\n");
	}

	return 0;
}

U_BOOT_CMD(pd_info, 1, 0, do_pd_info, "dump pd information", "");
