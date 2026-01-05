/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <dm.h>
#include <misc.h>
#include <mmc.h>
#include <spl.h>
#include <image.h>
#include <bidram.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/grf_rv1126b.h>
#include <asm/arch/ioc_rv1126b.h>
#include <asm/arch/rk_atags.h>
#include <asm/arch/param.h>
#include <asm/arch/rockchip_smccc.h>
#include <asm/system.h>

DECLARE_GLOBAL_DATA_PTR;

/* CRU */
#define CRU_BUS_BASE			0x20010000
#define CRU_BUS_GATE_CON06		0x818
#define CRU_BUS_SOFTRST_CON01		0x0a04
#define CRU_PMU_BASE			0x20040000
#define CRU_PMU_SOFTRST_CON03		0x0a0c
#define PERI_CRU_BASE			0x20020000
#define PERICRU_PERI_SOFTRST_CON01	0x0a04

/* GRF */
#define SYS_GRF_BASE			0x20100000
#define HPMCU_CACHE_MISC		0x18
#define TSADC_GRF_CON0			0x50
#define TSADC_GRF_CON1			0x54
#define TSADC_GRF_CON4			0x60
#define TSADC_GRF_CON6			0x68
#define TSADC_GRF_ST1			0x114
#define TSADC_DEF_WIDTH			0x00010001
#define TSADC_TARGET_WIDTH		24000
#define TSADC_DEF_BIAS			0x7f
#define TSADC_MIN_BIAS			0x1
#define TSADC_MAX_BIAS			0x7f
#define TSADC_UNLOCK_VALUE		0xa5
#define TSADC_UNLOCK_VALUE_MASK		(0xff << 16)
#define TSADC_UNLOCK_TRIGGER		BIT(8)
#define TSADC_UNLOCK_TRIGGER_MASK	(BIT(8) << 16)
#define GRF_JTAG_CON0			0x904

#define PERI_GRF_BASE			0x20110000
#define PERI_GRF_USB2HOSTPHY_CON0	0x001c
#define PERI_GRF_USB3DRD_CON1		0x003C

#define VI_GRF_BASE			0x20150000
#define SARADC1_GRF_CON0		0x80
#define SARADC2_GRF_CON0		0x90

#define VEPU_GRF_BASE			0x20160000
#define SARADC0_GRF_CON0		0x0C

/* PMU */
#define PMU_GRF_BASE			0x20130000
#define PMU_GRF_SOC_CON0		0x0000
#define PMU2_BASE			0x20838000
#define PMU2_PWR_GATE_SFTCON0		0x0210

/* GPIO/IOC */
#define GPIO0_BASE			0x20600000
#define GPIO_SWPORT_DR_L		0x00
#define GPIO_SWPORT_DDR_L		0x08

#define PMUIO0_IOC_BASE			0x201a0000
#define GPIO0A_IOMUX_SEL_L		0x0
#define GPIO0A_IOMUX_SEL_H		0x4
#define GPIO0B_IOMUX_SEL_L		0x8

#define VCCIO1_IOC_BASE			0x201b0000
#define GPIO1A_IOMUX_SEL_L		0x20
#define GPIO1A_IOMUX_SEL_H		0x24
#define GPIO1B_IOMUX_SEL_L		0x28
#define GPIO1B_IOMUX_SEL_H		0x2c
#define GPIO1_IOC_IO1_VSEL		0x908
#define VCCIO1_VD_3V3			BIT(15)
#define GPIO1_IOC_GPIO1B_DS_0		0x150
#define GPIO1_IOC_GPIO1B_DS_1		0x154
#define GPIO1_IOC_GPIO1B_DS_2		0x158
#define GPIO1_IOC_GPIO1B_DS_3		0x15c

#define VCCIO2_IOC_BASE			0x201b8000
#define GPIO2A_IOMUX_SEL_L		0x40
#define GPIO2A_IOMUX_SEL_H		0x44
#define GPIO2A_PULL			0x320

#define VCCIO3_IOC_BASE			0x201c0000
#define GPIO3A_IOMUX_SEL_L		0x60
#define GPIO3A_IOMUX_SEL_H		0x64
#define GPIO3B_IOMUX_SEL_H		0x6c
#define GPIO3A_PULL			0x330

