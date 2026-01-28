// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <fdt_support.h>
#include <string.h>
#include <scsi.h>
#include <tee/optee.h>
#include <linux/delay.h>
#include <asm/armv8/mmu.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/smccc.h>
#include <asm/arch-rockchip/vendor.h>

DECLARE_GLOBAL_DATA_PTR;

#define SYS_SGRF_BASE			0x26008000
#define SYS_SGRF_SOC_CON8		0x0040

#define SYS_GRF_BASE			0x26010000
#define SYS_GRF_SOC_CON10		0x0028

#define PHPPHY_CRU_BASE			0x26098000
#define PHPPHY_CRU_PPLL_CON1		0x204

#define PMU1_CRU_BASE			0x260B0000
#define PMU1_CLKSEL_CON03		0x030C	

#define VCCIO7_IOC_BASE			0x26076000
#define VCCIO7_IOC_XIN_UFS_CON		0x640
#define VCCIO7_IOC_GPIO4C_IOMUX_SEL_0	0x090

#define SGRF_FW_BASE			0x2600A000
#define SGRF_MST_DOMAIN_CON0		0x000
#define SGRF_MST_DOMAIN_CON1		0x004
#define SGRF_MST_DOMAIN_CON2		0x008
#define SGRF_MST_DOMAIN_CON3		0x00c
#define SGRF_MST_DOMAIN_CON4		0x010
#define SGRF_MST_DOMAIN_CON5		0x014
#define SGRF_MST_DOMAIN_CON6		0x018
#define SGRF_MST_DOMAIN_CON7		0x01c
#define SGRF_MST_DOMAIN_CON8		0x020
#define SGRF_MST_DOMAIN_CON9		0x024
#define SGRF_MST_DOMAIN_CON10		0x028

#define VCCIO0_3_IOC_BASE		0x26082000
#define VCCIO0_IOC_GPIO1A_IOMUX_SEL_0	0x00020
#define VCCIO0_IOC_GPIO1A_IOMUX_SEL_1	0x00024
#define VCCIO0_IOC_GPIO1B_IOMUX_SEL_0	0x00028

#define VCCIO1_2_4_IOC_BASE		0x26084000
#define VCCIO1_IOC_GPIO2A_IOMUX_SEL_0	0x00040
#define VCCIO1_IOC_GPIO2A_IOMUX_SEL_1	0x00044
#define VCCIO1_IOC_GPIO2A_PULL		0x00220
#define VCCIO4_IOC_GPIO2C_PULL		0x00228
#define VCCIO4_IOC_GPIO2D_PULL		0x0022C
#define VCCIO4_IOC_GPIO2C_IOMUX_SEL_1	0x00054
#define VCCIO4_IOC_GPIO2D_IOMUX_SEL_0	0x00058
#define VCCIO4_IOC_GPIO2D_IOMUX_SEL_1	0x0005C

#define VCCIO5_6_IOC_BASE		0x26086000

#define PMU0_IOC_BASE			0x26072000
#define PMUIO0_IOC_GPIO0A_IOMUX_SEL_0	0x000
#define PMUIO0_IOC_GPIO0A_IOMUX_SEL_1	0x004
#define PMUIO0_IOC_GPIO0B_IOMUX_SEL_0	0x008

#define PMU1_IOC_BASE			0x26074000
#define PMUIO1_IOC_GPIO0B_IOMUX_SEL_1	0x00C
#define PMUIO1_IOC_GPIO0D_IOMUX_SEL_1	0x010

#ifdef CONFIG_ARM64
#include <asm/armv8/mmu.h>
static struct mm_region rk3572_mem_map[] = {
	{
		/* I/O area, PMU_MEM, CBUF, SYSTEM_SRAM */
		.virt = 0x20000000UL,
		.phys = 0x20000000UL,
		.size = 0x20000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* PCIe */
		.virt = 0xc00000000UL,
		.phys = 0xc00000000UL,
		.size = 0x400000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* DDR */
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0x800000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		/* List terminator */
		0,
	}
};


struct mm_region *mem_map = rk3572_mem_map;
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
#endif

