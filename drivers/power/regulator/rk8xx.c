/*
 * Copyright (C) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * Based on Rockchip's drivers/power/pmic/pmic_rk808.c:
 * Copyright (C) 2012 rockchips
 * zyw <zyw@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <power/rk8xx_pmic.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <led.h>

#ifndef CONFIG_SPL_BUILD
#define ENABLE_DRIVER
#endif

/* Field Definitions */
#define RK808_BUCK_VSEL_MASK	0x3f
#define RK808_BUCK4_VSEL_MASK	0xf
#define RK808_LDO_VSEL_MASK	0x1f

#define RK818_BUCK_VSEL_MASK		0x3f
#define RK818_BUCK4_VSEL_MASK		0x1f
#define RK818_LDO_VSEL_MASK		0x1f
#define RK818_LDO3_ON_VSEL_MASK	0xf
#define RK818_BOOST_ON_VSEL_MASK	0xe0
#define RK818_USB_ILIM_SEL_MASK		0x0f
#define RK818_USB_CHG_SD_VSEL_MASK	0x70

struct rk8xx_reg_info {
	uint min_uv;
	uint step_uv;
	s8 vsel_reg;
	s8 vsel_sleep_reg;
	u8 vsel_mask;
};

static const struct rk8xx_reg_info rk808_buck[] = {
	{ 712500,   12500, REG_BUCK1_ON_VSEL, REG_BUCK1_SLP_VSEL, RK808_BUCK_VSEL_MASK, },
	{ 712500,   12500, REG_BUCK2_ON_VSEL, REG_BUCK2_SLP_VSEL, RK808_BUCK_VSEL_MASK, },
	{ 712500,   12500, -1, -1, RK808_BUCK_VSEL_MASK, },
	{ 1800000, 100000, REG_BUCK4_ON_VSEL, REG_BUCK4_SLP_VSEL, RK808_BUCK4_VSEL_MASK, },
};