#define VCCIO6_IOC_BASE			0x201D8000
#define GPIO6C_PULL			0x368
#define	GPIO6C_IE			0x468
#define	GPIO6C_SMT			0x568

#define VCCIO7_IOC_BASE			0x201E0000
#define GRF_DSM_IOC_CON0		0x0ca0

/* SGRF/FIREWALL */
#define SGRF_SYS_BASE			0x20220000
#define SGRF_HPMCU_BOOT_ADDR		0x0c
#define SGRF_SYS_AHB_SECURE_SGRF_CON	0x14
#define SGRF_SYS_AXI_SECURE_SGRF_CON0	0x18
#define FIREWALL_SLV_CON0		0x20
#define FIREWALL_SLV_CON1		0x24
#define FIREWALL_SLV_CON2		0x28
#define FIREWALL_SLV_CON3		0x2c
#define FIREWALL_SLV_CON4		0x30
#define FIREWALL_SLV_CON5		0x34
#define OTP_SGRF_CON			0x1c

#define SGRF_PMU_BASE			0x20230000
#define SGRF_PMU_SOC_CON0		0x00
#define SGRF_PMU_SOC_CON1		0x04
#define SGRF_LPMCU_BOOT_ADDR		0x20

#define PVTPLL_ISP_BASE			0x21C60000
#define PVTPLL_ENC_BASE			0x21F00000
#define PVTPLL_AIISP_BASE		0x21FC0000
#define PVTPLL_GCK_CFG			0x20
#define PVTPLL_GCK_LEN			0x24

/*
 * If need less wait time, please measure required
 * power down time on actual hardware.
 */
#define SDMMC_PWR_DOWN_MS		50

#ifdef CONFIG_ARM64
#include <asm/armv8/mmu.h>

