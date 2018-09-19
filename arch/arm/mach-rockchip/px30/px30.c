/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <asm/io.h>
#include <asm/arch/cru_px30.h>
#include <asm/arch/grf_px30.h>
#include <asm/arch/hardware.h>
#include <asm/arch/uart.h>
#include <asm/armv8/mmu.h>
#include <asm/arch/clock.h>
#include <asm/arch/cru_px30.h>
#include <dt-bindings/clock/px30-cru.h>

#define PMU_PWRDN_CON			0xff000018

#define SERVICE_CORE_ADDR		0xff508000
#define QOS_PRIORITY			0x08

#define QOS_PRIORITY_LEVEL(h, l)	((((h) & 3) << 8) | ((l) & 3))

static struct mm_region px30_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0xff000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0xff000000UL,
		.phys = 0xff000000UL,
		.size = 0x01000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = px30_mem_map;

int arch_cpu_init(void)
{
#ifdef CONFIG_SPL_BUILD
	/* We do some SoC one time setting here. */
	/* Disable the ddr secure region setting to make it non-secure */
#endif
	/* Enable PD_VO (default disable at reset) */
	rk_clrreg(PMU_PWRDN_CON, 1 << 13);

#ifdef CONFIG_TPL_BUILD
	/* Set cpu qos priority */
	writel(QOS_PRIORITY_LEVEL(1, 1), SERVICE_CORE_ADDR + QOS_PRIORITY);
#endif

	return 0;
}

#define GRF_BASE		0xff140000
#define UART2_BASE		0xff160000
#define CRU_BASE		0xff2b0000
void board_debug_uart_init(void)
{
	static struct px30_grf * const grf = (void *)GRF_BASE;

	/* GRF_IOFUNC_CON0 */
	enum {
		CON_IOMUX_UART2SEL_SHIFT	= 10,
		CON_IOMUX_UART2SEL_MASK = 3 << CON_IOMUX_UART2SEL_SHIFT,
		CON_IOMUX_UART2SEL_M0	= 0,
		CON_IOMUX_UART2SEL_M1,
		CON_IOMUX_UART2SEL_USBPHY,
	};

#ifdef CONFIG_TPL_BUILD
	static struct px30_cru * const cru = (void *)CRU_BASE;
	static struct rk_uart * const uart = (void *)UART2_BASE;

	/* GRF_GPIO2BH_IOMUX */
	enum {
		GPIO2B7_SHIFT		= 12,
		GPIO2B7_MASK		= 0xf << GPIO2B7_SHIFT,
		GPIO2B7_GPIO		= 0,
		GPIO2B7_CIF_D10M0,
		GPIO2B7_I2C2_SCL,

		GPIO2B6_SHIFT		= 8,
		GPIO2B6_MASK		= 0xf << GPIO2B6_SHIFT,
		GPIO2B6_GPIO		= 0,
		GPIO2B6_CIF_D1M0,
		GPIO2B6_UART2_RXM1,

		GPIO2B5_SHIFT		= 4,
		GPIO2B5_MASK		= 0xf << GPIO2B5_SHIFT,
		GPIO2B5_GPIO		= 0,
		GPIO2B5_PWM2,

		GPIO2B4_SHIFT		= 0,
		GPIO2B4_MASK		= 0xf << GPIO2B4_SHIFT,
		GPIO2B4_GPIO		= 0,
		GPIO2B4_CIF_D0M0,
		GPIO2B4_UART2_TXM1,
	};

	/* uart_sel_clk default select 24MHz */
	rk_clrsetreg(&cru->clksel_con[37],
		     UART2_PLL_SEL_MASK | UART2_DIV_CON_MASK,
		     UART2_PLL_SEL_24M << UART2_PLL_SEL_SHIFT | 0);
	rk_clrsetreg(&cru->clksel_con[38],
		     UART2_CLK_SEL_MASK,
		     UART2_CLK_SEL_UART2 << UART2_CLK_SEL_SHIFT);

	/* Enable early UART2 */
	rk_clrsetreg(&grf->iofunc_con0,
		     CON_IOMUX_UART2SEL_MASK,
		     CON_IOMUX_UART2SEL_M1 << CON_IOMUX_UART2SEL_SHIFT);

	/*
	 * Set iomux to UART2_M0 and UART2_M1.
	 * Because uart2_rxm0 and uart2_txm0 are default reset value,
	 * so only need set uart2_rxm1 and uart2_txm1 here.
	 */
	rk_clrsetreg(&grf->gpio2bh_iomux,
		     GPIO2B6_MASK,
		     GPIO2B6_UART2_RXM1 << GPIO2B6_SHIFT);
	rk_clrsetreg(&grf->gpio2bh_iomux,
		     GPIO2B4_MASK,
		     GPIO2B4_UART2_TXM1 << GPIO2B4_SHIFT);

	/* enable FIFO */
	writel(0x1, &uart->sfe);
#else
#ifdef CONFIG_SPL_BUILD
	/* GRF_GPIO1DL_IOMUX */
	enum {
		GPIO1D3_SHIFT		= 12,
		GPIO1D3_MASK		= 0xf << GPIO1D3_SHIFT,
		GPIO1D3_GPIO		= 0,
		GPIO1D3_SDMMC_D1,
		GPIO1D3_UART2_RXM0,

		GPIO1D2_SHIFT		= 8,
		GPIO1D2_MASK		= 0xf << GPIO1D2_SHIFT,
		GPIO1D2_GPIO		= 0,
		GPIO1D2_SDMMC_D0,
		GPIO1D2_UART2_TXM0,

		GPIO1D1_SHIFT		= 4,
		GPIO1D1_MASK		= 0xf << GPIO1D1_SHIFT,
		GPIO1D1_GPIO		= 0,
		GPIO1D1_SDIO_D3,

		GPIO1D0_SHIFT		= 0,
		GPIO1D0_MASK		= 0xf << GPIO1D0_SHIFT,
		GPIO1D0_GPIO		= 0,
		GPIO1D0_SDIO_D2,
	};

	/* Do not set the iomux in U-Boot proper because SD card may using it */
	/* Enable early UART2 channel m0 on the px30 */
	rk_clrsetreg(&grf->gpio1dl_iomux,
		     GPIO1D3_MASK | GPIO1D2_MASK,
		     GPIO1D3_UART2_RXM0 << GPIO1D3_SHIFT |
		     GPIO1D2_UART2_TXM0 << GPIO1D2_SHIFT);
#endif
	/* Set channel C as UART2 input */
	rk_clrsetreg(&grf->iofunc_con0,
		     CON_IOMUX_UART2SEL_MASK,
		     CON_IOMUX_UART2SEL_M0 << CON_IOMUX_UART2SEL_SHIFT);
#endif
}

int set_armclk_rate(void)
{
	struct px30_clk_priv *priv;
	struct clk clk;
	int ret;

	ret = rockchip_get_clk(&clk.dev);
	if (ret) {
		printf("Failed to get clk dev\n");
		return ret;
	}
	clk.id = ARMCLK;
	priv = dev_get_priv(clk.dev);
	ret = clk_set_rate(&clk, priv->armclk_hz);
	if (ret < 0) {
		printf("Failed to set armclk %lu\n", priv->armclk_hz);
		return ret;
	}

	return 0;
}
