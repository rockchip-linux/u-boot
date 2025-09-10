// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd.
 */

#include <dm.h>
#include <ram.h>
#include <asm/arch-rockchip/sdram.h>

#define PMU1GRF_BASE			0xfd180000
#define OS_REG0_REG			0x1000

static int rk3538_dmc_get_info(struct udevice *dev, struct ram_info *info)
{
	info->base = CFG_SYS_SDRAM_BASE;
	info->size = rockchip_sdram_size(PMU1GRF_BASE + OS_REG0_REG);

	return 0;
}

static struct ram_ops rk3538_dmc_ops = {
	.get_info = rk3538_dmc_get_info,
};

static const struct udevice_id rk3538_dmc_ids[] = {
	{ .compatible = "rockchip,rk3538-dmc" },
	{ }
};

U_BOOT_DRIVER(rockchip_rk3538_dmc) = {
	.name = "rockchip_rk3538_dmc",
	.id = UCLASS_RAM,
	.of_match = rk3538_dmc_ids,
	.ops = &rk3538_dmc_ops,
};
