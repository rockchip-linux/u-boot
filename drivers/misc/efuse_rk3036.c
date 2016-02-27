/*
 * (C) Copyright 2016 rk3036 Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <misc.h>
#include <linux/time.h>
#include <asm/arch-rockchip/grf_rk3036.h>
#include <asm/io.h>

#define EFUSE_A_SHIFT			8
#define EFUSE_A_MASK			0xff
#define EFUSE_PGMEN			BIT(3)
#define EFUSE_RDEN			BIT(2)
#define EFUSE_AEN			BIT(1)

struct rk3036_efuse_regs {
	u32	con;
	u32	data;
};

struct rk3036_efuse_platdata {
	struct rk3036_efuse_regs *regs;
};

static int rk3036_efuse_read(struct udevice *dev,
			     int offset, void *buf, int size)
{
	struct rk3036_efuse_platdata *plat = dev->platdata;
	struct rk3036_efuse_regs *const regs = plat->regs;
	char *data = buf;
	int i;

	writel(EFUSE_RDEN, &regs->con);
	udelay(1);
	for (i = 0; i < size; i++) {
		writel(readl(&regs->con) &
				(~(EFUSE_A_MASK << EFUSE_A_SHIFT)),
				&regs->con);
		writel(readl(&regs->con) |
				(((offset + i) & EFUSE_A_MASK) << EFUSE_A_SHIFT),
				&regs->con);
		udelay(1);
		writel(readl(&regs->con) |
				EFUSE_AEN, &regs->con);
		udelay(1);
		data[i] = readb(&regs->data);
		writel(readl(&regs->con) &
				(~EFUSE_AEN), &regs->con);
		udelay(1);
	}

	/* Switch to inactive mode */
	writel(0, &regs->con);

	return 0;
}

static int rk3036_efuse_ofdata_to_platdata(struct udevice *dev)
{
	struct rk3036_efuse_platdata *plat = dev_get_platdata(dev);

	plat->regs = map_physmem(dev_get_addr(dev),
			sizeof(struct rk3036_efuse_regs),
			MAP_NOCACHE);

	return 0;
}

static const struct misc_ops rk3036_efuse_ops = {
	.read = rk3036_efuse_read,
};

static const struct udevice_id rk3036_efuse_ids[] = {
	{ .compatible = "rockchip,rk3036-efuse" },
	{}
};

U_BOOT_DRIVER(rk3036_efuse) = {
	.name	= "rk3036_efuse",
	.id	= UCLASS_MISC,
	.of_match = rk3036_efuse_ids,
	.ofdata_to_platdata = rk3036_efuse_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct rk3036_efuse_platdata),
	.ops	= &rk3036_efuse_ops,
};
