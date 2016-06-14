/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <spl.h>

#ifdef CONFIG_USB_GADGET
#include <usb.h>
#include <usb/dwc2_udc.h>
#endif

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_MMC1;
	spl_boot_list[1] = BOOT_DEVICE_MMC2;
}

#ifdef CONFIG_USB_GADGET
#define RKIO_GRF_PHYS				0xff770000
#define RKIO_USBOTG_BASE			0xff580000
#define RK_USB_PHY_CONTROL			0xff580e00

static struct dwc2_plat_otg_data rk_otg_data = {
	.regs_phy	= RKIO_GRF_PHYS,
	.regs_otg	= RKIO_USBOTG_BASE,
	.usb_phy_ctrl	= RK_USB_PHY_CONTROL,
	.usb_gusbcfg	= 0x00001408
};

int board_usb_init(int index, enum usb_init_type init)
{
	debug("%s: performing dwc2_udc_probe\n", __func__);
	return dwc2_udc_probe(&rk_otg_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	debug("%s\n", __func__);
	return 0;
}
#endif
