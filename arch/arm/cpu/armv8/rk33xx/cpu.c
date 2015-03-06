/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;


void enable_caches(void)
{
#ifndef CONFIG_SYS_ICACHE_OFF
	icache_enable();
#endif
#ifndef CONFIG_SYS_DCACHE_OFF
	dcache_enable();
#endif
}


/*
 * rk3368 chip info:		{0x33333041, 0x32303134, 0x30393238, 0x56313030} - 330A20140928V100
 */
int rk_get_chiptype(void)
{
#ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
	/* second level bootrom is secure */
#if defined(CONFIG_RKCHIP_RK3368)
	return CONFIG_RK3368;
#endif
#else
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
#endif /* CONFIG_SECOND_LEVEL_BOOTLOADER */
}


#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
static secure_timer_init(void)
{
#define STIMER_LOADE_COUNT0		0x00
#define STIMER_LOADE_COUNT1		0x04
#define STIMER_CURRENT_VALUE0		0x08
#define STIMER_CURRENT_VALUE1		0x0C
#define STIMER_CONTROL_REG		0x10

	writel(0xffffffff, RKIO_SECURE_TIMER_2CH_PHYS + 0x20 + STIMER_LOADE_COUNT0);
	writel(0xffffffff, RKIO_SECURE_TIMER_2CH_PHYS + 0x20 + STIMER_LOADE_COUNT1);
	/* auto reload & enable the timer */
	writel(0x01, RKIO_SECURE_TIMER_2CH_PHYS + 0x20  + STIMER_CONTROL_REG);
}
#endif


/* cpu axi qos priority */
#define CPU_AXI_QOS_PRIORITY    0x08
#define CPU_AXI_QOS_PRIORITY_LEVEL(h, l) \
	((((h) & 3) << 8) | (((h) & 3) << 2) | ((l) & 3))


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	rkclk_set_pll();
	gd->arch.chiptype = rk_get_chiptype();

#if defined(CONFIG_FPGA_BOARD) || !defined(CONFIG_SECOND_LEVEL_BOOTLOADER)
	/* ddr space set no secure mode */
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON8);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON9);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON10);
#endif

#if defined(CONFIG_RKCHIP_RK3368)
	/* pwm select rk solution */
	grf_writel((0x01 << 12) | (0x01 << (12 + 16)), GRF_SOC_CON15);

	/* ddr read latency configure */
	writel(0x34, 0xffac0000 + 0x14);

	/* set lcdc cpu axi qos priority level */
	#define	CPU_AXI_QOS_PRIORITY_BASE	0xffad0300
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(2, 2), CPU_AXI_QOS_PRIORITY_BASE + CPU_AXI_QOS_PRIORITY);
#endif

#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
	secure_timer_init();
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