static struct mm_region rv1126b_mem_map[] = {
#ifndef CONFIG_SPL_BUILD
	{
		.virt = 0x00010000UL,
		.phys = 0x00010000UL,
		.size = 0x0fff0000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	},
#endif
	{
		.virt = 0x20000000UL,
		.phys = 0x20000000UL,
		.size = 0x02800000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x3ff1e000UL,
		.phys = 0x3ff1e000UL,
		.size = 0x000e2000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0x100000000UL - 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = rv1126b_mem_map;
#endif

void board_debug_uart_init(void)
{
	/* No need to change uart in most time. */
}

void board_set_iomux(enum if_type if_type, int devnum, int routing)
{
	switch (if_type) {
	case IF_TYPE_MMC:
		if (devnum == 0) {
			writel(0xffff1111, VCCIO1_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xffff1111, VCCIO1_IOC_BASE + GPIO1A_IOMUX_SEL_H);
			writel(0xf0f01010, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		} else if (devnum == 1) {
#if CONFIG_SPL_BUILD
			/* set SDMMC D0-3/CMD/CLK to gpio and pull down */
			writel(0xffff0000, VCCIO2_IOC_BASE + GPIO2A_IOMUX_SEL_L);
			writel(0x00ff0000, VCCIO2_IOC_BASE + GPIO2A_IOMUX_SEL_H);
			writel(0x00f00000, PMUIO0_IOC_BASE + GPIO0A_IOMUX_SEL_H);
			writel(0x0fff0aaa, VCCIO2_IOC_BASE + GPIO2A_PULL);

			/* SDMMC PWREN GPIO0B0 power down and power up */
			writel(0x01000100, GPIO0_BASE + GPIO_SWPORT_DR_L);
			writel(0x01000100, GPIO0_BASE + GPIO_SWPORT_DDR_L);
			mdelay(SDMMC_PWR_DOWN_MS);
			writel(0x01000000, GPIO0_BASE + GPIO_SWPORT_DR_L);
#endif
			/* set SDMMC D0-3/CMD/CLK and pull up */
			writel(0xffff1111, VCCIO2_IOC_BASE + GPIO2A_IOMUX_SEL_L);
			writel(0x00ff0011, VCCIO2_IOC_BASE + GPIO2A_IOMUX_SEL_H);
			writel(0x00f00010, PMUIO0_IOC_BASE + GPIO0A_IOMUX_SEL_H);
			writel(0x0fff0555, VCCIO2_IOC_BASE + GPIO2A_PULL);
		} else if (devnum == 2) {
			writel(0xffff1111, VCCIO3_IOC_BASE + GPIO3A_IOMUX_SEL_L);
			writel(0x00ff0011, VCCIO3_IOC_BASE + GPIO3A_IOMUX_SEL_H);
			writel(0x0f000300, VCCIO3_IOC_BASE + GPIO3B_IOMUX_SEL_H);
			/* Pull up */
			writel(0x0ffc0554, VCCIO2_IOC_BASE + GPIO3A_PULL);
		}
		break;
	case IF_TYPE_MTD:
		if (routing == 0) {
			/* FSPI0 M0 */
			writel(0x0f0f0101, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_L);
			writel(0xffff1111, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_H);
		} else if (routing == 1) {
			/* FSPI1 M0 */
			writel(0x0fff0111, PMUIO0_IOC_BASE + GPIO0B_IOMUX_SEL_L);
			writel(0xff001100, PMUIO0_IOC_BASE + GPIO0A_IOMUX_SEL_H);
			writel(0x00f00010, PMUIO0_IOC_BASE + GPIO0A_IOMUX_SEL_L);
		} else if (routing == 2) {
			/* FSPI1 M1 */
			writel(0xffff2222, VCCIO1_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xf0f02020, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		}
		break;
	default:
		printf("Bootdev 0x%x is not support\n", if_type);
	}
}

void board_unset_iomux(enum if_type if_type, int devnum, int routing)
{
	switch (if_type) {
	case IF_TYPE_MMC:
		if (devnum == 0) {
			writel(0xffff0000, VCCIO1_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xffff0000, VCCIO1_IOC_BASE + GPIO1A_IOMUX_SEL_H);
			writel(0xf0f00000, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		} else if (devnum == 1) {
			/* SDMMC0_D2,D3 -> JTAG_TMS_M1, JTAG_TCK_M1 */
			writel(0xffff4400, VCCIO2_IOC_BASE + GPIO2A_IOMUX_SEL_L);
			/* Other SDMMC0 PINS -> GPIO */
			writel(0x00ff0000, VCCIO2_IOC_BASE + GPIO2A_IOMUX_SEL_H);
			writel(0x00f00000, PMUIO0_IOC_BASE + GPIO0A_IOMUX_SEL_H);
			/* Pull down */
			writel(0x0fff0aaa, VCCIO2_IOC_BASE + GPIO2A_PULL);
		} else if (devnum == 2) {
			writel(0xffff0000, VCCIO3_IOC_BASE + GPIO3A_IOMUX_SEL_L);
			writel(0x00ff0000, VCCIO3_IOC_BASE + GPIO3A_IOMUX_SEL_H);
			writel(0x0f000000, VCCIO3_IOC_BASE + GPIO3B_IOMUX_SEL_H);
			/* Pull down */
			writel(0x0ffc0000, VCCIO2_IOC_BASE + GPIO3A_PULL);
		}
		break;
	case IF_TYPE_MTD:
		if (routing == 0) {
			/* FSPI0 M0 */
			writel(0x0f0f0000, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_L);
			writel(0xffff0000, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_H);
			writel(0x00f00000, VCCIO1_IOC_BASE + GPIO1A_IOMUX_SEL_H);
		} else if (routing == 1) {
			/* FSPI1 M0 */
			writel(0x0fff0000, PMUIO0_IOC_BASE + GPIO0B_IOMUX_SEL_L);
			writel(0xff000000, PMUIO0_IOC_BASE + GPIO0A_IOMUX_SEL_H);
			writel(0x00f00000, PMUIO0_IOC_BASE + GPIO0A_IOMUX_SEL_L);
		} else if (routing == 2) {
			/* FSPI1 M1 */
			writel(0xffff0000, VCCIO1_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xf0f00000, VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		}
		break;
	default:
		break;
	}
}

#ifdef CONFIG_SPL_BUILD
void rockchip_stimer_init(void)
{
	u32 reg;

	/* If Timer already enabled, don't re-init it */
	reg = readl(CONFIG_ROCKCHIP_STIMER_BASE + 0x4);
	if (reg & 0x1)
		return;
#ifdef COUNTER_FREQUENCY
	asm volatile("msr CNTFRQ_EL0, %0" : : "r" (COUNTER_FREQUENCY));
#endif
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x14);
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x18);
	writel(0x00010001, CONFIG_ROCKCHIP_STIMER_BASE + 0x04);
}

void spl_board_storages_fixup(struct spl_image_loader *loader)
{
	if (!loader)
		return;

	if (loader->boot_device == BOOT_DEVICE_MMC2)
		/* Unset the sdmmc0 iomux */
		board_unset_iomux(IF_TYPE_MMC, 1, 0);
}

static void tsadc_trigger(void)
{
	writel(TSADC_UNLOCK_VALUE | TSADC_UNLOCK_VALUE_MASK,
	       SYS_GRF_BASE + TSADC_GRF_CON1);
	writel(TSADC_UNLOCK_TRIGGER | TSADC_UNLOCK_TRIGGER_MASK,
	       SYS_GRF_BASE + TSADC_GRF_CON1);
	writel(TSADC_UNLOCK_TRIGGER_MASK, SYS_GRF_BASE + TSADC_GRF_CON1);
}

static void tsadc_adjust_bias_current(void)
{
	struct udevice *dev;
	int8_t offset_otp = 0;
	uint8_t bias_otp = 0;
	int16_t offset = 0;
	u32 bias, value = 0, width = 0;
	int ret = 0;

	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_GET_DRIVER(rockchip_otp), &dev);
	if (ret) {
		printf("failed to get otp device for tsadc\n");
	} else {
		if (misc_read(dev, 0x72, &bias_otp, 1))
			printf("failed to get otp tsadc bias\n");
		if (misc_read(dev, 0x73, (uint8_t *)&offset_otp, 1))
			printf("failed to get otp tsadc offset\n");

		if (offset_otp) {
			offset = (int16_t)offset_otp * 10;
			offset = 0x963f - offset;
			writel((uint32_t)offset | 0xffff0000,
			       SYS_GRF_BASE + TSADC_GRF_CON4);
			tsadc_trigger();
		}
		printf("tsadc otp bias=0x%x offset=0x%x, grf offset=0x%x\n",
		       bias_otp, (uint8_t)offset_otp, (uint16_t)offset);

		if (bias_otp) {
			if (bias_otp > TSADC_MAX_BIAS)
				bias_otp = TSADC_MAX_BIAS;
			if (bias_otp < TSADC_MIN_BIAS)
				bias_otp = TSADC_MIN_BIAS;
			writel((TSADC_MAX_BIAS << 16) | bias_otp,
			       SYS_GRF_BASE + TSADC_GRF_CON6);
			tsadc_trigger();
			return;
		}
	}

	value = readl(SYS_GRF_BASE + TSADC_GRF_ST1);
	if (!value || value == TSADC_DEF_WIDTH) {
		printf("Invalid tsadc width\n");
	} else {
		width = (value & 0x0000ffff) + ((value & 0xffff0000) >> 16);
		bias = width * TSADC_DEF_BIAS / TSADC_TARGET_WIDTH;
		if (bias > TSADC_MAX_BIAS)
			bias = TSADC_MAX_BIAS;
		if (bias < TSADC_MIN_BIAS)
			bias = TSADC_MIN_BIAS;
		printf("tsadc width=0x%x %u lo=%u hi=%u, bias=0x%x\n",
		       value, ((value & 0xffff0000) >> 16) + (value & 0xffff),
		       value & 0xffff, (value & 0xffff0000) >> 16, bias);
		writel((TSADC_MAX_BIAS << 16) | bias,
		       SYS_GRF_BASE + TSADC_GRF_CON6);
		tsadc_trigger();
	}
}

void spl_rk_board_prepare_for_jump(struct spl_image_info *spl_image)
{
	tsadc_adjust_bias_current();
}

int spl_fit_standalone_release(char *id, uintptr_t entry_point)
{
	if (!strcmp(id, "mcu0")) {
		writel(0x1e001e0, CRU_BUS_BASE + CRU_BUS_SOFTRST_CON01);
		writel(entry_point, SGRF_SYS_BASE + SGRF_HPMCU_BOOT_ADDR);
		writel(0x1 << 20, SGRF_SYS_BASE + SGRF_SYS_AHB_SECURE_SGRF_CON);
		writel(0x1e00000, CRU_BUS_BASE + CRU_BUS_SOFTRST_CON01);
	} else if (!strcmp(id, "mcu1")) {
		writel(0x1c001c, CRU_PMU_BASE + CRU_PMU_SOFTRST_CON03);
		writel(entry_point, SGRF_PMU_BASE + SGRF_LPMCU_BOOT_ADDR);
		writel(0x1 << 23, SGRF_PMU_BASE + SGRF_PMU_SOC_CON0);
		writel(0x1c0000, CRU_PMU_BASE + CRU_PMU_SOFTRST_CON03);
	}

	return 0;
}

#ifdef CONFIG_ROCKCHIP_META
void rk_meta_process(void)
{
	writel(0x00080008, SYS_GRF_BASE + HPMCU_CACHE_MISC);
}
#endif
#endif

#ifndef CONFIG_TPL_BUILD
int arch_cpu_init(void)
{
#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
	/* Enable npu pd */
	writel(0x00010000, PMU2_BASE + PMU2_PWR_GATE_SFTCON0);
	/* Set emmc master secure */
	writel(0x10000, SGRF_SYS_BASE + SGRF_SYS_AHB_SECURE_SGRF_CON);
	/* Set fspi master secure */
	writel(0x20000, SGRF_SYS_BASE + SGRF_SYS_AHB_SECURE_SGRF_CON);
	/* Set sdmmc0 master secure */
	writel(0x40000, SGRF_SYS_BASE + SGRF_SYS_AHB_SECURE_SGRF_CON);
	/* Set sdmmc1 master secure */
	writel(0x80000, SGRF_SYS_BASE + SGRF_SYS_AHB_SECURE_SGRF_CON);
	/* Set rkce master secure */
	writel(0x80038000, SGRF_SYS_BASE + SGRF_SYS_AXI_SECURE_SGRF_CON0);
	/* Set decom master secure */
	writel(0xC00000, SGRF_SYS_BASE + SGRF_SYS_AXI_SECURE_SGRF_CON0);

	/* Set all devices slave non-secure */
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_SLV_CON0);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_SLV_CON1);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_SLV_CON2);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_SLV_CON3);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_SLV_CON4);
	writel(0xffff0000, SGRF_SYS_BASE + FIREWALL_SLV_CON5);
	/* Set OTP to none secure mode */
	writel(0x00020000, SGRF_SYS_BASE + OTP_SGRF_CON);

	/* Set usb3phy clamp enable */
	writel(0x40000000, PMU_GRF_BASE + PMU_GRF_SOC_CON0);

	/* Assert the pipe phy reset and de-assert when in use */
	writel(0x00800080, PERI_CRU_BASE + PERICRU_PERI_SOFTRST_CON01);

	/* Restore pipe phy status to default from phy */
	writel(0xffff1100, PERI_GRF_BASE + PERI_GRF_USB3DRD_CON1);

	/* Set the USB 2.0 PHY Port1 to enter the sleep mode to save power consumption */
	writel(0x01ff01d1, PERI_GRF_BASE + PERI_GRF_USB2HOSTPHY_CON0);

	/* Enable tsadc phy */
	writel(0x01000000, CRU_BUS_BASE + CRU_BUS_GATE_CON06);
	writel(0x80788028, SYS_GRF_BASE + TSADC_GRF_CON0);
	writel(0x007f007f, SYS_GRF_BASE + TSADC_GRF_CON6);
	writel(0x00ff00a5, SYS_GRF_BASE + TSADC_GRF_CON1);
	writel(0x01000100, SYS_GRF_BASE + TSADC_GRF_CON1);
	writel(0x01000000, SYS_GRF_BASE + TSADC_GRF_CON1);

	/* set saradc ibp to 7 */
	writel(0x00700070, VEPU_GRF_BASE + SARADC0_GRF_CON0);
	writel(0x00700070, VI_GRF_BASE + SARADC1_GRF_CON0);
	writel(0x00700070, VI_GRF_BASE + SARADC2_GRF_CON0);

	/* FEPHY: disable gpio's smt and ie, keep high-z to save power consumption */
	writel(0xff000000, VCCIO6_IOC_BASE + GPIO6C_PULL);
	writel(0x00f00000, VCCIO6_IOC_BASE + GPIO6C_IE);
	writel(0x00f00000, VCCIO6_IOC_BASE + GPIO6C_SMT);

	/* Solve dsm pop pulse */
	writel(0xfffff990, VCCIO7_IOC_BASE + GRF_DSM_IOC_CON0);

	/* Enable pvtpll for isp/enc/aiisp */
	writel(0x01ff0064, PVTPLL_ISP_BASE + PVTPLL_GCK_LEN);
	writel(0x00230023, PVTPLL_ISP_BASE + PVTPLL_GCK_CFG);
	writel(0x01ff0058, PVTPLL_ENC_BASE + PVTPLL_GCK_LEN);
	writel(0x00230023, PVTPLL_ENC_BASE + PVTPLL_GCK_CFG);
	writel(0x01ff0008, PVTPLL_AIISP_BASE + PVTPLL_GCK_LEN);
	writel(0x00230023, PVTPLL_AIISP_BASE + PVTPLL_GCK_CFG);

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX)
	board_set_iomux(IF_TYPE_MMC, 0, 0);
