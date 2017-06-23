/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/arch/grf_rk3328.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <dwc3-uboot.h>
#include <power/regulator.h>
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	int ret;
#define GRF_BASE	0xff100000
	struct rk3328_grf_regs * const grf = (void *)GRF_BASE;

	/* uart2 select m1, sdcard select m1*/
	rk_clrsetreg(&grf->com_iomux,
		     IOMUX_SEL_UART2_MASK | IOMUX_SEL_SDMMC_MASK,
		     IOMUX_SEL_UART2_M1 << IOMUX_SEL_UART2_SHIFT |
		     IOMUX_SEL_SDMMC_M1 << IOMUX_SEL_SDMMC_SHIFT);

	ret = regulators_enable_boot_on(false);
	if (ret)
		debug("%s: Cannot enable boot on regulator\n", __func__);

	return ret;
}

int dram_init_banksize(void)
{
	size_t max_size = min((unsigned long)gd->ram_size, gd->ram_top);

	/* Reserve 0x200000 for ATF bl31 */
	gd->bd->bi_dram[0].start = 0x200000;
	gd->bd->bi_dram[0].size = max_size - gd->bd->bi_dram[0].start;

	return 0;
}

int usb_gadget_handle_interrupts(void)
{
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	return 0;
}
