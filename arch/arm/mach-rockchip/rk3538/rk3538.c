// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <fdt_support.h>
#include <misc.h>
#include <string.h>
#include <scsi.h>
#include <spl.h>
#include <tee/optee.h>
#include <linux/delay.h>
#include <asm/armv8/mmu.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/smccc.h>
#include <asm/arch-rockchip/vendor.h>

DECLARE_GLOBAL_DATA_PTR;

#define CRU_PHPL_BASE			0xfd080000
#define CRU_PHPL_GATE_CON01		0x804

#define VO_GRF_BASE			0xfd170000
#define SAI2ACODEC_CON2			0x8
#define USBPHY_HOST_CON0		0x1C

#define USBPHY_APB_BASE			0xfdba0000
#define USBPHY_DIFF_RECEIVER_0		0x0030
#define USBPHY_DIFF_RECEIVER_1		0x0430

#define PHPL_GRF_BASE			0xfd190000
#define TSADC_CON0			0x1c
#define TSADC_CON1			0x20
#define TSADC_CON4			0x2c
#define TSADC_CON6			0x34
#define TSADC_ST0			0x110
#define TSADC_ST1			0x114
#define TSADC_DEF_WIDTH			0x00010001
#define TSADC_TARGET_WIDTH		24000
#define TSADC_EOC_TIMEOUT		20000
#define TSADC_EOC_MASK			BIT(18)
#define TSADC_DEF_BIAS			0x7f
#define TSADC_MIN_BIAS			0x1
#define TSADC_MAX_BIAS			0x7f
#define TSADC_UNLOCK_VALUE		0xa5
#define TSADC_UNLOCK_VALUE_MASK		(0xff << 16)
#define TSADC_UNLOCK_TRIGGER		BIT(8)
#define TSADC_UNLOCK_TRIGGER_MASK	(BIT(8) << 16)

#define GPIO1_IOC_BASE		0xFD1D0000
#define GPIO1A_IOMUX_SEL_1	0x24
#define GPIO1B_IOMUX_SEL_0	0x28
#define GPIO1B_IOMUX_SEL_1	0x2c
#define GPIO1C_IOMUX_SEL_0	0x30

#define OTP_SPEC_NUM_OFFSET		0x02
#define OTP_SPEC_NUM_MASK		0xffff
#define REMARK_OTP_SPEC_NUM_OFFSET	0x14

#ifdef CONFIG_ARM64
#include <asm/armv8/mmu.h>
static struct mm_region rk3538_mem_map[] = {
	{
		.virt = 0xfafd0000UL,
		.phys = 0xfafd0000UL,
		.size = 0x030f0000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x00000000UL,
		.phys = 0x00000000UL,
		.size = 0xfafd0000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = rk3538_mem_map;
#endif

#if 0
const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/soc/mmc@2a330000",
	[BROM_BOOTSOURCE_SD] = "/soc/mmc@2a310000",
};
#endif

void board_debug_uart_init(void)
{
}

#ifdef CONFIG_SPL_BUILD
void rockchip_stimer_init(void)
{
	u32 reg;

	if (!IS_ENABLED(CONFIG_XPL_BUILD))
		return;

	reg = readl(CONFIG_ROCKCHIP_STIMER_BASE + 0x4);
	if (reg & 0x1)
		return;

#ifdef CONFIG_COUNTER_FREQUENCY
	asm volatile("msr cntfrq_el0, %0" : : "r" (CONFIG_COUNTER_FREQUENCY));
#endif
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x14);
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x18);
	writel(0x00010001, CONFIG_ROCKCHIP_STIMER_BASE + 0x04);
}

static void tsadc_trigger(void)
{
	writel(TSADC_UNLOCK_VALUE | TSADC_UNLOCK_VALUE_MASK,
	       PHPL_GRF_BASE + TSADC_CON1);
	writel(TSADC_UNLOCK_TRIGGER | TSADC_UNLOCK_TRIGGER_MASK,
	       PHPL_GRF_BASE + TSADC_CON1);
	writel(TSADC_UNLOCK_TRIGGER_MASK, PHPL_GRF_BASE + TSADC_CON1);
}

