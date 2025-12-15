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

#define GPIO1_IOC_BASE		0xFD1D0000
#define GPIO1A_IOMUX_SEL_1	0x24
#define GPIO1B_IOMUX_SEL_0	0x28
#define GPIO1B_IOMUX_SEL_1	0x2c
#define GPIO1C_IOMUX_SEL_0	0x30


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
#endif

	return 0;
}
#endif