void board_set_iomux(enum uclass_id uclass, int devnum, int routing)
{
	switch (uclass) {
	case UCLASS_MMC:
		if (devnum == 0) {
			writel(0xffff1111, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_0);
			writel(0xffff1111, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_1);
			writel(0x0fff0111, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1B_IOMUX_SEL_0);
		} else if (devnum == 1) {
			writel(0xffff2222, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_0);
			writel(0x00ff0022, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_1);
			writel(0x10001000, PMU0_IOC_BASE + PMUIO0_IOC_GPIO0A_IOMUX_SEL_1);
			writel(0x01000100, PMU1_IOC_BASE + PMUIO1_IOC_GPIO0B_IOMUX_SEL_1);
			/* Pull up */
			writel(0x03FF03FF, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_PULL);
		}
		break;
	case UCLASS_MTD:
		if (routing == 0) {
			/* FSPI0 M0 */
			writel(0xffff2222, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_0);
			writel(0xffff2020, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1B_IOMUX_SEL_0);
		} else if (routing == 1) {
			/* FSPI1 M0 */
			writel(0xffff1111, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_0);
			writel(0x00ff0011, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_1);
			/* Pull up */
			writel(0x03ff03ff, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_PULL);
		} else if (routing == 2) {
			/* FSPI1 M1 */
			writel(0xffff2222, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_0);
			writel(0xfff03220, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_1);
			/* Pull up */
			writel(0xff00ff00, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2C_PULL);
			writel(0x30ff30ff, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_PULL);
		}
		break;
	default:
		printf("Bootdev 0x%x is not support\n", uclass);
	}
}

#ifndef CONFIG_TPL_BUILD
int arch_cpu_init(void)
{
#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
	u32 val;

	/* Set emmc master domain */
	val = readl(SGRF_FW_BASE + SGRF_MST_DOMAIN_CON3);
	writel(val | (0x7 << 4) | 0x7, SGRF_FW_BASE + SGRF_MST_DOMAIN_CON3);
	/* Set sdmmc master domain */
	val = readl(SGRF_FW_BASE + SGRF_MST_DOMAIN_CON2);
	writel(val | (0x7 << 16), SGRF_FW_BASE + SGRF_MST_DOMAIN_CON2);
	/* Set usb3otg0 master domain */
	val = readl(SGRF_FW_BASE + SGRF_MST_DOMAIN_CON1);
	writel(val | (0x7 << 20) | (0x7 << 16), SGRF_FW_BASE + SGRF_MST_DOMAIN_CON1);
	/* Set usb3otg0 master domain */
	val = readl(SGRF_FW_BASE + SGRF_MST_DOMAIN_CON1);
	writel(val | (0x7 << 4) | 0x7, SGRF_FW_BASE + SGRF_MST_DOMAIN_CON1);
	/* Set fspi0 and fspi1 master domain */
	val = readl(SGRF_FW_BASE + SGRF_MST_DOMAIN_CON2);
	writel(val | (0x7 << 24) | (0x7 << 28), SGRF_FW_BASE + SGRF_MST_DOMAIN_CON2);

	/*
	 * Enable cci channels for below module AXI R/W
	 * Module: GMAC, PCIe, SATA, USB3
	 */
	writel(0xffffffff, SYS_SGRF_BASE + SYS_SGRF_SOC_CON8);

	/* Enable NOC timeout */
	writel(0xffffffff, SYS_GRF_BASE + SYS_GRF_SOC_CON10);
#endif

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX)
	board_set_iomux(UCLASS_MMC, 0, 0);
#elif defined(CONFIG_ROCKCHIP_SFC_IOMUX)
	/*
	 * (UCLASS_MTD, 0, 0) FSPI0
	 * (UCLASS_MTD, 1, 0) FSPI1 M0
	 */
	board_set_iomux(UCLASS_MTD, 0, 0);
#endif

#if defined(CONFIG_UFS)
	/* Set ref pll 26MHZ */
	writel(0x01c00080, PHPPHY_CRU_BASE + PHPPHY_CRU_PPLL_CON1);
	/* UFS PHY select 26M from ppll */
	writel(0x00010001, PMU1_CRU_BASE + PMU1_CLKSEL_CON03);

	/* set UFS_RSTN to low */
	writel(0x00100000, VCCIO7_IOC_BASE + VCCIO7_IOC_XIN_UFS_CON);
	/* set iomux UFS_REFCLK, UFS_RSTN */
	writel(0x00FF0011, VCCIO7_IOC_BASE + VCCIO7_IOC_GPIO4C_IOMUX_SEL_0);
	udelay(10);
	/* set UFS_RSTN to high */
	writel(0x00100010, VCCIO7_IOC_BASE + VCCIO7_IOC_XIN_UFS_CON);
#endif

	return 0;
}
#endif

