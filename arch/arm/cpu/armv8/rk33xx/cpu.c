/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif


/*
 * rk3368 chip info:		{0x33333041, 0x32303134, 0x30393238, 0x56313030} - 330A20140928V100
 */
int rk_get_chiptype(void)
{
	unsigned int chip_info[4];
	unsigned int chip_class;

	memset(chip_info, 0, sizeof(chip_info));
	memcpy((char *)chip_info, (char *)RKIO_ROM_CHIP_VER_ADDR, RKIO_ROM_CHIP_VER_SIZE);

	chip_class = (chip_info[0] & 0xFFFF0000) >> 16;
	if (chip_class == 0x3333) { // 33
		if (chip_info[0] == 0x33333041) { // 330A
			return CONFIG_RK3368;
		}

	}

	return RKCHIP_UNKNOWN;
}


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	rkclk_set_pll();
	gd->arch.chiptype = rk_get_chiptype();

#if !defined(CONFIG_FPGA_BOARD) && !defined(CONFIG_SECOND_LEVEL_BOOTLOADER)
	/* ddr space set no secure mode */
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON8);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON9);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON10);
#endif

	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	if (gd->arch.chiptype == RKCHIP_UNKNOWN) {
		rk_get_chiptype();
	}

#if defined(CONFIG_RKCHIP_RK3368)
	if (gd->arch.chiptype == CONFIG_RK3368) {
		printf("CPU: rk3368\n");
	}
#endif

	rkclk_get_pll();
	rkclk_dump_pll();

	return 0;
}
#endif

