// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <dm/pinctrl.h>
#include <regmap.h>
#include <syscon.h>

#include "pinctrl-rockchip.h"
#include <dt-bindings/pinctrl/rockchip.h>

static int rk3572_set_mux(struct rockchip_pin_bank *bank, int pin, int mux)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;
	int iomux_num = (pin / 8);
	struct regmap *regmap;
	int reg, mask;
	u8 bit;
	u32 data, rmask;

	regmap = priv->regmap_base;
	reg = bank->iomux[iomux_num].offset;
	if ((pin % 8) >= 4)
		reg += 0x4;
	bit = (pin % 4) * 4;
	mask = 0xf;

	data = (mask << (bit + 16));
	rmask = data | (data >> 16);
	data |= (mux & mask) << bit;

	if (bank->bank_num == 0 && pin >= RK_PB4 && pin <= RK_PB7)
		reg += 0x2000; /* GPIO0_IOC_GPIO0B_IOMUX_SEL_H */

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK3572_DRV_BITS_PER_PIN		4
#define RK3572_DRV_PINS_PER_REG		4
#define RK3572_DRV_GPIO0_OFFSET		0x100
#define RK3572_DRV_GPIO1_OFFSET		0x10120
#define RK3572_DRV_GPIO2_OFFSET		0x12140
#define RK3572_DRV_GPIO3_OFFSET		0x14160
#define RK3572_DRV_GPIO4_OFFSET		0x14180

static void rk3572_calc_drv_reg_and_bit(struct rockchip_pin_bank *bank,
					int pin_num, struct regmap **regmap,
					int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	switch (bank->bank_num) {
	case 0:
		*reg = RK3572_DRV_GPIO0_OFFSET;
		if (pin_num >= 12)
			*reg += 0x2000;
		break;
	case 1:
		*reg = RK3572_DRV_GPIO1_OFFSET;
		break;
	case 2:
		*reg = RK3572_DRV_GPIO2_OFFSET;
		break;
	case 3:
		*reg = RK3572_DRV_GPIO3_OFFSET;
		break;
	case 4:
		*reg = RK3572_DRV_GPIO4_OFFSET;
		if (pin_num >= 16)
			*reg -= 0x10000;
		break;
	default:
		*reg = 0;
		debug("unsupported bank_num %d\n", bank->bank_num);
		break;
	}

	*reg += ((pin_num / RK3572_DRV_PINS_PER_REG) * 4);
	*bit = pin_num % RK3572_DRV_PINS_PER_REG;
	*bit *= RK3572_DRV_BITS_PER_PIN;
}