#elif defined(CONFIG_ROCKCHIP_SFC_IOMUX)
	/*
	 * (IF_TYPE_MTD, 0, 0) FSPI0
	 * (IF_TYPE_MTD, 0, 1) FSPI1 M0
	 * (IF_TYPE_MTD, 0, 2) FSPI1 M1
	 */
	board_set_iomux(IF_TYPE_MTD, 0, 0);
#endif

#if defined(CONFIG_ROCKCHIP_SDMMC_IOMUX)
	/* Set the sdmmc iomux and power cycle */
	board_set_iomux(IF_TYPE_MMC, 1, 0);
#endif

	/*
	 * Fix fspi io ds level:
	 *
	 * level 3 for 1V8(default)
	 * level 4 for 3V3
	 */
	if (readl(VCCIO1_IOC_BASE + GPIO1B_IOMUX_SEL_H) == 0x1111) {
		if (readl(VCCIO1_IOC_BASE + GPIO1_IOC_IO1_VSEL) & VCCIO1_VD_3V3) {
			writel(0x003f001f, VCCIO1_IOC_BASE + GPIO1_IOC_GPIO1B_DS_0);
			writel(0x003f001f, VCCIO1_IOC_BASE + GPIO1_IOC_GPIO1B_DS_1);
			writel(0x3f3f1f1f, VCCIO1_IOC_BASE + GPIO1_IOC_GPIO1B_DS_2);
			writel(0x3f3f1f1f, VCCIO1_IOC_BASE + GPIO1_IOC_GPIO1B_DS_3);
		}
	}
