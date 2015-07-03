/*
 * (C) Copyright 2008-2015 Rockchip Electronics
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


#if defined(CONFIG_RKCHIP_RK3368)
	#define RKIO_SECURE_TIMER_BASE		(RKIO_SECURE_TIMER_2CH_PHYS + 0x20)

	#define RKIO_DDR_LATENCY_BASE		(0xffac0000 + 0x14)
	#define DDR_READ_LATENCY_VALUE		(0x34)

	#define	RKIO_CPU_AXI_QOS_PRIORITY_BASE	0xffad0300
	#define CPU_AXI_QOS_PRIORITY    	0x08
	#define QOS_PRIORITY_LEVEL_H		2
	#define QOS_PRIORITY_LEVEL_L		2
#else
	#error "PLS config platform for secure/latency/qos!"
#endif


/* secure parameter init */
#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
static void inline secure_parameter_init(void)
{
	/* secure timer init */
	#define STIMER_LOADE_COUNT0		0x00
	#define STIMER_LOADE_COUNT1		0x04
	#define STIMER_CURRENT_VALUE0		0x08
	#define STIMER_CURRENT_VALUE1		0x0C
	#define STIMER_CONTROL_REG		0x10

	writel(0xffffffff, RKIO_SECURE_TIMER_BASE + STIMER_LOADE_COUNT0);
	writel(0xffffffff, RKIO_SECURE_TIMER_BASE + STIMER_LOADE_COUNT1);
	/* auto reload & enable the timer */
	writel(0x01, RKIO_SECURE_TIMER_BASE  + STIMER_CONTROL_REG);

#if defined(CONFIG_RKCHIP_RK3368)
	/* ddr space set no secure mode */
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON8);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON9);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON10);
#else
	#error "PLS config platform for secure parameter init!"
#endif
}
#endif /* CONFIG_SECOND_LEVEL_BOOTLOADER */


/* ddr read latency configure */
static void inline ddr_read_latency_config(void)
{
	writel(DDR_READ_LATENCY_VALUE, RKIO_DDR_LATENCY_BASE);
}


/* cpu axi qos priority */
#define CPU_AXI_QOS_PRIORITY_LEVEL(h, l) \
	((((h) & 3) << 8) | (((h) & 3) << 2) | ((l) & 3))
static void inline cpu_axi_qos_prority_level_config(void)
{
	uint32_t level;

	/* set lcdc cpu axi qos priority level */
	level = CPU_AXI_QOS_PRIORITY_LEVEL(QOS_PRIORITY_LEVEL_H, QOS_PRIORITY_LEVEL_L);
	writel(level, RKIO_CPU_AXI_QOS_PRIORITY_BASE + CPU_AXI_QOS_PRIORITY);
}


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	rkclk_set_pll();
	gd->arch.chiptype = rk_get_chiptype();

#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
	secure_parameter_init();
#endif
	ddr_read_latency_config();
	cpu_axi_qos_prority_level_config();

#if defined(CONFIG_RKCHIP_RK3368)
	/* pwm select rk solution */
	grf_writel((0x01 << 12) | (0x01 << (12 + 16)), GRF_SOC_CON15);

	/* select 32KHz clock source */
	pmugrf_writel((1 << (7 + 16)) | (0 << 7), PMU_GRF_SOC_CON0);

	/* enable force to jtag */
	grf_writel((0x01 << 13) | (0x01 << (13 + 16)), GRF_SOC_CON15);
#endif /* CONFIG_RKCHIP_RK3368 */

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