static int rk3572_set_drive(struct rockchip_pin_bank *bank,
			    int pin_num, int strength)
{
	struct regmap *regmap;
	int reg;
	u32 data, rmask;
	u8 bit, ret;

	if ((bank->bank_num == 0 && pin_num < 12) ||
	    (bank->bank_num == 4 && (pin_num == 24 || pin_num == 25))) {
		/* only support 4 drive strength levels */
		if (strength > 3)
			return -EINVAL;
		ret = ((strength & BIT(1)) >> 1) | ((strength & BIT(0)) << 1);
	} else {
		ret = ((strength & BIT(2)) >> 2) |
		      ((strength & BIT(0)) << 2) |
		      (strength & BIT(1));
	}

	rk3572_calc_drv_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);

	/* enable the write to the equivalent lower bits */
	data = ((1 << RK3572_DRV_BITS_PER_PIN) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (ret << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK3572_PULL_BITS_PER_PIN	2
#define RK3572_PULL_PINS_PER_REG	8
#define RK3572_PULL_GPIO0_OFFSET	0x200
#define RK3572_PULL_GPIO1_OFFSET	0x10210
#define RK3572_PULL_GPIO2_OFFSET	0x12220
#define RK3572_PULL_GPIO3_OFFSET	0x14230
#define RK3572_PULL_GPIO4_OFFSET	0x14240

static void rk3572_calc_pull_reg_and_bit(struct rockchip_pin_bank *bank,
					 int pin_num, struct regmap **regmap,
					 int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	switch (bank->bank_num) {
	case 0:
		*reg = RK3572_PULL_GPIO0_OFFSET;
		if (pin_num >= 12)
			*reg += 0x2000;
		break;
	case 1:
		*reg = RK3572_PULL_GPIO1_OFFSET;
		break;
	case 2:
		*reg = RK3572_PULL_GPIO2_OFFSET;
		break;
	case 3:
		*reg = RK3572_PULL_GPIO3_OFFSET;
		break;
	case 4:
		*reg = RK3572_PULL_GPIO4_OFFSET;
		if (pin_num >= 16)
			*reg -= 0x10000;
		break;
	default:
		*reg = 0;
		debug("unsupported bank_num %d\n", bank->bank_num);
		break;
	}

	*reg += ((pin_num / RK3572_PULL_PINS_PER_REG) * 4);
	*bit = pin_num % RK3572_PULL_PINS_PER_REG;
	*bit *= RK3572_PULL_BITS_PER_PIN;
}

static int rk3572_set_pull(struct rockchip_pin_bank *bank,
			   int pin_num, int pull)
{
	struct regmap *regmap;
	int reg, ret;
	u8 bit, type;
	u32 data, rmask;

	if (pull == PIN_CONFIG_BIAS_PULL_PIN_DEFAULT)
		return -ENOTSUPP;

	rk3572_calc_pull_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);
	type = 1; /* FIXME: was always set to 1 in vendor kernel */
	ret = rockchip_translate_pull_value(type, pull);
	if (ret < 0) {
		debug("unsupported pull setting %d\n", pull);
		return ret;
	}

	/* enable the write to the equivalent lower bits */
	data = ((1 << RK3572_PULL_BITS_PER_PIN) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (ret << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK3572_SMT_BITS_PER_PIN		1
#define RK3572_SMT_PINS_PER_REG		8
#define RK3572_SMT_GPIO0_OFFSET		0x400
#define RK3572_SMT_GPIO1_OFFSET		0x10410
#define RK3572_SMT_GPIO2_OFFSET		0x12420
#define RK3572_SMT_GPIO3_OFFSET		0x14430
#define RK3572_SMT_GPIO4_OFFSET		0x14440

static void rk3572_calc_schmitt_reg_and_bit(struct rockchip_pin_bank *bank,
					    int pin_num,
					    struct regmap **regmap,
					    int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	switch (bank->bank_num) {
	case 0:
		*reg = RK3572_SMT_GPIO0_OFFSET;
		if (pin_num >= 12)
			*reg += 0x2000;
		break;
	case 1:
		*reg = RK3572_SMT_GPIO1_OFFSET;
		break;
	case 2:
		*reg = RK3572_SMT_GPIO2_OFFSET;
		break;
	case 3:
		*reg = RK3572_SMT_GPIO3_OFFSET;
		break;
	case 4:
		*reg = RK3572_SMT_GPIO4_OFFSET;
		if (pin_num >= 16)
			*reg -= 0x10000;
		break;
	default:
		*reg = 0;
		debug("unsupported bank_num %d\n", bank->bank_num);
		break;
	}

	*reg += ((pin_num / RK3572_SMT_PINS_PER_REG) * 4);
	*bit = pin_num % RK3572_SMT_PINS_PER_REG;
	*bit *= RK3572_SMT_BITS_PER_PIN;
}

static int rk3572_set_schmitt(struct rockchip_pin_bank *bank,
			      int pin_num, int enable)
{
	struct regmap *regmap;
	int reg;
	u32 data, rmask;
	u8 bit;

	rk3572_calc_schmitt_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);

	/* enable the write to the equivalent lower bits */
	data = ((1 << RK3572_SMT_BITS_PER_PIN) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (enable << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

static struct rockchip_pin_bank rk3572_pin_banks[] = {
	RK3576_PIN_BANK_FLAGS(0, 32, "gpio0", IOMUX_WIDTH_4BIT,
			      0, 0x8, 0x2010, 0x2018),
	RK3576_PIN_BANK_FLAGS(1, 32, "gpio1", IOMUX_WIDTH_4BIT,
			      0x10020, 0x10028, 0x10030, 0x10038),
	RK3576_PIN_BANK_FLAGS(2, 32, "gpio2", IOMUX_WIDTH_4BIT,
			      0x12040, 0x12048, 0x12050, 0x12058),
	RK3576_PIN_BANK_FLAGS(3, 32, "gpio3", IOMUX_WIDTH_4BIT,
			      0x14060, 0x14068, 0x14070, 0x14078),
	RK3576_PIN_BANK_FLAGS(4, 32, "gpio4", IOMUX_WIDTH_4BIT,
			      0x14080, 0x14088, 0x4090, 0x4098),
};

static const struct rockchip_pin_ctrl rk3572_pin_ctrl = {
	.pin_banks		= rk3572_pin_banks,
	.nr_banks		= ARRAY_SIZE(rk3572_pin_banks),
	.nr_pins		= 160,
	.grf_mux_offset		= 0x0,
	.set_mux		= rk3572_set_mux,
	.set_pull		= rk3572_set_pull,
	.set_drive		= rk3572_set_drive,
	.set_schmitt		= rk3572_set_schmitt,
};

static const struct udevice_id rk3572_pinctrl_ids[] = {
	{
		.compatible = "rockchip,rk3572-pinctrl",
		.data = (ulong)&rk3572_pin_ctrl
	},
	{ }
};

U_BOOT_DRIVER(pinctrl_rk3572) = {
	.name		= "rockchip_rk3572_pinctrl",
	.id		= UCLASS_PINCTRL,
	.of_match	= rk3572_pinctrl_ids,
	.priv_auto	= sizeof(struct rockchip_pinctrl_priv),
	.ops		= &rockchip_pinctrl_ops,
#if CONFIG_IS_ENABLED(OF_REAL)
	.bind		= dm_scan_fdt_dev,
#endif
	.probe		= rockchip_pinctrl_probe,
};
