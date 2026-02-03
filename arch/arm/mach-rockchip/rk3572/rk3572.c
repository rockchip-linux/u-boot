// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <fdt_support.h>
#include <string.h>
#include <spl.h>
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
#define SYS_GRF_SOC_CON03		0x000c
#define SYS_GRF_SOC_CON10		0x0028

#define BIGCORE_GRF_BASE			0x26012000
#define BIGCORE_GRF_CPU_CON1			0x0038
#define BIGCORE_GRF_CPU_MEM_CFG_HDSPRF		0x0040
#define BIGCORE_GRF_CPU_MEM_CFG_HSSPRF_LOW	0x0044

#define LITCORE0_GRF_BASE			0x26014000
#define LITCORE0_GRF_CPU_CON1			0x0038
#define LITCORE0_GRF_CPU_MEM_CFG_HDSPRF		0x0040
#define LITCORE0_GRF_CPU_MEM_CFG_HSSPRF_LOW	0x0044

#define LITCORE1_GRF_BASE			0x26016000
#define LITCORE1_GRF_CPU_CON1			0x0038
#define LITCORE1_GRF_CPU_MEM_CFG_HDSPRF		0x0040
#define LITCORE1_GRF_CPU_MEM_CFG_HSSPRF_LOW	0x0044

#define CCI_GRF_BASE			0x26018000
#define CCI_GRF_CCI_CON0		0x0000
#define CCI_GRF_CCI_CON1		0x0004
#define CCI_GRF_CCI_MEM_CFG_HDSPRF	0x0054

#define PMU1_GRF_BASE			0x26042000
#define PMU1_GRF_SOC_CON0		0x0

#define PHPPHY_CRU_BASE			0x26098000
#define PHPPHY_CRU_PPLL_CON1		0x204
#define PHPPHY_CRU_SOFTRST_CON02	0xA08

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

#define USB2PHY0_GRF_BASE		0x26054000
#define USB2PHY1_GRF_BASE		0x26056000
#define USB2PHY_GRF_CON4		0x010
#define USB2PHY_GRF_DBG_CON		0x0040
#define USB2PHY_GRF_LS_TIMEOUT		0x0044
#define USB2PHY_GRF_LS_DEB		0x0048
#define USB2PHY_GRF_RX_TIMEOUT		0x004c
#define USB2PHY_GRF_SEQ_LIMT		0x0050

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
#define PMUIO0_IOC_GPIO0B_IE		0x304

#define PMU1_IOC_BASE			0x26074000
#define PMUIO1_IOC_GPIO0B_IOMUX_SEL_1	0x00C
#define PMUIO1_IOC_GPIO0D_IOMUX_SEL_1	0x010

const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/soc/mmc@2a010000",
	[BROM_BOOTSOURCE_SPINOR] = "/soc/spi@2a020000",
	[BROM_BOOTSOURCE_SPINAND] = "/soc/spi@2a020000",
	[BROM_BOOTSOURCE_SD] = "/soc/mmc@2a090000",
	[BROM_BOOTSOURCE_UFS] = "/soc/ufs@29e00000",
};

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

void board_debug_uart_init(void)
{
	return;
}

#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
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
			writel(0xf0f02020, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1B_IOMUX_SEL_0);
#if defined(CONFIG_ROCKCHIP_SFC_OCTAL_SETTING) || defined(CONFIG_SUPPORT_USBPLUG)
			writel(0xffff2222, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_1);
			writel(0x0f0f0203, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1B_IOMUX_SEL_0);
#endif
		} else if (routing == 1) {
			/* FSPI1 M0 */
			writel(0xffff1111, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_0);
			writel(0x00ff0011, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_1);
			/* Pull up */
			writel(0x03ff03ff, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_PULL);
		} else if (routing == 2) {
			/* FSPI1 M1 */
			writel(0xffff2222, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_0);
			writel(0x0ff00220, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_1);
			/* Pull up */
			writel(0xff00ff00, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2C_PULL);
			writel(0x30ff30ff, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_PULL);
#if defined(CONFIG_ROCKCHIP_SFC_OCTAL_SETTING) || defined(CONFIG_SUPPORT_USBPLUG)
			writel(0xffff2222, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2C_IOMUX_SEL_1);
			writel(0xf00f3002, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_1);
#endif
		}
		break;
	default:
		printf("Bootdev 0x%x is not support\n", uclass);
	}
}

