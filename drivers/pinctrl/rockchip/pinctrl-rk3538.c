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

static int rk3538_set_mux(struct rockchip_pin_bank *bank, int pin, int mux)
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

#define RK3538_DRV_BITS_PER_PIN		8
#define RK3538_DRV_PINS_PER_REG		2
#define RK3538_DRV_GPIO0_A_OFFSET	0x100
#define RK3538_DRV_GPIO0_D_OFFSET	0x10100
#define RK3538_DRV_GPIO0_5VIO_0_OFFSET	0x900
#define RK3538_DRV_GPIO0_5VIO_1_OFFSET	0x904
#define RK3538_DRV_GPIO1_OFFSET		0x20140
#define RK3538_DRV_GPIO2_OFFSET		0x30180
#define RK3538_DRV_GPIO3_OFFSET		0x401c0
#define RK3538_DRV_GPIO4_OFFSET		0x50200
#define RK3538_DRV_GPIO5_OFFSET		0x60240
#define RK3538_DRV_GPIO6_OFFSET		0x70280

static void rk3538_calc_drv_reg_and_bit(struct rockchip_pin_bank *bank,
					int pin_num, struct regmap **regmap,
					int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	if (bank->bank_num == 0) {
		*bit = 0;
		if (pin_num == 4) {
			*reg = RK3538_DRV_GPIO0_5VIO_0_OFFSET;
			*bit = 13;
		} else if (pin_num == 6) {
			*reg = RK3538_DRV_GPIO0_5VIO_1_OFFSET;
			*bit = 1;
		} else if (pin_num == 7) {
			*reg = RK3538_DRV_GPIO0_5VIO_1_OFFSET;
			*bit = 6;
		} else if (pin_num == 8) {
			*reg = RK3538_DRV_GPIO0_5VIO_1_OFFSET;
			*bit = 9;
		} else if (pin_num == 9) {
			*reg = RK3538_DRV_GPIO0_5VIO_1_OFFSET;
			*bit = 13;
		}
		if (*bit)
			return;
	}
	if (bank->bank_num == 0 && pin_num < 24) {
		*reg = RK3538_DRV_GPIO0_A_OFFSET;
	} else if (bank->bank_num == 0) {
		*reg = RK3538_DRV_GPIO0_D_OFFSET;
	} else if (bank->bank_num == 1) {
		*reg = RK3538_DRV_GPIO1_OFFSET;
	} else if (bank->bank_num == 2) {
		*reg = RK3538_DRV_GPIO2_OFFSET;
	} else if (bank->bank_num == 3) {
		*reg = RK3538_DRV_GPIO3_OFFSET;
	} else if (bank->bank_num == 4) {
		*reg = RK3538_DRV_GPIO4_OFFSET;
	} else if (bank->bank_num == 5) {
		*reg = RK3538_DRV_GPIO5_OFFSET;
	} else if (bank->bank_num == 6) {
		*reg = RK3538_DRV_GPIO6_OFFSET;
	} else {
		*reg = 0;
		debug("unsupported bank_num %d\n", bank->bank_num);
	}

	*reg += ((pin_num / RK3538_DRV_PINS_PER_REG) * 4);
	*bit = pin_num % RK3538_DRV_PINS_PER_REG;
	*bit *= RK3538_DRV_BITS_PER_PIN;
}