static void tsadc_adjust_bias_current(void)
{
	u32 i, bias, value = 0, width = 0;

	for (i = 0; i < TSADC_EOC_TIMEOUT; i++) {
		value = readl(PHPL_GRF_BASE + TSADC_ST0);
		if ((value & TSADC_EOC_MASK) == TSADC_EOC_MASK)
			break;
		udelay(1);
	}

	if (i == TSADC_EOC_TIMEOUT)
		printf("Failed to adjust tsadc bias\n");

	value = readl(PHPL_GRF_BASE + TSADC_ST1);
	width = (value & 0x0000ffff) + ((value & 0xffff0000) >> 16);
	bias = width * TSADC_DEF_BIAS / TSADC_TARGET_WIDTH;
	if (bias > TSADC_MAX_BIAS)
		bias = TSADC_MAX_BIAS;
	else if (bias < TSADC_MIN_BIAS)
		bias = TSADC_MIN_BIAS;
	printf("tsadc width=0x%x %u lo=%u hi=%u, bias=0x%x\n",
	       value, ((value & 0xffff0000) >> 16) + (value & 0xffff),
	       value & 0xffff, (value & 0xffff0000) >> 16, bias);
	writel((TSADC_MAX_BIAS << 16) | bias, PHPL_GRF_BASE + TSADC_CON6);
	tsadc_trigger();
}

void spl_rk_board_prepare_for_jump(struct spl_image_info *spl_image)
{
	tsadc_adjust_bias_current();
}
#endif

#ifndef CONFIG_TPL_BUILD
int arch_cpu_init(void)
{
#ifdef CONFIG_SPL_BUILD

#ifdef CONFIG_ROCKCHIP_SFC_IOMUX
	writel(0xf0002000, GPIO1_IOC_BASE + GPIO1B_IOMUX_SEL_0); /* FSPI_D0 */
	writel(0xffff1111, GPIO1_IOC_BASE + GPIO1B_IOMUX_SEL_1); /* FSPI_CSN0/D1/D2/CLK */
	writel(0x000f0001, GPIO1_IOC_BASE + GPIO1C_IOMUX_SEL_0); /* FSPI_D3 */
#endif

	/* Enable tsadc phy */
	writel(0x04000000, CRU_PHPL_BASE+ CRU_PHPL_GATE_CON01);
	writel(0x007f007f, PHPL_GRF_BASE + TSADC_CON6);
	writel(0x80088008, PHPL_GRF_BASE + TSADC_CON0);
	tsadc_trigger();

	/* Enable sai2 to iomux path default */
	writel(0xffff00b0, VO_GRF_BASE + SAI2ACODEC_CON2);

	/*
	 * Set the USB2 PHY Port1 in suspend mode and
	 * turn off the differential receiver for both
	 * Port0 and Port1 to save power.
	 */
	writel(0x01ff01d1, VO_GRF_BASE + USBPHY_HOST_CON0);
	writel(0x00000059, USBPHY_APB_BASE + USBPHY_DIFF_RECEIVER_0);
	writel(0x00000059, USBPHY_APB_BASE + USBPHY_DIFF_RECEIVER_1);
#endif

	return 0;
}
#endif

#ifdef CONFIG_ROCKCHIP_OTP
int soc_id_init(void)
{
	struct udevice *dev;
	u16 val, spec;
	int ret;

	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(rockchip_otp),
					  &dev);
	if (ret < 0) {
		printf("No OTP device, ret=%d\n", ret);
		return ret;
	}
	ret = misc_read(dev, REMARK_OTP_SPEC_NUM_OFFSET, &val, 1);
	if (ret < 0) {
		printf("Fail to read otp remark-spec, ret=%d\n", ret);
		return ret;
	}
	if (!val) {
		ret = misc_read(dev, OTP_SPEC_NUM_OFFSET, &val, 1);
		if (ret < 0) {
			printf("Fail to read otp spec, ret=%d\n", ret);
			return ret;
		}
	}

	spec = val & OTP_SPEC_NUM_MASK;
	if (spec == 0x3935)
		printf("SoC: rk3539\n");

	return 0;
}
#endif