void board_unset_iomux(enum uclass_id uclass, int devnum, int routing)
{
	switch (uclass) {
	case UCLASS_MMC:
		if (devnum == 0) {
			writel(0xffff0000, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_0);
			writel(0xffff0000, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_1);
			writel(0x0fff0000, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1B_IOMUX_SEL_0);
		} else if (devnum == 1) {
			writel(0xffff0000, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_0);
			writel(0x00ff0000, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_1);
			writel(0x10000000, PMU0_IOC_BASE + PMUIO0_IOC_GPIO0A_IOMUX_SEL_1);
			writel(0x01000000, PMU1_IOC_BASE + PMUIO1_IOC_GPIO0B_IOMUX_SEL_1);
			/* Pull up */
			writel(0x03FF0155, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_PULL);
		}
		break;
	case UCLASS_MTD:
		if (routing == 0) {
			/* FSPI0 M0 */
			writel(0xffff0000, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_0);
			writel(0xf0f00000, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1B_IOMUX_SEL_0);
#if defined(CONFIG_ROCKCHIP_SFC_OCTAL_SETTING) || defined(CONFIG_SUPPORT_USBPLUG)
			writel(0xffff0000, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1A_IOMUX_SEL_1);
			writel(0x0f0f0000, VCCIO0_3_IOC_BASE + VCCIO0_IOC_GPIO1B_IOMUX_SEL_0);
#endif
		} else if (routing == 1) {
			/* FSPI1 M0 */
			writel(0xffff0000, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_0);
			writel(0x00ff0000, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_IOMUX_SEL_1);
			/* Pull up */
			writel(0x03ff0155, VCCIO1_2_4_IOC_BASE + VCCIO1_IOC_GPIO2A_PULL);
		} else if (routing == 2) {
			/* FSPI1 M1 */
			writel(0xffff0000, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_0);
			writel(0x0ff00000, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_1);
			/* Pull up */
			writel(0xff005500, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2C_PULL);
			writel(0x30ff1055, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_PULL);
#if defined(CONFIG_ROCKCHIP_SFC_OCTAL_SETTING) || defined(CONFIG_SUPPORT_USBPLUG)
			writel(0xffff0000, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2C_IOMUX_SEL_1);
			writel(0xf00f0000, VCCIO1_2_4_IOC_BASE + VCCIO4_IOC_GPIO2D_IOMUX_SEL_1);
#endif
		}
		break;
	default:
		printf("Bootdev 0x%x is not support\n", uclass);
	}
}
#endif

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

