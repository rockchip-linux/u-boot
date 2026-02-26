// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd
 * Author: Elaine Zhang <zhangqing@rock-chips.com>
 */

#include <common.h>
#include <bitfield.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <syscon.h>
#include <asm/arch/clock.h>
#include <asm/arch/cru_rk3576.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <dm/lists.h>

DECLARE_GLOBAL_DATA_PTR;

struct rockchip_dclk_mos {
	int (*set_parent)(struct clk *clk, struct clk *parent);
};

struct rockchip_dclk_mos_priv {
	void *base;
	const char *name;
	const struct rockchip_dclk_mos *data;
};

static int rk3576_dclk_mos_set_parent(struct clk *clk, struct clk *parent)
{
	struct rockchip_dclk_mos_priv *priv = dev_get_priv(clk->dev);
	const char *clock_dev_name = parent->dev->name;
	const char *clock_name = priv->name;
	u32 sel;

	if (!strcmp(clock_name, "dclk_vp0_mos")) {
		if (!strcmp(clock_dev_name, "hdmiphypll_clk0"))
			sel = 1;
		else
			sel = 0;
		rk_clrsetreg(priv->base, DCLK0_VOP_SEL_MASK,
			     sel << DCLK0_VOP_SEL_SHIFT);
	} else if (!strcmp(clock_name, "dclk_vp1_mos")) {
		if (!strcmp(clock_dev_name, "hdmiphypll_clk0"))
			sel = 1;
		else
			sel = 0;
		rk_clrsetreg(priv->base, DCLK1_VOP_SEL_MASK,
			     sel << DCLK1_VOP_SEL_SHIFT);

	} else {
		if (!strcmp(clock_dev_name, "hdmiphypll_clk0"))
			sel = 1;
		else
			sel = 0;
		rk_clrsetreg(priv->base, DCLK2_VOP_SEL_MASK,
			     sel << DCLK2_VOP_SEL_SHIFT);
	}

	return 0;
}

static int rockchip_clk_dclk_mos_set_parent(struct clk *clk, struct clk *parent)
{
	struct rockchip_dclk_mos_priv *priv = dev_get_priv(clk->dev);

	return priv->data->set_parent(clk, parent);
}

static struct clk_ops rockchip_clk_dclk_mos_ops = {
	.set_parent = rockchip_clk_dclk_mos_set_parent,
};

static int rockchip_clk_dclk_mos_probe(struct udevice *dev)
{
	struct rockchip_dclk_mos_priv *priv = dev_get_priv(dev);
	struct rockchip_dclk_mos *dclk_mos;
	const char *output_name;

	dclk_mos = (struct rockchip_dclk_mos *)dev_get_driver_data(dev);

	output_name = dev_read_string(dev, "clock-output-names");
	priv->name = output_name ? output_name : dev->name;

	priv->data = dclk_mos;

	return 0;
}

static int rockchip_clk_dclk_mos_ofdata_to_platdata(struct udevice *dev)
{
	struct rockchip_dclk_mos_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_ptr(dev);

	return 0;
}

static const struct rockchip_dclk_mos rk3576_dclk_mos_data = {
	.set_parent = rk3576_dclk_mos_set_parent,
};

static const struct udevice_id rockchip_clk_dclk_mos_ids[] = {
	{
		.compatible = "rockchip,rk3576-clock-dclk-mos",
		.data = (ulong)&rk3576_dclk_mos_data,
	},
	{ }
};

U_BOOT_DRIVER(rockchip_rk3576_cru_mos) = {
	.name		= "rockchip_clk_dclk_mos",
	.id		= UCLASS_CLK,
	.of_match	= rockchip_clk_dclk_mos_ids,
	.priv_auto_alloc_size = sizeof(struct rockchip_dclk_mos_priv),
	.ofdata_to_platdata = rockchip_clk_dclk_mos_ofdata_to_platdata,
	.ops		= &rockchip_clk_dclk_mos_ops,
	.probe		= rockchip_clk_dclk_mos_probe,
};
