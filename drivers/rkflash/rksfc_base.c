/*
 * Copyright (c) 2018 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <asm/arch/clock.h>
#include <rksfc.h>
#include <asm/arch/vendor.h>
#include <asm/gpio.h>

#include "rkflash_blk.h"
#include "rkflash_api.h"

#define SFC_CS_GPIO_MAX	4

struct rksfc_info {
	void *reg_base;
#if defined(CONFIG_DM_GPIO) && (defined(CONFIG_SPL_GPIO_SUPPORT) || !defined(CONFIG_SPL_BUILD))
	struct gpio_desc cs_gpiods[SFC_CS_GPIO_MAX];
#endif
	int num_cs_gpios;
	struct clk clk;
	struct clk ahb_clk;
	unsigned long clk_rate;
	bool sclk_x2_bypass;
};

static struct rksfc_info g_sfc_info;

static int rksfc_clk_set_rate(struct rksfc_info *sfc, unsigned long speed)
{
	if (sfc_get_version() < SFC_VER_8 || sfc->sclk_x2_bypass)
		return clk_set_rate(&sfc->clk, speed);
	else
		return clk_set_rate(&sfc->clk, speed * 2);
}

static unsigned long rksfc_clk_get_rate(struct rksfc_info *sfc)
{
	if (sfc_get_version() < SFC_VER_8 || sfc->sclk_x2_bypass)
		return clk_get_rate(&sfc->clk);
	else
		return clk_get_rate(&sfc->clk) / 2;
}

void rksfc_set_cs_gpio(u8 cs, bool enable)
{
#if defined(CONFIG_DM_GPIO) && (defined(CONFIG_SPL_GPIO_SUPPORT) || !defined(CONFIG_SPL_BUILD))
	if (cs < SFC_CS_GPIO_MAX)
		if (dm_gpio_is_valid(&g_sfc_info.cs_gpiods[cs]))
			dm_gpio_set_value(&g_sfc_info.cs_gpiods[cs], enable);
#endif
}

static int rksfc_get_gpio_descs(struct udevice *dev)
{
#if defined(CONFIG_DM_GPIO) && (defined(CONFIG_SPL_GPIO_SUPPORT) || !defined(CONFIG_SPL_BUILD))
	int ret;
	int i;

	ret = gpio_request_list_by_name(dev, "sfc-cs-gpios", g_sfc_info.cs_gpiods,
					ARRAY_SIZE(g_sfc_info.cs_gpiods), 0);
	if (ret < 0) {
		pr_err("Can't get %s gpios! Error: %d\n", dev->name, ret);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(g_sfc_info.cs_gpiods); i++) {
		if (!dm_gpio_is_valid(&g_sfc_info.cs_gpiods[i]))
			continue;

		ret = dm_gpio_set_dir_flags(&g_sfc_info.cs_gpiods[i],
					    GPIOD_IS_OUT | GPIOD_ACTIVE_LOW);
		if (ret) {
			dev_err(dev, "Setting cs %d error, ret=%d\n", i, ret);
			return ret;
		}
		dm_gpio_set_value(&g_sfc_info.cs_gpiods[i], 0);
	}
#endif

	return 0;
}

static struct flash_operation sfc_nor_op = {
#ifdef	CONFIG_RKSFC_NOR
	IF_TYPE_SPINOR,
	rksfc_nor_init,
	rksfc_nor_get_capacity,
	rksfc_nor_read,
	rksfc_nor_write,
	NULL,
	rksfc_nor_vendor_read,
	rksfc_nor_vendor_write,
#else
	-1, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
#endif
};

static struct flash_operation sfc_nand_op = {
#ifdef CONFIG_RKSFC_NAND
	IF_TYPE_SPINAND,
	rksfc_nand_init,
	rksfc_nand_get_density,
	rksfc_nand_read,
	rksfc_nand_write,
	NULL,
	rksfc_nand_vendor_read,
	rksfc_nand_vendor_write,
#else
	-1, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
#endif
};

static struct flash_operation *spi_flash_op[2] = {
	&sfc_nor_op,
	&sfc_nand_op,
};

int rksfc_scan_namespace(void)
{
	struct uclass *uc;
	struct udevice *dev;
	int ret;

	ret = uclass_get(UCLASS_SPI_FLASH, &uc);
	if (ret)
		return ret;

	uclass_foreach_dev(dev, uc) {
		debug("%s %d %p\n", __func__, __LINE__, dev);
		ret = device_probe(dev);
		if (ret)
			return ret;
	}

	return 0;
}

static int rksfc_blk_bind(struct udevice *udev)
{
	struct udevice *bdev;
	int ret;

	ret = blk_create_devicef(udev, "rkflash_blk", "spinand.blk",
				 IF_TYPE_SPINAND,
				 0, 512, 0, &bdev);
	ret = blk_create_devicef(udev, "rkflash_blk", "spinor.blk",
				 IF_TYPE_SPINOR,
				 1, 512, 0, &bdev);

	if (ret) {
		debug("Cannot create block device\n");
		return ret;
	}

	return 0;
}

static int rockchip_rksfc_ofdata_to_platdata(struct udevice *dev)
{
	struct rkflash_info *priv = dev_get_priv(dev);

	priv->ioaddr = dev_read_addr_ptr(dev);

	return 0;
}

static int rockchip_rksfc_probe(struct udevice *udev)
{
	int ret = 0;
	int i;
	struct rkflash_info *priv = dev_get_priv(udev);

	debug("%s %d %p ndev = %p\n", __func__, __LINE__, udev, priv);

	ret = clk_get_by_index(udev, 0, &g_sfc_info.clk);
	if (ret) {
		printf("%s get clk error\n", __func__);
		return ret;
	}

	ret = clk_get_by_index(udev, 1, &g_sfc_info.ahb_clk);
	if (ret) {
		printf("%s get ahb_clk error\n", __func__);
		return ret;
	}

	g_sfc_info.sclk_x2_bypass = dev_read_bool(udev, "rockchip,sclk-x2-bypass");

	g_sfc_info.clk_rate = dev_read_u32_default(udev, "spi-max-frequency", 0);

	ret = rksfc_get_gpio_descs(udev);
	if (ret)
		return ret;

	sfc_init(priv->ioaddr);

	if (!g_sfc_info.clk_rate)
		g_sfc_info.clk_rate = rksfc_clk_get_rate(&g_sfc_info);
	else if (g_sfc_info.clk_rate > RKSFC_CLK_MAX_RATE)
		g_sfc_info.clk_rate = RKSFC_DLL_THRESHOLD_RATE;
	rksfc_clk_set_rate(&g_sfc_info, g_sfc_info.clk_rate);
	g_sfc_info.clk_rate = rksfc_clk_get_rate(&g_sfc_info);
	printf("%s clk rate = %ld\n", __func__, g_sfc_info.clk_rate);

	for (i = 0; i < 2; i++) {
		if (spi_flash_op[i]->id <= 0) {
			debug("%s no optional spi flash for type %x\n",
			      __func__, i);
			continue;
		}
		ret = spi_flash_op[i]->flash_init(udev);
		if (!ret) {
			priv->flash_con_type = spi_flash_op[i]->id;
			priv->density =
				spi_flash_op[i]->flash_get_capacity(udev);
			priv->read = spi_flash_op[i]->flash_read;
			priv->write = spi_flash_op[i]->flash_write;
#ifdef CONFIG_ROCKCHIP_VENDOR_PARTITION
			flash_vendor_dev_ops_register(spi_flash_op[i]->vendor_read,
						      spi_flash_op[i]->vendor_write);
#endif
			debug("%s probe success\n", __func__);
			break;
		} else {
			pr_err("ret %d\n", ret);
		}
	}

	return ret;
}

UCLASS_DRIVER(rksfc) = {
	.id		= UCLASS_SPI_FLASH,
	.name		= "rksfc",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
};

static const struct udevice_id rockchip_sfc_ids[] = {
	{ .compatible = "rockchip,rksfc" },
	{ }
};

U_BOOT_DRIVER(rksfc) = {
	.name		= "rksfc",
	.id		= UCLASS_SPI_FLASH,
	.of_match	= rockchip_sfc_ids,
	.bind		= rksfc_blk_bind,
	.probe		= rockchip_rksfc_probe,
	.priv_auto_alloc_size = sizeof(struct rkflash_info),
	.ofdata_to_platdata = rockchip_rksfc_ofdata_to_platdata,
};