#endif

	return 0;
}

#if defined(CONFIG_ROCKCHIP_PRELOADER_ATAGS)
#if defined(CONFIG_SPL_KERNEL_BOOT)
int rk_board_fit_image_post_process(void *fit, int node, ulong *load_addr,
				     ulong **src_addr, size_t *src_len)
{
	struct tag *t;
	uint8_t image_arch;
	int count;

	/* Only current node is kernel needs go further. */
	if (!fit_image_check_type(fit, node, IH_TYPE_KERNEL))
		return 0;

	t = atags_get_tag(ATAG_DDR_MEM);
	count = t->u.ddr_mem.count;
	if (!t || !count)
		return -EINVAL;

	if (t->u.ddr_mem.bank[0] == 0x0) {
		/*
		 * Dynamically create memory mapping for 0x10000-0x10000000(256M)
		 * only when needed. This avoids too much boot time without 4G DDR support.
		 */
		mmu_set_region_dcache_behaviour(0x10000,
						0x10000000 - 0x10000,
						DCACHE_WRITEBACK);

		if (fit_image_get_arch(fit, node, &image_arch))
			return -EINVAL;

		if (image_arch == IH_ARCH_ARM) {
			*load_addr = KERNEL_ADDR1_AARCH32_R;
		} else if (image_arch == IH_ARCH_ARM64) {
			*load_addr = KERNEL_ADDR1_R;
		} else {
			printf("Unknown image arch: 0x%x\n", image_arch);
			return -EINVAL;
		}
		printf("Relocate kernel to 0x%lx.\n", *load_addr);
	}

	return 0;
}

