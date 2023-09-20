// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * maxim-max96772.c  --  I2C register interface access for max96772 serdes chip
 *
 * Copyright (c) 2023-2028 Rockchip Electronics Co. Ltd.
 *
 * Author: luowei <lw@rock-chips.com>
 */

#include <serdes-display-core.h>
#include "maxim-max96772.h"

int max96772_panel_init(struct serdes *serdes)
{
	return 0;
}

int max96772_panel_prepare(struct serdes *serdes)
{
	return 0;
}

int max96772_panel_unprepare(struct serdes *serdes)
{
	return 0;
}

int max96772_panel_enable(struct serdes *serdes)
{
	return 0;
}

int max96772_panel_disable(struct serdes *serdes)
{
	return 0;
}

int max96772_panel_backlight_enable(struct serdes *serdes)
{
	return 0;
}

int max96772_panel_backlight_disable(struct serdes *serdes)
{
	return 0;
}

static struct serdes_chip_panel_ops max96772_panel_ops = {
	.init		= max96772_panel_init,
	.prepare	= max96772_panel_prepare,
	.unprepare	= max96772_panel_unprepare,
	.enable		= max96772_panel_enable,
	.disable	= max96772_panel_disable,
	.backlight_enable	= max96772_panel_backlight_enable,
	.backlight_disable	= max96772_panel_backlight_disable,
};

struct serdes_chip_data serdes_max96772_data = {
	.name		= "max96772",
	.serdes_type	= TYPE_DES,
	.serdes_id	= MAXIM_ID_MAX96772,
	.panel_ops	= &max96772_panel_ops,
};
EXPORT_SYMBOL_GPL(serdes_max96772_data);