static const struct rk8xx_reg_info rk816_buck[] = {
	/* buck 1 */
	{  712500,  12500, REG_BUCK1_ON_VSEL, REG_BUCK1_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	{ 1800000, 200000, REG_BUCK1_ON_VSEL, REG_BUCK1_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	{ 2300000,      0, REG_BUCK1_ON_VSEL, REG_BUCK1_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	/* buck 2 */
	{  712500,  12500, REG_BUCK2_ON_VSEL, REG_BUCK2_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	{ 1800000, 200000, REG_BUCK2_ON_VSEL, REG_BUCK2_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	{ 2300000,      0, REG_BUCK2_ON_VSEL, REG_BUCK2_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	/* buck 3 */
	{ 712500, 12500, -1, -1, RK818_BUCK_VSEL_MASK, },
	/* buck 4 */
	{  800000, 100000, REG_BUCK4_ON_VSEL, REG_BUCK4_SLP_VSEL, RK818_BUCK4_VSEL_MASK, },
};

static const struct rk8xx_reg_info rk818_buck[] = {
	{ 712500,   12500, REG_BUCK1_ON_VSEL, REG_BUCK1_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	{ 712500,   12500, REG_BUCK2_ON_VSEL, REG_BUCK2_SLP_VSEL, RK818_BUCK_VSEL_MASK, },
	{ 712500,   12500, -1, -1, RK818_BUCK_VSEL_MASK, },
	{ 1800000, 100000, REG_BUCK4_ON_VSEL, REG_BUCK4_SLP_VSEL, RK818_BUCK4_VSEL_MASK, },
};

#ifdef ENABLE_DRIVER
static const struct rk8xx_reg_info rk808_ldo[] = {
	{ 1800000, 100000, REG_LDO1_ON_VSEL, REG_LDO1_SLP_VSEL, RK808_LDO_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO2_ON_VSEL, REG_LDO2_SLP_VSEL, RK808_LDO_VSEL_MASK, },
	{  800000, 100000, REG_LDO3_ON_VSEL, REG_LDO3_SLP_VSEL, RK808_BUCK4_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO4_ON_VSEL, REG_LDO4_SLP_VSEL, RK808_LDO_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO5_ON_VSEL, REG_LDO5_SLP_VSEL, RK808_LDO_VSEL_MASK, },
	{  800000, 100000, REG_LDO6_ON_VSEL, REG_LDO6_SLP_VSEL, RK808_LDO_VSEL_MASK, },
	{  800000, 100000, REG_LDO7_ON_VSEL, REG_LDO7_SLP_VSEL, RK808_LDO_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO8_ON_VSEL, REG_LDO8_SLP_VSEL, RK808_LDO_VSEL_MASK, },
};

static const struct rk8xx_reg_info rk816_ldo[] = {
	{ 800000, 100000, REG_LDO1_ON_VSEL, REG_LDO1_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 800000, 100000, REG_LDO2_ON_VSEL, REG_LDO2_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 800000, 100000, REG_LDO3_ON_VSEL, REG_LDO3_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 800000, 100000, REG_LDO4_ON_VSEL, REG_LDO4_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 800000, 100000, REG_LDO5_ON_VSEL, REG_LDO5_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 800000, 100000, REG_LDO6_ON_VSEL, REG_LDO6_SLP_VSEL, RK818_LDO_VSEL_MASK, },
};

static const struct rk8xx_reg_info rk818_ldo[] = {
	{ 1800000, 100000, REG_LDO1_ON_VSEL, REG_LDO1_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO2_ON_VSEL, REG_LDO2_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{  800000, 100000, REG_LDO3_ON_VSEL, REG_LDO3_SLP_VSEL, RK818_LDO3_ON_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO4_ON_VSEL, REG_LDO4_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO5_ON_VSEL, REG_LDO5_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{  800000, 100000, REG_LDO6_ON_VSEL, REG_LDO6_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{  800000, 100000, REG_LDO7_ON_VSEL, REG_LDO7_SLP_VSEL, RK818_LDO_VSEL_MASK, },
	{ 1800000, 100000, REG_LDO8_ON_VSEL, REG_LDO8_SLP_VSEL, RK818_LDO_VSEL_MASK, },
};
#endif

static const u16 rk818_chrg_cur_input_array[] = {
	450, 800, 850, 1000, 1250, 1500, 1750, 2000, 2250, 2500, 2750, 3000
};

static const uint rk818_chrg_shutdown_vsel_array[] = {
	2780000, 2850000, 2920000, 2990000, 3060000, 3130000, 3190000, 3260000
};

static const struct rk8xx_reg_info *get_buck_reg(struct udevice *pmic,
						 int num, int uvolt)
{
	struct rk8xx_priv *priv = dev_get_priv(pmic);

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		switch (num) {
		case 0:
		case 1:
			if (uvolt <= 1450000)
				return &rk816_buck[num * 3 + 0];
			else if (uvolt <= 2200000)
				return &rk816_buck[num * 3 + 1];
			else
				return &rk816_buck[num * 3 + 2];
		default:
			return &rk816_buck[num + 4];
		}
	case RK818_ID:
		return &rk818_buck[num];
	default:
		return &rk808_buck[num];
	}
}

static int _buck_set_value(struct udevice *pmic, int buck, int uvolt)
{
	const struct rk8xx_reg_info *info = get_buck_reg(pmic, buck, uvolt);
	int mask = info->vsel_mask;
	int val;

	if (info->vsel_reg == -1)
		return -ENOSYS;
	val = (uvolt - info->min_uv) / info->step_uv;
	debug("%s: reg=%x, mask=%x, val=%x\n", __func__, info->vsel_reg, mask,
	      val);

	return pmic_clrsetbits(pmic, info->vsel_reg, mask, val);
}

static int _buck_set_suspend_value(struct udevice *pmic, int buck, int uvolt)
{
	const struct rk8xx_reg_info *info = get_buck_reg(pmic, buck, uvolt);
	int mask = info->vsel_mask;
	int val;

	if (info->vsel_sleep_reg == -1)
		return -ENOSYS;
	val = (uvolt - info->min_uv) / info->step_uv;
	debug("%s: reg=%x, mask=%x, val=%x\n", __func__, info->vsel_sleep_reg, mask,
	      val);

	return pmic_clrsetbits(pmic, info->vsel_sleep_reg, mask, val);
}

static int _buck_get_enable(struct udevice *pmic, int buck)
{
	struct rk8xx_priv *priv = dev_get_priv(pmic);
	uint mask = 0;
	int ret = 0;

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		if (buck >= 4) {
			mask = 1 << (buck - 4);
			ret = pmic_reg_read(pmic, RK816_REG_DCDC_EN2);
		} else {
			mask = 1 << buck;
			ret = pmic_reg_read(pmic, RK816_REG_DCDC_EN1);
		}
		break;
	case RK808_ID:
	case RK818_ID:
		mask = 1 << buck;
		ret = pmic_reg_read(pmic, REG_DCDC_EN);
		if (ret < 0)
			return ret;
		break;
	}
	return ret & mask ? true : false;
}

static int _buck_set_enable(struct udevice *pmic, int buck, bool enable)
{
	uint mask, value, en_reg;
	int ret;
	struct rk8xx_priv *priv = dev_get_priv(pmic);

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		if (buck >= 4) {
			buck -= 4;
			en_reg = RK816_REG_DCDC_EN2;
		} else {
			en_reg = RK816_REG_DCDC_EN1;
		}
		if (enable)
			value = ((1 << buck) | (1 << (buck + 4)));
		else
			value = ((0 << buck) | (1 << (buck + 4)));
		ret = pmic_reg_write(pmic, en_reg, value);
		break;

	case RK808_ID:
	case RK818_ID:
		mask = 1 << buck;
		if (enable) {
			ret = pmic_clrsetbits(pmic, REG_DCDC_ILMAX,
					      0, 3 << (buck * 2));
			if (ret)
				return ret;
			ret = pmic_clrsetbits(pmic, REG_DCDC_UV_ACT,
					      1 << buck, 0);
			if (ret)
				return ret;
		}
		ret = pmic_clrsetbits(pmic, REG_DCDC_EN, mask,
				      enable ? mask : 0);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int _buck_set_suspend_enable(struct udevice *pmic, int buck, bool enable)
{
	uint mask;
	int ret;
	struct rk8xx_priv *priv = dev_get_priv(pmic);

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		mask = 1 << buck;
		ret = pmic_clrsetbits(pmic, RK816_REG_DCDC_SLP_EN, mask,
				      enable ? mask : 0);
		break;

	case RK808_ID:
	case RK818_ID:
		mask = 1 << buck;
		ret = pmic_clrsetbits(pmic, REG_SLEEP_SET_OFF1, mask,
				      enable ? 0 : mask);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

#ifdef ENABLE_DRIVER
static const struct rk8xx_reg_info *get_ldo_reg(struct udevice *pmic,
					     int num)
{
	struct rk8xx_priv *priv = dev_get_priv(pmic);

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		return &rk816_ldo[num];
	case RK818_ID:
		return &rk818_ldo[num];
	default:
		return &rk808_ldo[num];
	}
}

static int _ldo_get_enable(struct udevice *pmic, int ldo)
{
	struct rk8xx_priv *priv = dev_get_priv(pmic);
	uint mask = 0;
	int ret = 0;

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		if (ldo >= 4) {
			mask = 1 << (ldo - 4);
			ret = pmic_reg_read(pmic, RK816_REG_LDO_EN2);
		} else {
			mask = 1 << ldo;
			ret = pmic_reg_read(pmic, RK816_REG_LDO_EN1);
		}
		break;
	case RK808_ID:
	case RK818_ID:
		mask = 1 << ldo;
		ret = pmic_reg_read(pmic, REG_LDO_EN);
		if (ret < 0)
			return ret;
		break;
	}
	return ret & mask ? true : false;
}

static int _ldo_set_enable(struct udevice *pmic, int ldo, bool enable)
{
	struct rk8xx_priv *priv = dev_get_priv(pmic);
	uint mask, value, en_reg;
	int ret = 0;

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		if (ldo >= 4) {
			ldo -= 4;
			en_reg = RK816_REG_LDO_EN2;
		} else {
			en_reg = RK816_REG_LDO_EN1;
		}
		if (enable)
			value = ((1 << ldo) | (1 << (ldo + 4)));
		else
			value = ((0 << ldo) | (1 << (ldo + 4)));

		ret = pmic_reg_write(pmic, en_reg, value);
		break;
	case RK808_ID:
	case RK818_ID:
		mask = 1 << ldo;
		ret = pmic_clrsetbits(pmic, REG_LDO_EN, mask,
				       enable ? mask : 0);
		break;
	}

	return ret;
}

static int _ldo_set_suspend_enable(struct udevice *pmic, int ldo, bool enable)
{
	struct rk8xx_priv *priv = dev_get_priv(pmic);
	uint mask;
	int ret = 0;

	switch (priv->variant) {
	case RK805_ID:
	case RK816_ID:
		mask = 1 << ldo;
		ret = pmic_clrsetbits(pmic, RK816_REG_LDO_SLP_EN, mask,
				      enable ? mask : 0);
		break;
	case RK808_ID:
	case RK818_ID:
		mask = 1 << ldo;
		ret = pmic_clrsetbits(pmic, REG_SLEEP_SET_OFF2, mask,
				      enable ? 0 : mask);
		break;
	}

	return ret;
}

static int buck_get_value(struct udevice *dev)
{
	int buck = dev->driver_data - 1;
	const struct rk8xx_reg_info *info = get_buck_reg(dev->parent, buck, 0);
	int mask = info->vsel_mask;
	int ret, val;

	if (info->vsel_reg == -1)
		return -ENOSYS;
	ret = pmic_reg_read(dev->parent, info->vsel_reg);
	if (ret < 0)
		return ret;
	val = ret & mask;

	return info->min_uv + val * info->step_uv;
}

static int buck_set_value(struct udevice *dev, int uvolt)
{
	int buck = dev->driver_data - 1;

	return _buck_set_value(dev->parent, buck, uvolt);
}

static int buck_set_suspend_value(struct udevice *dev, int uvolt)
{
	int buck = dev->driver_data - 1;

	return _buck_set_suspend_value(dev->parent, buck, uvolt);
}

static int buck_set_enable(struct udevice *dev, bool enable)
{
	int buck = dev->driver_data - 1;

	return _buck_set_enable(dev->parent, buck, enable);
}

static int buck_set_suspend_enable(struct udevice *dev, bool enable)
{
	int buck = dev->driver_data - 1;

	return _buck_set_suspend_enable(dev->parent, buck, enable);
}

static int buck_get_enable(struct udevice *dev)
{
	int buck = dev->driver_data - 1;

	return _buck_get_enable(dev->parent, buck);
}

static int ldo_get_value(struct udevice *dev)
{
	int ldo = dev->driver_data - 1;
	const struct rk8xx_reg_info *info = get_ldo_reg(dev->parent, ldo);
	int mask = info->vsel_mask;
	int ret, val;

	if (info->vsel_reg == -1)
		return -ENOSYS;
	ret = pmic_reg_read(dev->parent, info->vsel_reg);
	if (ret < 0)
		return ret;
	val = ret & mask;

	return info->min_uv + val * info->step_uv;
}

static int ldo_set_value(struct udevice *dev, int uvolt)
{
	int ldo = dev->driver_data - 1;
	const struct rk8xx_reg_info *info = get_ldo_reg(dev->parent, ldo);
	int mask = info->vsel_mask;
	int val;

	if (info->vsel_reg == -1)
		return -ENOSYS;
	val = (uvolt - info->min_uv) / info->step_uv;
	debug("%s: reg=%x, mask=%x, val=%x\n", __func__, info->vsel_reg, mask,
	      val);

	return pmic_clrsetbits(dev->parent, info->vsel_reg, mask, val);
}

static int ldo_set_suspend_value(struct udevice *dev, int uvolt)
{
	int ldo = dev->driver_data - 1;
	const struct rk8xx_reg_info *info = get_ldo_reg(dev->parent, ldo);
	int mask = info->vsel_mask;
	int val;

	if (info->vsel_sleep_reg == -1)
		return -ENOSYS;
	val = (uvolt - info->min_uv) / info->step_uv;
	debug("%s: reg=%x, mask=%x, val=%x\n", __func__, info->vsel_sleep_reg, mask,
	      val);

	return pmic_clrsetbits(dev->parent, info->vsel_sleep_reg, mask, val);
}

static int ldo_set_enable(struct udevice *dev, bool enable)
{
	int ldo = dev->driver_data - 1;

	return _ldo_set_enable(dev->parent, ldo, enable);
}

static int ldo_set_suspend_enable(struct udevice *dev, bool enable)
{
	int ldo = dev->driver_data - 1;

	return _ldo_set_suspend_enable(dev->parent, ldo, enable);
}

static int ldo_get_enable(struct udevice *dev)
{
	int ldo = dev->driver_data - 1;

	return _ldo_get_enable(dev->parent, ldo);
}

static int switch_set_enable(struct udevice *dev, bool enable)
{
	int sw = dev->driver_data - 1;
	uint mask;

	mask = 1 << (sw + 5);

	return pmic_clrsetbits(dev->parent, REG_DCDC_EN, mask,
			       enable ? mask : 0);
}

static int switch_get_enable(struct udevice *dev)
{
	int sw = dev->driver_data - 1;
	int ret;
	uint mask;

	mask = 1 << (sw + 5);

	ret = pmic_reg_read(dev->parent, REG_DCDC_EN);
	if (ret < 0)
		return ret;

	return ret & mask ? true : false;
}

static enum led_state_t rk8xx_led_get_state(struct udevice *dev)
{
	int sw = dev->driver_data - 1;
	int ret;
	uint mask;

	mask = 1 << sw;

	ret = pmic_reg_read(dev->parent, REG_OUT_CTRL);
	if (ret < 0)
		return ret;

	return ret & mask ? LEDST_OFF : LEDST_ON;
}

static int rk8xx_led_set_state(struct udevice *dev, enum led_state_t state)
{
	int sw = dev->driver_data - 1;
	uint mask;

	mask = 1 << sw;

	if (state == LEDST_TOGGLE) {
		if (rk8xx_led_get_state(dev) == LEDST_ON)
			state = LEDST_OFF;
		else
			state = LEDST_ON;
	}

	return pmic_clrsetbits(dev->parent, REG_OUT_CTRL, mask,
			       state ? 0 : mask);
}

static int rk8xx_buck_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_platdata *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);

	uc_pdata->type = REGULATOR_TYPE_BUCK;
	uc_pdata->mode_count = 0;

	return 0;
}

static int rk8xx_ldo_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_platdata *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);

	uc_pdata->type = REGULATOR_TYPE_LDO;
	uc_pdata->mode_count = 0;

	return 0;
}

static int rk8xx_switch_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_platdata *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);

	uc_pdata->type = REGULATOR_TYPE_FIXED;
	uc_pdata->mode_count = 0;

	return 0;
}

static int rk8xx_led_bind(struct udevice *dev)
{
	struct led_uc_plat *uc_pdata;

	uc_pdata = dev_get_uclass_platdata(dev);
	uc_pdata->label = dev_read_string(dev, "label");

	return 0;
}

static int rk8xx_led_probe(struct udevice *dev)
{
	return 0;
}

static const struct dm_regulator_ops rk8xx_buck_ops = {
	.get_value  = buck_get_value,
	.set_value  = buck_set_value,
	.set_suspend_value = buck_set_suspend_value,
	.get_enable = buck_get_enable,
	.set_enable = buck_set_enable,
	.set_suspend_enable = buck_set_suspend_enable,
};

static const struct dm_regulator_ops rk8xx_ldo_ops = {
	.get_value  = ldo_get_value,
	.set_value  = ldo_set_value,
	.set_suspend_value = ldo_set_suspend_value,
	.get_enable = ldo_get_enable,
	.set_enable = ldo_set_enable,
	.set_suspend_enable = ldo_set_suspend_enable,
};

static const struct dm_regulator_ops rk8xx_switch_ops = {
	.get_enable = switch_get_enable,
	.set_enable = switch_set_enable,
};

static const struct led_ops rk8xx_led_ops = {
	.get_state = rk8xx_led_get_state,
	.set_state = rk8xx_led_set_state,
};

U_BOOT_DRIVER(rk8xx_buck) = {
	.name = "rk8xx_buck",
	.id = UCLASS_REGULATOR,
	.ops = &rk8xx_buck_ops,
	.probe = rk8xx_buck_probe,
};

U_BOOT_DRIVER(rk8xx_ldo) = {
	.name = "rk8xx_ldo",
	.id = UCLASS_REGULATOR,
	.ops = &rk8xx_ldo_ops,
	.probe = rk8xx_ldo_probe,
};

U_BOOT_DRIVER(rk8xx_switch) = {
	.name = "rk8xx_switch",
	.id = UCLASS_REGULATOR,
	.ops = &rk8xx_switch_ops,
	.probe = rk8xx_switch_probe,
};

U_BOOT_DRIVER(rk8xx_led) = {
	.name = "rk8xx_led",
	.id = UCLASS_LED,
	.ops = &rk8xx_led_ops,
	.bind = rk8xx_led_bind,
	.probe = rk8xx_led_probe,
};
#endif

int rk8xx_spl_configure_buck(struct udevice *pmic, int buck, int uvolt)
{
	int ret;

	ret = _buck_set_value(pmic, buck, uvolt);
	if (ret)
		return ret;

	return _buck_set_enable(pmic, buck, true);
}

int rk818_spl_configure_usb_input_current(struct udevice *pmic, int current_ma)
{
	uint i;

	for (i = 0; i < ARRAY_SIZE(rk818_chrg_cur_input_array); i++)
		if (current_ma <= rk818_chrg_cur_input_array[i])
			break;

	return pmic_clrsetbits(pmic, REG_USB_CTRL, RK818_USB_ILIM_SEL_MASK, i);
}

int rk818_spl_configure_usb_chrg_shutdown(struct udevice *pmic, int uvolt)
{
	uint i;

	for (i = 0; i < ARRAY_SIZE(rk818_chrg_shutdown_vsel_array); i++)
		if (uvolt <= rk818_chrg_shutdown_vsel_array[i])
			break;

	return pmic_clrsetbits(pmic, REG_USB_CTRL, RK818_USB_CHG_SD_VSEL_MASK,
			       i);
}