void board_bidram_fixup(void)
{
	struct memblock *mem;
	struct tag *t;
	u64 size = 0;
	int i, count, n, noffset, num = 0;

	t = atags_get_tag(ATAG_DDR_MEM);
	count = t->u.ddr_mem.count;
	if (!t || !count)
		return;
	if (t->u.ddr_mem.bank[0] != 0x0)
		return;
	size = t->u.ddr_mem.bank[count];

	/* Record current bi_dram banks. */
	mem = calloc(count + MEM_RESV_COUNT, sizeof(*mem));
	if (!mem) {
		printf("Calloc ddr memory failed\n");
		return;
	}
	for (i = 0, n = 0; i < MEM_RESV_COUNT; i++) {
		if (!gd->bd->bi_dram[i].size)
			continue;
		mem[n].base = gd->bd->bi_dram[i].start;
		mem[n].size = gd->bd->bi_dram[i].size;
		n++;

		assert(n < MEM_RESV_COUNT);
	}

	/* Handle that always used as DDR. */
	gd->bd->bi_dram[num].start = 0x00010000;
	gd->bd->bi_dram[num].size  = 0x0fff0000;
	num++;

	/* Remap DSMC_MEM to DDR. */
	noffset = fdt_path_offset(gd->fdt_blob, "/dsmc@21ca0000");
	if ((noffset < 0) || !fdtdec_get_is_enabled(gd->fdt_blob, noffset)) {
#ifdef CONFIG_SPL_BUILD
		writel(0x08000800, SGRF_PMU_BASE + SGRF_PMU_SOC_CON1);
#elif CONFIG_ROCKCHIP_SMCCC
		sip_smc_secure_reg_write(SGRF_PMU_BASE + SGRF_PMU_SOC_CON1, 0x08000800);
#endif
		gd->bd->bi_dram[num].start = 0x10000000;
		gd->bd->bi_dram[num].size  = 0x10000000;
		num++;
	}

	if (size > SZ_512M) {
		/* Handle that always used as DDR. */
		gd->bd->bi_dram[num].start = 0x22800000;
		gd->bd->bi_dram[num].size  = 0x01800000;
		num++;

		/* Remap FSPI_PMU_XIP to DDR. */
#ifdef CONFIG_SPL_BUILD
		writel(0x20002000, SGRF_PMU_BASE + SGRF_PMU_SOC_CON1);
#elif CONFIG_ROCKCHIP_SMCCC
		sip_smc_secure_reg_write(SGRF_PMU_BASE + SGRF_PMU_SOC_CON1, 0x20002000);
#endif
		gd->bd->bi_dram[num].start = 0x24000000;
		gd->bd->bi_dram[num].size  = 0x02000000;
		num++;

		/* Handle that always used as DDR. */
		gd->bd->bi_dram[num].start = 0x26000000;
		gd->bd->bi_dram[num].size  = 0x02000000;
		num++;

		/* Remap FSPI_XIP to DDR. */
#ifdef CONFIG_SPL_BUILD
		writel(0x10001000, SGRF_PMU_BASE + SGRF_PMU_SOC_CON1);
#elif CONFIG_ROCKCHIP_SMCCC
		sip_smc_secure_reg_write(SGRF_PMU_BASE + SGRF_PMU_SOC_CON1, 0x10001000);
#endif
		gd->bd->bi_dram[num].start = 0x28000000;
		gd->bd->bi_dram[num].size  = 0x08000000;
		num++;

		/* Handle that always used as DDR. */
		gd->bd->bi_dram[num].start = 0x30000000;
		gd->bd->bi_dram[num].size  = 0x0ff1e000;
		num++;
	}

	/* Append recorded bi_dram banks. */
	for (n = 0; n < MEM_RESV_COUNT; n++) {
		if (!mem[n].size)
			continue;
		gd->bd->bi_dram[num].start = mem[n].base;
		gd->bd->bi_dram[num].size  = mem[n].size;
		num++;

		assert(num < MEM_RESV_COUNT);
	}
}

