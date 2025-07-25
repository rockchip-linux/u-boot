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

#ifdef CONFIG_USB_DWC3
#define CRU_BASE		0x27200000
#define CRU_SOFTRST_CON47	0x0abc
#define U3PHY_BASE		0x2b010000

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = 0x23000000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.hsphy_mode = USBPHY_INTERFACE_MODE_UTMIW,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
	.dis_u1u2_quirk = 1,
};

int rkusb_dev_bind_to_udc_data(struct udevice *dev)
{
	if (IS_ERR_OR_NULL(dev))
		return -EINVAL;

	dwc3_device_data.dev = dev;

	return 0;
}

bool rkusb_usb3_capable(void)
{
	return true;
}

static void usb_reset_otg_controller(void)
{
	writel(0x00200020, CRU_BASE + CRU_SOFTRST_CON47);
	mdelay(1);
	writel(0x00200000, CRU_BASE + CRU_SOFTRST_CON47);
	mdelay(1);
}

int board_usb_init(int index, enum usb_init_type init)
{
	u32 ret = 0;

	usb_reset_otg_controller();

#if defined(CONFIG_SUPPORT_USBPLUG)
	dwc3_device_data.maximum_speed = USB_SPEED_HIGH;

	if (rkusb_switch_usb3_enabled()) {
		dwc3_device_data.maximum_speed = USB_SPEED_SUPER;
		ret = rockchip_u3phy_uboot_init(U3PHY_BASE);
		if (ret) {
			rkusb_force_to_usb2(true);
			dwc3_device_data.maximum_speed = USB_SPEED_HIGH;
		}
	}
#else
	ret = rockchip_u3phy_uboot_init(U3PHY_BASE);
	if (ret) {
		rkusb_force_to_usb2(true);
		dwc3_device_data.maximum_speed = USB_SPEED_HIGH;
	}
#endif

	return dwc3_uboot_init(&dwc3_device_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	dwc3_uboot_exit(index);
	return 0;
}
#endif
