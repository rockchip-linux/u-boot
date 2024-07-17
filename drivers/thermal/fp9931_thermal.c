// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <common.h>
#include <dm.h>
#include <thermal.h>
#include <power/pmic.h>
#include <power/fp9931.h>

DECLARE_GLOBAL_DATA_PTR;

static int fp9931_get_temp(struct udevice *dev, int *temp)
{
	struct udevice *pmic = dev_get_parent(dev);
	int ret;

	ret = pmic_reg_read(pmic, FP9931_TMST_VALUE);
	if (ret < 0)
		return ret;

	*temp = *((signed char *)&ret);

	return 0;
}

static const struct dm_thermal_ops fp9931_thermal_ops = {
	.get_temp = fp9931_get_temp,
};

static const struct udevice_id fp9931_thermal_of_match[] = {
	{ .compatible = FP9931_THERMAL_COMTATIBLE_NAME },
	{ }
};

U_BOOT_DRIVER(fp9931_thermal) = {
	.name		= FP9931_THERMAL_COMTATIBLE_NAME,
	.id		= UCLASS_THERMAL,
	.of_match	= fp9931_thermal_of_match,
	.ops		= &fp9931_thermal_ops,
};