void spl_fdt_fixup_memory(struct spl_image_info *spl_image)
{
	struct memblock *list;
	void *blob = spl_image->fdt_addr;
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
	int i, count = 0;

	if (fdt_check_header(blob)) {
		printf("Invalid dtb\n");
		return;
	}

	list = param_parse_ddr_mem(&count);
	if (!list) {
		printf("Can't get dram banks\n");
		return;
	}

	if (count > CONFIG_NR_DRAM_BANKS) {
		printf("Dram banks num=%d, over %d\n", count, CONFIG_NR_DRAM_BANKS);
		return;
	}

	for (i = 0; i < count; i++) {
		gd->bd->bi_dram[i].start = list[i].base < SZ_1G ? SZ_1G : list[i].base;
		gd->bd->bi_dram[i].size = ddr_mem_get_usable_size(list[i].base, list[i].size);
	}

	board_bidram_fixup();

	for (i = 0, count = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		start[i] = gd->bd->bi_dram[i].start;
		size[i] = gd->bd->bi_dram[i].size;
#ifdef SPL_RESV_MEM_SIZE
		if ((start[i] == CONFIG_SYS_SDRAM_BASE) &&
		    (start[i] + size[i] > CONFIG_SYS_SDRAM_BASE + SPL_RESV_MEM_SIZE)) {
			start[i] += SPL_RESV_MEM_SIZE;
			size[i] -= SPL_RESV_MEM_SIZE;
		}
#endif
		if (size[i] == 0)
			continue;
		debug("Adding bank: 0x%08llx - 0x%08llx (size: 0x%08llx)\n",
		      start[i], start[i] + size[i], size[i]);
		count++;
	}

	fdt_increase_size(blob, 512);

	if (fdt_fixup_memory_banks(blob, start, size, count)) {
		printf("Fixup kernel dtb memory node failed.\n");
		return;
	}

	return;
}

