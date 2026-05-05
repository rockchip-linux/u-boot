/*
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * (C) Copyright 2021 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dwc3-uboot.h>
#include <usb.h>
#include <linux/usb/phy-rockchip-usbdp.h>
#include <asm/io.h>
#include <rockusb.h>
#ifdef CONFIG_DISPLAY_CPUINFO
#include <clk.h>
#include <dm.h>
#include <dt-bindings/clock/rk3588-cru.h>
#include <asm/arch-rockchip/param.h>
#ifdef CONFIG_ROCKCHIP_PRELOADER_ATAGS
#include <asm/arch-rockchip/rk_atags.h>
#endif
#endif

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	struct udevice *dev;
	struct clk clk;
	ulong cpul_hz = 0, cpub01_hz = 0, cpub23_hz = 0;
	ulong gpu_hz = 0, npu_hz = 0, ddr_hz = 0;
	u64 ddr_size = 0;
	int i;

	/* query all frequencies via SCMI clock driver */
	if (!uclass_get_device_by_driver(UCLASS_CLK,
					 DM_GET_DRIVER(scmi_clock), &dev)) {
		clk.dev = dev;

		clk.id = SCMI_CLK_CPUL;
		cpul_hz = clk_get_rate(&clk);

		clk.id = SCMI_CLK_CPUB01;
		cpub01_hz = clk_get_rate(&clk);

		clk.id = SCMI_CLK_CPUB23;
		cpub23_hz = clk_get_rate(&clk);

		clk.id = SCMI_CLK_GPU;
		gpu_hz = clk_get_rate(&clk);

		clk.id = SCMI_CLK_NPU;
		npu_hz = clk_get_rate(&clk);

		clk.id = SCMI_CLK_DDR;
		ddr_hz = clk_get_rate(&clk);
	}

	/*
	 * Read raw DDR bank sizes from ATAGs to get the true physical capacity
	 * (avoids the SDRAM_MAX_SIZE cap in param_parse_ddr_mem which only
	 * returns 3840 MiB for a 8 GiB board).
	 */
#ifdef CONFIG_ROCKCHIP_PRELOADER_ATAGS
	{
		struct tag *t = atags_get_tag(ATAG_DDR_MEM);

		if (t && t->u.ddr_mem.count) {
			u32 dc = t->u.ddr_mem.count;

			for (i = 0; i < (int)dc; i++)
				ddr_size += t->u.ddr_mem.bank[i + dc];
		}
	}
#endif

	printf("CPU:   Rockchip RK3588  (4x Cortex-A76 + 4x Cortex-A55)\n");
	if (cpul_hz)
		printf("       Cortex-A55 (Little):  %lu MHz\n",
		       cpul_hz / 1000000);
	if (cpub01_hz)
		printf("       Cortex-A76 (Big 0-1): %lu MHz\n",
		       cpub01_hz / 1000000);
	if (cpub23_hz)
		printf("       Cortex-A76 (Big 2-3): %lu MHz\n",
		       cpub23_hz / 1000000);

	printf("GPU:   ARM Mali-G610 MC4");
	if (gpu_hz)
		printf("  @ %lu MHz", gpu_hz / 1000000);
	printf("\n");

	printf("NPU:   6 TOPS  (3-core, int4/int8/int16/FP16/BF16/TF32)");
	if (npu_hz)
		printf("  @ %lu MHz", npu_hz / 1000000);
	printf("\n");

	if (ddr_size && !(ddr_size & ((1ULL << 30) - 1)))
		printf("DDR:   %llu GiB", ddr_size >> 30);
	else if (ddr_size)
		printf("DDR:   %llu MiB", ddr_size >> 20);
	else
		printf("DDR:   unknown");
	if (ddr_hz)
		printf("  @ %lu MHz", ddr_hz / 1000000);
	printf("\n");

	return 0;
}
#endif

#ifdef CONFIG_USB_DWC3
#define CRU_BASE		0xfd7c0000
#define CRU_SOFTRST_CON42	0x0aa8
#define U3PHY_BASE		0xfed80000

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = 0xfc000000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
	.dis_u1u2_quirk = 1,
	.usb2_phyif_utmi_width = 16,
};

int usb_gadget_handle_interrupts(int index)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

bool rkusb_usb3_capable(void)
{
	return true;
}

static void usb_reset_otg_controller(void)
{
	writel(0x00100010, CRU_BASE + CRU_SOFTRST_CON42);
	mdelay(1);
	writel(0x00100000, CRU_BASE + CRU_SOFTRST_CON42);
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

#if defined(CONFIG_SUPPORT_USBPLUG)
int board_usb_cleanup(int index, enum usb_init_type init)
{
	dwc3_uboot_exit(index);
	return 0;
}
#endif

#endif
