// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * maxim-max96752.c  --  I2C register interface access for max96752 serdes chip
 *
 * Copyright (c) 2023-2028 Rockchip Electronics Co. Ltd.
 *
 * Author: luowei <lw@rock-chips.com>
 */
#include <serdes-display-core.h>
#include "maxim-max96752.h"

int max96752_panel_prepare(struct serdes *serdes)
{
	return 0;
}

int max96752_panel_unprepare(struct serdes *serdes)
{
	return 0;
}

int max96752_panel_enable(struct serdes *serdes)
{
	return 0;
}

int max96752_panel_disable(struct serdes *serdes)
{
	return 0;
}

int max96752_panel_backlight_enable(struct serdes *serdes)
{
	return 0;
}

int max96752_panel_backlight_disable(struct serdes *serdes)
{
	return 0;
}

static struct serdes_chip_panel_ops max96752_panel_ops = {
	.prepare	= max96752_panel_prepare,
	.unprepare	= max96752_panel_unprepare,
	.enable		= max96752_panel_enable,
	.disable	= max96752_panel_disable,
	.backlight_enable	= max96752_panel_backlight_enable,
	.backlight_disable	= max96752_panel_backlight_disable,
};

struct serdes_chip_data serdes_max96752_data = {
	.name		= "max96752",
	.serdes_type	= TYPE_DES,
	.serdes_id	= MAXIM_ID_MAX96752,
	.panel_ops	= &max96752_panel_ops,
};
EXPORT_SYMBOL_GPL(serdes_max96752_data);