u64 board_bidram_append_size(void)
{
	struct tag *t;
	int count;

	t = atags_get_tag(ATAG_DDR_MEM);
	count = t->u.ddr_mem.count;
	if (!t || !count)
		return 0;

	if (t->u.ddr_mem.bank[0] == 0x0)
		return t->u.ddr_mem.bank[count] > SZ_1G ?
		       SZ_1G : t->u.ddr_mem.bank[count];

	return 0;
}

bool rk_board_req_mem_layout1(void)
{
	/*
	 * 1. If atags first dram bank addr != 0 (0~1G is not available),
	 *    use ENV_MEM_LAYOUT_SETTINGS, all Image run at 1G+ space.
	 * 2. If atags first dram bank addr == 0 (max 4G size and 0~1G is available)
	 *    use ENV_MEM_LAYOUT_SETTINGS1, uboot and atf run at 1G+,
	 *    while kernel and ramdisk run at 0-1G.
	 */
	struct tag *t;
	int count;

	t = atags_get_tag(ATAG_DDR_MEM);
	count = t->u.ddr_mem.count;
	if (!t || !count)
		return false;

	if (t->u.ddr_mem.bank[0] == 0x0)
		return true;
	else
		return false;
}
#endif
#endif
#endif

int fit_standalone_release(char *id, uintptr_t entry_point)
{
	if (!strcmp(id, "hpmcu")) {
		writel(0x1e001e0, CRU_BUS_BASE + CRU_BUS_SOFTRST_CON01);
		sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
			ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
			entry_point);
		writel(0x1e00000, CRU_BUS_BASE + CRU_BUS_SOFTRST_CON01);
	}

	return 0;
}

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX) && defined(CONFIG_ROCKCHIP_SFC_IOMUX)
#error FSPI0 M0 and eMMC iomux is incompatible for rv1126b Soc. You should disable one of them.
#endif
