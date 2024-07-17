// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <common.h>
#include <dm.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/fp9931.h>

DECLARE_GLOBAL_DATA_PTR;

/* in uV */
const static int fp9931_vpos_vneg_voltages[] = {
	7040000, 7040000, 7040000, 7040000, 7040000,
	7260000, 7490000, 7710000, 7930000, 8150000,
	8380000, 8600000, 8820000, 9040000, 9270000,
	9490000, 9710000, 9940000, 10160000, 10380000,
	10600000, 10830000, 11050000, 11270000, 11490000,
	11720000, 11940000, 12160000, 12380000, 12610000,
	12830000, 13050000, 13280000, 13500000, 13720000,
	13940000, 14170000, 14390000, 14610000, 14830000,
	15060000
};

static int fp9931_vcom_set_enable(struct udevice *dev, bool enable)
{
	struct udevice *pmic = dev_get_parent(dev);
	struct fp9931_plat_data *data = dev_get_platdata(pmic);
	int ret;

	if (enable) {
		ret = pmic_clrsetbits(pmic, FP9931_CONTROL_REG1, 0, CONTROL_REG1_V3P3_EN);
		if (ret)
			return ret;
	}

	/* V3P3 is auto off when EN pin disable */
	ret = dm_gpio_set_value(&data->enable_gpio, enable);

	/* It takes about 55 ms at most for i2c to become available again. */
	if (enable)
		mdelay(55);

	return ret;
}

/* VCOM = 0V + [(-5 / 255) * N]V, N = 1~255 (0 is meaningless) */
static int fp9931_vcom_set_value(struct udevice *dev, int uV)
{
	struct udevice *pmic = dev_get_parent(dev);
	u32 val;
	int ret;

	val = 255ul * uV;
	val /= 5000000;

	if (val == 0)
		val = 1;
	else if (val > 255)
		val = 255;

	ret = pmic_reg_write(pmic, FP9931_VCOM_SETTING, val);

	return ret;
}

static int fp9931_regulator_get_enable(struct udevice *dev)
{
	struct udevice *pmic = dev_get_parent(dev);
	struct fp9931_plat_data *data = dev_get_platdata(pmic);
	int ret;

	ret = dm_gpio_get_value(&data->enable_gpio);
	if (ret < 0)
		return ret;

	return !!ret;
}

static int fp9931_vpos_vneg_set_enable(struct udevice *dev, bool enable)
{
	return 0;
}

static int fp9931_vpos_vneg_set_value(struct udevice *dev, int uV)
{
	struct udevice *pmic = dev_get_parent(dev);
	int i, ret, val;

	for (i = 0; i < ARRAY_SIZE(fp9931_vpos_vneg_voltages); i++) {
		if (fp9931_vpos_vneg_voltages[i] > uV)
			break;
	}

	if (--i < 0)
		i = 0;

	ret = pmic_reg_read(pmic, FP9931_VPOS_VNEG_SETTING);
	if (ret < 0)
		return ret;

	val = (ret & 0xFF) & (~VPOS_VNEG_SETTING);
	val |= i;

	ret = pmic_reg_write(pmic, FP9931_VPOS_VNEG_SETTING, val);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct dm_regulator_ops fp9931_vcom_ops = {
	.get_enable = fp9931_regulator_get_enable,
	.set_enable = fp9931_vcom_set_enable,
	.set_value = fp9931_vcom_set_value,
};

U_BOOT_DRIVER(fp9931_vcom) = {
	.name = FP9931_VCOM_DRIVER_NAME,
	.id = UCLASS_REGULATOR,
	.ops = &fp9931_vcom_ops,
};

static const struct dm_regulator_ops fp9931_vpos_vneg_ops = {
	.get_enable = fp9931_regulator_get_enable,
	.set_enable = fp9931_vpos_vneg_set_enable,
	.set_value = fp9931_vpos_vneg_set_value,
};

U_BOOT_DRIVER(fp9931_vpos_vneg) = {
	.name = FP9931_VPOS_VNEG_DRIVER_NAME,
	.id = UCLASS_REGULATOR,
	.ops = &fp9931_vpos_vneg_ops,
};
