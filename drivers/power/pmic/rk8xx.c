/*
 * Copyright (C) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <power/rk8xx_pmic.h>
#include <power/pmic.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Only when system suspend while U-Boot charge needs this config support
 */
#ifdef CONFIG_DM_CHARGE_DISPLAY
static struct reg_data rk817_init_reg[] = {
	/* Set pmic_sleep as sleep function */
	{ RK817_PMIC_SYS_CFG3, 0x08, 0x18 },
	/* Set pmic_int active low */
	{ RK817_GPIO_INT_CFG,  0x00, 0x02 },
};
#endif

static const struct pmic_child_info pmic_children_info[] = {
	{ .prefix = "DCDC_REG", .driver = "rk8xx_buck"},
	{ .prefix = "LDO_REG", .driver = "rk8xx_ldo"},
	{ .prefix = "SWITCH_REG", .driver = "rk8xx_switch"},
	{ },
};

static const struct pmic_child_info power_key_info[] = {
	{ .prefix = "pwrkey", .driver = "rk8xx_pwrkey"},
	{ },
};

static const struct pmic_child_info fuel_gauge_info[] = {
	{ .prefix = "battery", .driver = "rk818_fg"},
	{ .prefix = "battery", .driver = "rk817_fg"},
	{ .prefix = "battery", .driver = "rk816_fg"},
	{ },
};

static int rk8xx_reg_count(struct udevice *dev)
{
	return RK808_NUM_OF_REGS;
}

static int rk8xx_write(struct udevice *dev, uint reg, const uint8_t *buff,
			  int len)
{
	int ret;

	ret = dm_i2c_write(dev, reg, buff, len);
	if (ret) {
		printf("%s: write reg 0x%02x failed, ret=%d\n", __func__, reg, ret);
		return ret;
	}

	return 0;
}

static int rk8xx_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	int ret;

	ret = dm_i2c_read(dev, reg, buff, len);
	if (ret) {
		printf("%s: read reg 0x%02x failed, ret=%d\n", __func__, reg, ret);
		return ret;
	}

	return 0;
}

static int rk8xx_shutdown(struct udevice *dev)
{
	struct rk8xx_priv *priv = dev_get_priv(dev);
	u8 val, dev_off, devctrl_reg;
	int ret = 0;

	switch (priv->variant) {
	case RK808_ID:
		devctrl_reg = REG_DEVCTRL;
		dev_off = BIT(3);
		break;
	case RK805_ID:
	case RK816_ID:
	case RK818_ID:
		devctrl_reg = REG_DEVCTRL;
		dev_off = BIT(0);
		break;
	case RK809_ID:
	case RK817_ID:
		devctrl_reg = RK817_REG_SYS_CFG3;
		dev_off = BIT(0);
		break;
	default:
		printf("Unknown PMIC: RK%x\n", priv->variant);
		return -EINVAL;
	}

	ret = dm_i2c_read(dev, devctrl_reg, &val, 1);
	if (ret) {
		printf("%s: read reg 0x%02x failed, ret=%d\n", __func__, devctrl_reg, ret);
		return ret;
	}

	val |= dev_off;
	ret = dm_i2c_write(dev, devctrl_reg, &val, 1);
	if (ret) {
		printf("%s: write reg 0x%02x failed, ret=%d\n", __func__, devctrl_reg, ret);
		return ret;
	}

	return 0;
}