void spl_board_storages_fixup(struct spl_image_loader *loader)
{
	if (!loader)
		return;

	if (loader->boot_device == BOOT_DEVICE_MMC2)
		/* Unset the sdmmc0 iomux */
		board_unset_iomux(UCLASS_MMC, 1, 0);
}
#endif

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

	/* Enable NOC timeout, except DSMC */
	writel(0xfffffff7, SYS_GRF_BASE + SYS_GRF_SOC_CON10);

	/*
	 * bit0: Force rdata all 1's when pcie slv err occur
	 * bit1: Force rresp 2'b0 when pcie slv err occur
	 * We forces rdata to all 1's but doesn't force rresp
	 * to 0(normal response) to CPU, so NOC timeout can capture
	 * it and print a log in the irq for debugging purpose.
	 */
	writel(0x10001, CCI_GRF_BASE + CCI_GRF_CCI_CON1);

	/*
	 * The default read margin (RM) value is 4, and the CPU startup
	 * voltage is 850mV. Set RM value to 2 provides the optimal read
	 * margin for this voltage level.
	 */
	/* Set litcore0 RM to 2 */
	writel(0x00010001, LITCORE0_GRF_BASE + LITCORE0_GRF_CPU_CON1);
	writel(0x001c0008, LITCORE0_GRF_BASE + LITCORE0_GRF_CPU_MEM_CFG_HDSPRF);
	writel(0x001c0008, LITCORE0_GRF_BASE + LITCORE0_GRF_CPU_MEM_CFG_HSSPRF_LOW);
	/* Set litcore1 RM to 2 */
	writel(0x00010001, LITCORE1_GRF_BASE + LITCORE1_GRF_CPU_CON1);
	writel(0x001c0008, LITCORE1_GRF_BASE + LITCORE1_GRF_CPU_MEM_CFG_HDSPRF);
	writel(0x001c0008, LITCORE1_GRF_BASE + LITCORE1_GRF_CPU_MEM_CFG_HSSPRF_LOW);
	/* Set cci RM to 2 */
	writel(0x40004000, CCI_GRF_BASE + CCI_GRF_CCI_CON0);
	writel(0x001c0008, CCI_GRF_BASE + CCI_GRF_CCI_MEM_CFG_HDSPRF);
	/* Set bigcore RM to 2 */
	writel(0x00010001, BIGCORE_GRF_BASE + BIGCORE_GRF_CPU_CON1);
	writel(0x001c0008, BIGCORE_GRF_BASE + BIGCORE_GRF_CPU_MEM_CFG_HDSPRF);
	writel(0x001c0008, BIGCORE_GRF_BASE + BIGCORE_GRF_CPU_MEM_CFG_HSSPRF_LOW);

	/*
	 * Assert reset the combphy0_psu, combphy1_psu, and combphy2_ps,
	 * and need to de-assert reset in combophy driver.
	 */
	writel(0x01300130, PHPPHY_CRU_BASE + PHPPHY_CRU_SOFTRST_CON02);

	/*
	 * Assert SIDDQ for USB 2.0 PHY1 to power down
	 * PHY1 analog block to save power. And let the
	 * PHY0 for DRD0 interface still in normal mode.
	 */
	writel(0x20002000, USB2PHY1_GRF_BASE + USB2PHY_GRF_CON4);

	/*
	 * Enable USB to DEBUG
	 * 1. Set linestate timeout 8ms.
	 * 2. Set linestate fiter time 500us.
	 * 3. Set Rx timeout counter for RX pulldown 2s.
	 * 4. Set handshake counter number for SE0 and SE1 sequence at least 5.
	 */
	writel(0xff, USB2PHY0_GRF_BASE + USB2PHY_GRF_LS_TIMEOUT);
	writel(0x10, USB2PHY0_GRF_BASE + USB2PHY_GRF_LS_DEB);
	writel(0xffff, USB2PHY0_GRF_BASE + USB2PHY_GRF_RX_TIMEOUT);
	writel(0x05, USB2PHY0_GRF_BASE + USB2PHY_GRF_SEQ_LIMT);
	writel(0x00010001, USB2PHY0_GRF_BASE + USB2PHY_GRF_DBG_CON);

	/* Set UART1 uartx_dma_rx_single_bypass 1 */
	writel(0x20002000, PMU1_GRF_BASE + PMU1_GRF_SOC_CON0);
	/* Set UART0-UART11 uartx_dma_rx_single_bypass 1 */
	writel(0x0fff0ffd, SYS_GRF_BASE + SYS_GRF_SOC_CON03);

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX)
	board_set_iomux(UCLASS_MMC, 0, 0);
#elif defined(CONFIG_ROCKCHIP_SFC_IOMUX)
	/*
	 * (UCLASS_MTD, 0, 0) FSPI0
	 * (UCLASS_MTD, 1, 0) FSPI1 M0
	 */
	board_set_iomux(UCLASS_MTD, 0, 0);
#endif

#if defined(CONFIG_ROCKCHIP_SDMMC_IOMUX)
	/* Set the sdmmc iomux and power cycle */
	board_set_iomux(UCLASS_MMC, 1, 0);
#endif
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