static int rk3538_set_drive(struct rockchip_pin_bank *bank,
			    int pin_num, int strength)
{
	struct regmap *regmap;
	int reg;
	u32 data, rmask;
	u8 bit;
	u32 rmask_bits = RK3538_DRV_BITS_PER_PIN;

	rk3538_calc_drv_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);

	if (bank->bank_num == 0) {
		/* gpio0, pin 4/6/8/9 only support drive strength level1.5/3 */
		if (pin_num == 4 || pin_num == 6 || pin_num == 8 || pin_num == 9) {
			rmask_bits = 1;
			if (strength== 0x6)
				strength= 0x0;
			else if (strength== 0x1c)
				strength= 0x1;
			else
				return -EINVAL;
		}
		/* gpio0a7 only support drive strength level1/1.5/2/3 */
		if (pin_num == 7) {
			rmask_bits = 2;
			if (strength== 0x4)
				strength= 0x0;
			else if (strength== 0x6)
				strength= 0x1;
			else if (strength== 0xc)
				strength= 0x2;
			else if (strength== 0x1c)
				strength= 0x3;
			else
				return -EINVAL;
		}
	}
	/* enable the write to the equivalent lower bits */
	data = ((1 << rmask_bits) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (strength << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK3538_PULL_BITS_PER_PIN		2
#define RK3538_PULL_PINS_PER_REG		8
#define RK3538_PULL_GPIO0_A_OFFSET		0x300
#define RK3538_PULL_GPIO0_D_OFFSET		0x10300
#define RK3538_PULL_GPIO0_5VIO_1_OFFSET		0x904
#define RK3538_PULL_GPIO1_OFFSET		0x20310
#define RK3538_PULL_GPIO2_OFFSET		0x30320
#define RK3538_PULL_GPIO3_OFFSET		0x40330
#define RK3538_PULL_GPIO4_OFFSET		0x50340
#define RK3538_PULL_GPIO5_OFFSET		0x60350
#define RK3538_PULL_GPIO6_OFFSET		0x70360

static void rk3538_calc_pull_reg_and_bit(struct rockchip_pin_bank *bank,
					 int pin_num, struct regmap **regmap,
					 int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	if (bank->bank_num == 0 && pin_num == 7) {
		*reg = RK3538_PULL_GPIO0_5VIO_1_OFFSET;
		*bit = 4;

		return;
	} else if (bank->bank_num == 0 && pin_num < 24) {
		*reg = RK3538_PULL_GPIO0_A_OFFSET;
	} else if (bank->bank_num == 0) {
		*reg = RK3538_PULL_GPIO0_D_OFFSET;
	} else if (bank->bank_num == 1) {
		*reg = RK3538_PULL_GPIO1_OFFSET;
	} else if (bank->bank_num == 2) {
		*reg = RK3538_PULL_GPIO2_OFFSET;
	} else if (bank->bank_num == 3) {
		*reg = RK3538_PULL_GPIO3_OFFSET;
	} else if (bank->bank_num == 4) {
		*reg = RK3538_PULL_GPIO4_OFFSET;
	} else if (bank->bank_num == 5) {
		*reg = RK3538_PULL_GPIO5_OFFSET;
	} else if (bank->bank_num == 6) {
		*reg = RK3538_PULL_GPIO6_OFFSET;
	} else {
		*reg = 0;
		debug("unsupported bank_num %d\n", bank->bank_num);
	}

	*reg += ((pin_num / RK3538_PULL_PINS_PER_REG) * 4);
	*bit = pin_num % RK3538_PULL_PINS_PER_REG;
	*bit *= RK3538_PULL_BITS_PER_PIN;
}

static int rk3538_set_pull(struct rockchip_pin_bank *bank,
			   int pin_num, int pull)
{
	struct regmap *regmap;
	int reg, ret;
	u8 bit, type;
	u32 data, rmask;
	u32 rmask_bits = RK3538_PULL_BITS_PER_PIN;

	if (pull == PIN_CONFIG_BIAS_PULL_PIN_DEFAULT)
		return -ENOTSUPP;

	rk3538_calc_pull_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);
	type = bank->pull_type[pin_num / 8];
	ret = rockchip_translate_pull_value(type, pull);
	if (ret < 0) {
		debug("unsupported pull setting %d\n", pull);
		return ret;
	}

	if (bank->bank_num == 0 && pin_num == 7) {
		/* gpio0a7 unsupports pull down */
		if (ret == 2)
			return -EINVAL;
		rmask_bits = 1;
	}
	/* enable the write to the equivalent lower bits */
	data = ((1 << rmask_bits) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (ret << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

#define RK3538_SMT_BITS_PER_PIN		1
#define RK3538_SMT_PINS_PER_REG		8
#define RK3538_SMT_GPIO0_A_OFFSET	0x500
#define RK3538_SMT_GPIO0_D_OFFSET	0x10500
#define RK3538_SMT_GPIO1_OFFSET		0x20510
#define RK3538_SMT_GPIO2_OFFSET		0x30520
#define RK3538_SMT_GPIO3_OFFSET		0x40530
#define RK3538_SMT_GPIO4_OFFSET		0x50540
#define RK3538_SMT_GPIO5_OFFSET		0x60550
#define RK3538_SMT_GPIO6_OFFSET		0x70560

static void rk3538_calc_schmitt_reg_and_bit(struct rockchip_pin_bank *bank,
					    int pin_num,
					    struct regmap **regmap,
					    int *reg, u8 *bit)
{
	struct rockchip_pinctrl_priv *priv = bank->priv;

	*regmap = priv->regmap_base;
	if (bank->bank_num == 0 && pin_num < 24) {
		*reg = RK3538_SMT_GPIO0_A_OFFSET;
	} else if (bank->bank_num == 0) {
		*reg = RK3538_SMT_GPIO0_D_OFFSET;
	} else if (bank->bank_num == 1) {
		*reg = RK3538_SMT_GPIO1_OFFSET;
	} else if (bank->bank_num == 2) {
		*reg = RK3538_SMT_GPIO2_OFFSET;
	} else if (bank->bank_num == 3) {
		*reg = RK3538_SMT_GPIO3_OFFSET;
	} else if (bank->bank_num == 4) {
		*reg = RK3538_SMT_GPIO4_OFFSET;
	} else if (bank->bank_num == 5) {
		*reg = RK3538_SMT_GPIO5_OFFSET;
	} else if (bank->bank_num == 6) {
		*reg = RK3538_SMT_GPIO6_OFFSET;
	} else {
		*reg = 0;
		debug("unsupported bank_num %d\n", bank->bank_num);
	}

	*reg += ((pin_num / RK3538_SMT_PINS_PER_REG) * 4);
	*bit = pin_num % RK3538_SMT_PINS_PER_REG;
	*bit *= RK3538_SMT_BITS_PER_PIN;
}

static int rk3538_set_schmitt(struct rockchip_pin_bank *bank,
			      int pin_num, int enable)
{
	struct regmap *regmap;
	int reg;
	u32 data, rmask;
	u8 bit;

	rk3538_calc_schmitt_reg_and_bit(bank, pin_num, &regmap, &reg, &bit);

	/* enable the write to the equivalent lower bits */
	data = ((1 << RK3538_SMT_BITS_PER_PIN) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (enable << bit);

	return regmap_update_bits(regmap, reg, rmask, data);
}

static struct rockchip_pin_bank rk3538_pin_banks[] = {
	PIN_BANK_IOMUX_4_OFFSET(0, 32, "gpio0", 0, 0x8, 0x10, 0x10018),
	PIN_BANK_IOMUX_4_OFFSET(1, 32, "gpio1", 0x20020, 0x20028, 0x20030, 0x20038),
	PIN_BANK_IOMUX_4_OFFSET(2, 32, "gpio2", 0x30040, 0x30048, 0x30050, 0x30058),
	PIN_BANK_IOMUX_4_OFFSET(3, 32, "gpio3", 0x40060, 0x40068, 0x40070, 0x40078),
	PIN_BANK_IOMUX_4_OFFSET(4, 32, "gpio4", 0x50080, 0x50088, 0x50090, 0x50098),
	PIN_BANK_IOMUX_4_OFFSET(5, 32, "gpio5", 0x600a0, 0x600a8, 0x600b0, 0x600b8),
	PIN_BANK_IOMUX_4_OFFSET(6, 32, "gpio6", 0x700c0, 0x700c8, 0x700d0, 0x700d8),
};

static const struct rockchip_pin_ctrl rk3538_pin_ctrl = {
	.pin_banks		= rk3538_pin_banks,
	.nr_banks		= ARRAY_SIZE(rk3538_pin_banks),
	.nr_pins		= 224,
	.grf_mux_offset		= 0x0,
	.set_mux		= rk3538_set_mux,
	.set_pull		= rk3538_set_pull,
	.set_drive		= rk3538_set_drive,
	.set_schmitt		= rk3538_set_schmitt,
};

static const struct udevice_id rk3538_pinctrl_ids[] = {
	{
		.compatible = "rockchip,rk3538-pinctrl",
		.data = (ulong)&rk3538_pin_ctrl
	},
	{ }
};

U_BOOT_DRIVER(pinctrl_rk3538) = {
	.name		= "rockchip_rk3538_pinctrl",
	.id		= UCLASS_PINCTRL,
	.of_match	= rk3538_pinctrl_ids,
	.priv_auto	= sizeof(struct rockchip_pinctrl_priv),
	.ops		= &rockchip_pinctrl_ops,
#if CONFIG_IS_ENABLED(OF_REAL)
	.bind		= dm_scan_fdt_dev,
#endif
	.probe		= rockchip_pinctrl_probe,
};