#if CONFIG_IS_ENABLED(PMIC_CHILDREN)
static int rk8xx_bind(struct udevice *dev)
{
	ofnode regulators_node;
	int children;

	regulators_node = dev_read_subnode(dev, "regulators");
	if (!ofnode_valid(regulators_node)) {
		debug("%s: %s regulators subnode not found!\n", __func__,
		      dev->name);
		return -ENXIO;
	}

	debug("%s: '%s' - found regulators subnode\n", __func__, dev->name);

	children = pmic_bind_children(dev, regulators_node, pmic_children_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	children = pmic_bind_children(dev, dev->node, power_key_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	children = pmic_bind_children(dev, dev->node, fuel_gauge_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	/* Always return success for this device */
	return 0;
}
#endif

static int rk8xx_probe(struct udevice *dev)
{
	struct rk8xx_priv *priv = dev_get_priv(dev);
	struct reg_data *init_data = NULL;
	int init_data_num = 0;
	int ret = 0, i, show_variant;
	uint8_t msb, lsb, id_msb, id_lsb;
	uint8_t on_source = 0, off_source = 0;

	/* read Chip variant */
	if (device_is_compatible(dev, "rockchip,rk817") ||
	    device_is_compatible(dev, "rockchip,rk809")) {
		id_msb = RK817_ID_MSB;
		id_lsb = RK817_ID_LSB;
	} else {
		id_msb = ID_MSB;
		id_lsb = ID_LSB;
	}

	ret = rk8xx_read(dev, id_msb, &msb, 1);
	if (ret)
		return ret;
	ret = rk8xx_read(dev, id_lsb, &lsb, 1);
	if (ret)
		return ret;

	priv->variant = ((msb << 8) | lsb) & RK8XX_ID_MSK;
	show_variant = priv->variant;
	switch (priv->variant) {
	case RK808_ID:
		show_variant = 0x808;	/* RK808 hardware ID is 0 */
		break;
	case RK805_ID:
	case RK816_ID:
	case RK818_ID:
		on_source = RK8XX_ON_SOURCE;
		off_source = RK8XX_OFF_SOURCE;
		break;
	case RK809_ID:
	case RK817_ID:
		on_source = RK817_ON_SOURCE;
		off_source = RK817_OFF_SOURCE;
#ifdef CONFIG_DM_CHARGE_DISPLAY
		init_data = rk817_init_reg;
		init_data_num = ARRAY_SIZE(rk817_init_reg);
#endif
		break;
	default:
		printf("Unknown PMIC: RK%x!!\n", priv->variant);
		return -EINVAL;
	}

	for (i = 0; i < init_data_num; i++) {
		ret = pmic_clrsetbits(dev,
				      init_data[i].reg,
				      init_data[i].mask,
				      init_data[i].val);
		if (ret < 0) {
			printf("%s: i2c set reg 0x%x failed, ret=%d\n",
			       __func__, init_data[i].reg, ret);
		}

		debug("%s: reg[0x%x] = 0x%x\n", __func__, init_data[i].reg,
		      pmic_reg_read(dev, init_data[i].reg));
	}

	printf("PMIC:  RK%x ", show_variant);

	if (on_source && off_source) {
		on_source = pmic_reg_read(dev, on_source);
		off_source = pmic_reg_read(dev, off_source);
		printf("(on=0x%02x, off=0x%02x)",
		       pmic_reg_read(dev, on_source),
		       pmic_reg_read(dev, off_source));
	}
	printf("\n");

	return 0;
}

static struct dm_pmic_ops rk8xx_ops = {
	.reg_count = rk8xx_reg_count,
	.read = rk8xx_read,
	.write = rk8xx_write,
	.shutdown = rk8xx_shutdown,
};

static const struct udevice_id rk8xx_ids[] = {
	{ .compatible = "rockchip,rk805" },
	{ .compatible = "rockchip,rk808" },
	{ .compatible = "rockchip,rk809" },
	{ .compatible = "rockchip,rk816" },
	{ .compatible = "rockchip,rk817" },
	{ .compatible = "rockchip,rk818" },
	{ }
};

U_BOOT_DRIVER(pmic_rk8xx) = {
	.name = "rk8xx pmic",
	.id = UCLASS_PMIC,
	.of_match = rk8xx_ids,
#if CONFIG_IS_ENABLED(PMIC_CHILDREN)
	.bind = rk8xx_bind,
#endif
	.priv_auto_alloc_size   = sizeof(struct rk8xx_priv),
	.probe = rk8xx_probe,
	.ops = &rk8xx_ops,
};
