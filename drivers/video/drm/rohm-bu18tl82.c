// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2022 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <video_bridge.h>
#include <asm/unaligned.h>
#include <linux/media-bus-format.h>
#include <power/regulator.h>

#include "rockchip_bridge.h"
#include "rockchip_display.h"
#include "rockchip_panel.h"

struct ser_reg_sequence {
	uint reg;
	uint def;
};

struct serdes_init_seq {
	struct ser_reg_sequence *reg_sequence;
	uint reg_seq_cnt;
};

struct bu18tl82_priv {
	struct udevice *dev;
	struct udevice *power_supply;
	struct gpio_desc enable_gpio;
	struct serdes_init_seq *serdes_init_seq;
};

static void bu18tl82_serdes_init_sequence_write(struct bu18tl82_priv *priv)
{
	struct serdes_init_seq *serdes_init_seq = priv->serdes_init_seq;
	struct ser_reg_sequence *reg_sequence =  serdes_init_seq->reg_sequence;
	uint cnt = serdes_init_seq->reg_seq_cnt;
	struct udevice *dev = priv->dev;
	uint i;

	for (i = 0; i < cnt; i++)
		dm_i2c_reg_write(dev, reg_sequence[i].reg, reg_sequence[i].def);

	mdelay(1000);
}

static void bu18tl82_bridge_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct bu18tl82_priv *priv = dev_get_priv(dev);

	bu18tl82_serdes_init_sequence_write(priv);
}

static void bu18tl82_bridge_disable(struct rockchip_bridge *bridge)
{
}

static void bu18tl82_bridge_pre_enable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct bu18tl82_priv *priv = dev_get_priv(dev);

	if (priv->power_supply)
		regulator_set_enable(priv->power_supply, true);

	if (dm_gpio_is_valid(&priv->enable_gpio))
		dm_gpio_set_value(&priv->enable_gpio, 1);

	mdelay(120);

	video_bridge_set_active(priv->dev, true);
}

static void bu18tl82_bridge_post_disable(struct rockchip_bridge *bridge)
{
	struct udevice *dev = bridge->dev;
	struct bu18tl82_priv *priv = dev_get_priv(dev);

	video_bridge_set_active(priv->dev, false);

	if (dm_gpio_is_valid(&priv->enable_gpio))
		dm_gpio_set_value(&priv->enable_gpio, 0);

	if (priv->power_supply)
		regulator_set_enable(priv->power_supply, false);
}

static const struct rockchip_bridge_funcs bu18tl82_bridge_funcs = {
	.enable = bu18tl82_bridge_enable,
	.disable = bu18tl82_bridge_disable,
	.pre_enable = bu18tl82_bridge_pre_enable,
	.post_disable = bu18tl82_bridge_post_disable,
};

static int bu18tl82_parse_init_seq(struct udevice *dev, const u16 *data,
				   int length, struct serdes_init_seq *seq)
{
	struct ser_reg_sequence *reg_sequence;
	u16 *buf, *d;
	unsigned int i, cnt;
	int ret;

	if (!seq)
		return -EINVAL;

	buf = calloc(1, length);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, data, length);

	d = buf;
	cnt = length / 4;
	seq->reg_seq_cnt = cnt;

	seq->reg_sequence = calloc(cnt, sizeof(struct ser_reg_sequence));
	if (!seq->reg_sequence) {
		ret = -ENOMEM;
		goto free_buf;
	}

	for (i = 0; i < cnt; i++) {
		reg_sequence = &seq->reg_sequence[i];
		reg_sequence->reg = get_unaligned_be16(&d[0]);
		reg_sequence->def = get_unaligned_be16(&d[1]);
		d += 2;
	}

	return 0;

free_buf:
	free(buf);

	return ret;
}

static int bu18tl82_get_init_seq(struct bu18tl82_priv *priv)
{
	const void *data = NULL;
	int len, err;

	data = dev_read_prop(priv->dev, "serdes-init-sequence", &len);
	if (!data) {
		printf("failed to get serdes-init-sequence\n");
		return -EINVAL;
	}

	priv->serdes_init_seq = calloc(1, sizeof(*priv->serdes_init_seq));
	if (!priv->serdes_init_seq)
		return -ENOMEM;

	err = bu18tl82_parse_init_seq(priv->dev, data, len, priv->serdes_init_seq);
	if (err) {
		printf("failed to parse serdes-init-sequence\n");
		goto free_init_seq;
	}

	return 0;

free_init_seq:
	free(priv->serdes_init_seq);

	return err;
}

static int bu18tl82_probe(struct udevice *dev)
{
	struct bu18tl82_priv *priv = dev_get_priv(dev);
	struct rockchip_bridge *bridge;
	int ret;

	ret = i2c_set_chip_offset_len(dev, 2);
	if (ret)
		return ret;

	priv->dev = dev;

	ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
					   "power-supply", &priv->power_supply);
	if (ret && ret != -ENOENT) {
		printf("%s: Cannot get power supply: %d\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &priv->enable_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "%s: failed to get enable GPIO: %d\n", __func__, ret);
		return ret;
	}

	bridge = calloc(1, sizeof(*bridge));
	if (!bridge)
		return -ENOMEM;

	ret = bu18tl82_get_init_seq(priv);
	if (ret)
		goto free_bridge;

	dev->driver_data = (ulong)bridge;
	bridge->dev = dev;
	bridge->funcs = &bu18tl82_bridge_funcs;

	return 0;

free_bridge:
	free(bridge);

	return ret;
}

static const struct udevice_id bu18tl82_of_match[] = {
	{ .compatible = "rohm,bu18tl82", },
	{}
};

U_BOOT_DRIVER(bu18tl82) = {
	.name = "bu18tl82",
	.id = UCLASS_VIDEO_BRIDGE,
	.of_match = bu18tl82_of_match,
	.probe = bu18tl82_probe,
	.priv_auto_alloc_size = sizeof(struct bu18tl82_priv),
};
