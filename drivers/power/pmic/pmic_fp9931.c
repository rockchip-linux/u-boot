// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <common.h>
#include <asm/gpio.h>
#include <dm.h>
#include <dm/lists.h>
#include <dm/device-internal.h>
#include <dm/of_access.h>
#include <dm/pinctrl.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/fp9931.h>

DECLARE_GLOBAL_DATA_PTR;

static const struct pmic_child_info pmic_children_info[] = {
	{ .prefix = "vcom", .driver = FP9931_VCOM_DRIVER_NAME },
	{ .prefix = "vpos_vneg", .driver = FP9931_VPOS_VNEG_DRIVER_NAME },
	{ },
};

static const struct pmic_child_info thermal_child_info[] = {
	{ .prefix = "fp9931_thermal", .driver = FP9931_THERMAL_COMTATIBLE_NAME },
	{ },
};

static int fp9931_reg_count(struct udevice *dev)
{
        return fp9931_REG_MAX;
}

static int fp9931_write(struct udevice *dev, uint reg, const uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_write(dev, reg, buff, len);
	if (ret) {
		pr_err("fp9931 failed to write register: %#x, ret:%d\n", reg, ret);
		return ret;
	}

	return 0;
}

static int fp9931_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_read(dev, reg, buff, len);
	if (ret) {
		pr_err("fp9931 failed to write register: %#x, ret:%d\n", reg, ret);
		return ret;
	}

	return 0;
}

static int pmic_fp9931_probe(struct udevice *dev)
{
	struct fp9931_plat_data *data = dev_get_platdata(dev);
	uint8_t val;
	int ret;

	ret = gpio_request_list_by_name(dev, "power-gpios", data->power_gpio, 4,
					GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	if (ret < 0)
		dev_warn(dev, "fp9931 failed to get power gpios:%d\n", ret);

	data->num_power_gpio = ret;

	ret = gpio_request_by_name(dev, "enable-gpios", 0, &data->enable_gpio, GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "fp9931 failed to get enable gpios:%d\n", ret);
		return ret;
	}

	/* After power on, fp9931 requires 1ms delay time to enter active mode */
	udelay(1100);

	/* check is device i2c present */
	ret = dm_i2c_read(dev, FP9931_VCOM_SETTING, &val, 1);
	if (ret) {
		dev_warn(dev, "fp9931 i2c not present: %d\n", ret);
		return ret;
	}

	return 0;
}

static int pmic_fp9931_bind(struct udevice *dev)
{
	ofnode regulators_node;
	int children;

	regulators_node = dev_read_subnode(dev, "regulators");
	if (!ofnode_valid(regulators_node)) {
		dev_err(dev, "Regulators subnode not found!");
		return -ENXIO;
	}

	children = pmic_bind_children(dev, regulators_node, pmic_children_info);
	if (!children)
		dev_err(dev, "Failed to bind fp9931 regulator\n");

	children = pmic_bind_children(dev, dev->node, thermal_child_info);
	if (!children)
		dev_err(dev, "Failed to bind fp9931 thermal\n");

        return 0;
}

static struct dm_pmic_ops fp9931_ops = {
	.reg_count = fp9931_reg_count,
	.read = fp9931_read,
	.write = fp9931_write,
};

static const struct udevice_id pmic_fp9931_of_match[] = {
	{ .compatible = "fitipower,fp9931-pmic" },
	{ }
};

U_BOOT_DRIVER(pmic_fp9931) = {
	.name = "pmic_fp9931",
	.id = UCLASS_PMIC,
	.of_match = pmic_fp9931_of_match,
	.probe = pmic_fp9931_probe,
	.ops = &fp9931_ops,
	.bind = pmic_fp9931_bind,
	.platdata_auto_alloc_size = sizeof(struct fp9931_plat_data),
};
