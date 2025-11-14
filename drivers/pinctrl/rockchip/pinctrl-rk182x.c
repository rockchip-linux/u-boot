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

static int rk182x_set_mux(struct rockchip_pin_bank *bank, int pin, int mux)
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

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK182X_DRV_BITS_PER_PIN		8
#define RK182X_DRV_PINS_PER_REG		2
#define RK182X_DRV_GPIO0_OFFSET		0x100
#define RK182X_DRV_GPIO1_OFFSET		0x140

static void rk182x_calc_drv_reg_and_bit(struct rockchip_pin_bank *bank,
					int pin_num, struct regmap **regmap,
					int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	if (bank->bank_num == 0)
		*reg = RK182X_DRV_GPIO0_OFFSET;
	else if (bank->bank_num == 1)
		*reg = RK182X_DRV_GPIO1_OFFSET;
	else
		debug("unsupported bank_num %d\n", bank->bank_num);

	*reg += ((pin_num / RK182X_DRV_PINS_PER_REG) * 4);
	*bit = pin_num % RK182X_DRV_PINS_PER_REG;
	*bit *= RK182X_DRV_BITS_PER_PIN;
}

static int rk182x_set_drive(struct rockchip_pin_bank *bank,
			    int pin_num, int strength)
{
	struct regmap *regmap;
	int reg;
	u32 data, rmask;
	u8 bit;

	rk182x_calc_drv_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);

	/* enable the write to the equivalent lower bits */
	data = ((1 << RK182X_DRV_BITS_PER_PIN) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (strength << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK182X_PULL_BITS_PER_PIN	2
#define RK182X_PULL_PINS_PER_REG	8
#define RK182X_PULL_GPIO0_OFFSET	0x200
#define RK182X_PULL_GPIO1_OFFSET	0x210

static void rk182x_calc_pull_reg_and_bit(struct rockchip_pin_bank *bank,
					 int pin_num, struct regmap **regmap,
					 int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	if (bank->bank_num == 0)
		*reg = RK182X_PULL_GPIO0_OFFSET;
	else if (bank->bank_num == 1)
		*reg = RK182X_PULL_GPIO1_OFFSET;
	else
		debug("unsupported bank_num %d\n", bank->bank_num);

	*reg += ((pin_num / RK182X_PULL_PINS_PER_REG) * 4);
	*bit = pin_num % RK182X_PULL_PINS_PER_REG;
	*bit *= RK182X_PULL_BITS_PER_PIN;
}

static int rk182x_set_pull(struct rockchip_pin_bank *bank,
			   int pin_num, int pull)
{
	struct regmap *regmap;
	int reg, ret;
	u8 bit, type;
	u32 data, rmask;

	if (pull == PIN_CONFIG_BIAS_PULL_PIN_DEFAULT)
		return -ENOTSUPP;

	rk182x_calc_pull_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);
	type = bank->pull_type[pin_num / 8];
	ret = rockchip_translate_pull_value(type, pull);
	if (ret < 0) {
		debug("unsupported pull setting %d\n", pull);
		return ret;
	}

	/* enable the write to the equivalent lower bits */
	data = ((1 << RK182X_PULL_BITS_PER_PIN) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (ret << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK182X_SMT_BITS_PER_PIN		1
#define RK182X_SMT_PINS_PER_REG		8
#define RK182X_SMT_GPIO0_OFFSET		0x400
#define RK182X_SMT_GPIO1_OFFSET		0x410

static void rk182x_calc_schmitt_reg_and_bit(struct rockchip_pin_bank *bank,
					    int pin_num,
					    struct regmap **regmap,
					    int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	if (bank->bank_num == 0)
		*reg = RK182X_SMT_GPIO0_OFFSET;
	else if (bank->bank_num == 1)
		*reg = RK182X_SMT_GPIO1_OFFSET;
	else
		debug("unsupported bank_num %d\n", bank->bank_num);

	*reg += ((pin_num / RK182X_SMT_PINS_PER_REG) * 4);
	*bit = pin_num % RK182X_SMT_PINS_PER_REG;
	*bit *= RK182X_SMT_BITS_PER_PIN;
}

static int rk182x_set_schmitt(struct rockchip_pin_bank *bank,
			      int pin_num, int enable)
{
	struct regmap *regmap;
	int reg;
	u32 data, rmask;
	u8 bit;

	rk182x_calc_schmitt_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);

	/* enable the write to the equivalent lower bits */
	data = ((1 << RK182X_SMT_BITS_PER_PIN) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (enable << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

static struct rockchip_pin_bank rk182x_pin_banks[] = {
	PIN_BANK_IOMUX_4_OFFSET(0, 32, "gpio0", 0x0, 0x8, 0x10, 0x18),
	PIN_BANK_IOMUX_4_OFFSET(1, 32, "gpio1", 0x20, 0x28, 0x30, 0x38),
};

static const struct rockchip_pin_ctrl rk182x_pin_ctrl = {
	.pin_banks		= rk182x_pin_banks,
	.nr_banks		= ARRAY_SIZE(rk182x_pin_banks),
	.nr_pins		= 64,
	.grf_mux_offset		= 0x0,
	.set_mux		= rk182x_set_mux,
	.set_pull		= rk182x_set_pull,
	.set_drive		= rk182x_set_drive,
	.set_schmitt		= rk182x_set_schmitt,
};

static const struct udevice_id rk182x_pinctrl_ids[] = {
	{
		.compatible = "rockchip,rk182x-pinctrl",
		.data = (ulong)&rk182x_pin_ctrl
	},
	{ }
};

U_BOOT_DRIVER(pinctrl_rk182x) = {
	.name		= "rockchip_rk182x_pinctrl",
	.id		= UCLASS_PINCTRL,
	.of_match	= rk182x_pinctrl_ids,
	.priv_auto	= sizeof(struct rockchip_pinctrl_priv),
	.ops		= &rockchip_pinctrl_ops,
#if CONFIG_IS_ENABLED(OF_REAL)
	.bind		= dm_scan_fdt_dev,
#endif
	.probe		= rockchip_pinctrl_probe,
};
