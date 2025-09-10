// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <asm/arch-rockchip/clock.h>

static const struct udevice_id rk3538_syscon_ids[] = {
	{ .compatible = "rockchip,rk3538-vo-grf", .data = ROCKCHIP_SYSCON_VO_GRF },
	{ .compatible = "rockchip,rk3538-ioc-grf", .data = ROCKCHIP_SYSCON_IOC },
	{ }
};

U_BOOT_DRIVER(rockchip_rk3538_syscon) = {
	.name = "rockchip_rk3538_syscon",
	.id = UCLASS_SYSCON,
	.of_match = rk3538_syscon_ids,
#if CONFIG_IS_ENABLED(OF_REAL)
	.bind = dm_scan_fdt_dev,
#endif
};
