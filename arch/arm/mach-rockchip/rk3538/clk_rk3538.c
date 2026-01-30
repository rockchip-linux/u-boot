// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd.
 */

#include <dm.h>
#include <asm/arch-rockchip/cru_rk3538.h>

int rockchip_get_clk(struct udevice **devp)
{
	return uclass_get_device_by_driver(UCLASS_CLK,
				DM_DRIVER_GET(rockchip_rk3538_cru), devp);
}

#if CONFIG_IS_ENABLED(CLK_SCMI)
int rockchip_get_scmi_clk(struct udevice **devp)
{
	return uclass_get_device_by_driver(UCLASS_CLK,
			DM_DRIVER_GET(scmi_clock), devp);
}
#endif

void *rockchip_get_cru(void)
{
	return (void *)RK3538_CRU_BASE;
}
