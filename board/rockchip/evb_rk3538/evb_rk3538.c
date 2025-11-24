// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 */

 #include <common.h>
 #include <dwc3-uboot.h>
 #include <usb.h>
 #include <linux/delay.h>
 #include <linux/usb/phy.h>
 #include <linux/usb/phy-rockchip-usbdp.h>
 #include <asm/io.h>
 #include <rockusb.h>

DECLARE_GLOBAL_DATA_PTR;
#define VO_CRU_BASE		0xFD060000
#define VO_CRU_SOFTRST_CON01	0x0A04

#ifdef CONFIG_USB_DWC3
static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = 0xfc000000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.hsphy_mode = USBPHY_INTERFACE_MODE_UTMIW,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
};

int rkusb_dev_bind_to_udc_data(struct udevice *dev)
{
	if (IS_ERR_OR_NULL(dev))
		return -EINVAL;

	dwc3_device_data.dev = dev;

	return 0;
}

static void usb_reset_otg_controller(void)
{
	writel(0x08000800, VO_CRU_BASE + VO_CRU_SOFTRST_CON01);
	mdelay(1);
	writel(0x08000000, VO_CRU_BASE + VO_CRU_SOFTRST_CON01);
	mdelay(1);
}

int board_usb_init(int index, enum usb_init_type init)
{
	usb_reset_otg_controller();
	return dwc3_uboot_init(&dwc3_device_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	dwc3_uboot_exit(index);
	return 0;
}
#endif
