// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * maxim-max96755.c  --  I2C register interface access for max96755 serdes chip
 *
 * Copyright (c) 2023-2028 Rockchip Electronics Co. Ltd.
 *
 * Author: luowei <lw@rock-chips.com>
 */
#include <serdes-display-core.h>
#include "maxim-max96755.h"

int max96755_bridge_init(struct serdes *serdes)
{
	return 0;
}

static bool max96755_bridge_detect(struct serdes *serdes)
{
	u32 val;

	if (dm_gpio_is_valid(&serdes->lock_gpio)) {
		if (!dm_gpio_get_value(&serdes->lock_gpio))
			return false;
	}

	if (serdes_reg_read(serdes, 0x0013, &val))
		return false;

	if (!FIELD_GET(LOCKED, val))
		return false;

	return true;
}

static int max96755_bridge_enable(struct serdes *serdes)
{
	int ret = 0;

	SERDES_DBG_CHIP("%s: serdes chip %s ret=%d\n",
			__func__, serdes->chip_data->name, ret);
	return ret;
}

static int max96755_bridge_disable(struct serdes *serdes)
{
	int ret = 0;

	ret = serdes_set_bits(serdes, 0x0002, VID_TX_EN_X,
			      FIELD_PREP(VID_TX_EN_X, 0));

	return ret;
}

static struct serdes_chip_bridge_ops max96755_bridge_ops = {
	.init	= max96755_bridge_init,
	.detect	= max96755_bridge_detect,
	.enable	= max96755_bridge_enable,
	.disable	= max96755_bridge_disable,
};

struct serdes_chip_data serdes_max96755_data = {
	.name		= "max96755",
	.serdes_type	= TYPE_SER,
	.serdes_id	= MAXIM_ID_MAX96755,
	.bridge_ops	= &max96755_bridge_ops,
};
EXPORT_SYMBOL_GPL(serdes_max96755_data);
