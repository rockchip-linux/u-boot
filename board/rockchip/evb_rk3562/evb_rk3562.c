// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dwc3-uboot.h>
#include <usb.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = 0xfe500000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.hsphy_mode = USBPHY_INTERFACE_MODE_UTMIW,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
};

int board_usb_init(int index, enum usb_init_type init)
{
	return dwc3_uboot_init(&dwc3_device_data);
}
#endif
