/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
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
 * rk3366 chip info:		{0x33333042, 0x32303135, 0x30363234, 0x56313030} - 330B20150624V100
 * rk3399 chip info:		{0x33333043, 0x32303136, 0x30313138, 0x56313030} - 330B20160118V100
 */
int rk_get_bootrom_chip_version(unsigned int chip_info[])
{
	if (chip_info == NULL)
		return -1;

#if defined(CONFIG_NORMAL_WORLD)
	/* bootrom is secure, second level can't read */
#if defined(CONFIG_RKCHIP_RK3368)
	chip_info[0] = 0x33333041;
	chip_info[3] = 0x56313030;
#elif defined(CONFIG_RKCHIP_RK3366)
	chip_info[0] = 0x33333042;
	chip_info[3] = 0x56313030;
#elif defined(CONFIG_RKCHIP_RK3399)
	chip_info[0] = 0x33333043;
	chip_info[3] = 0x56313030;
#endif
#else
	memcpy((char *)chip_info, (char *)RKIO_ROM_CHIP_VER_ADDR, RKIO_ROM_CHIP_VER_SIZE);
#endif /* CONFIG_NORMAL_WORLD */

	return 0;
}

int rk_get_chiptype(void)
{
	unsigned int chip_info[4];
	unsigned int chip_class;

	memset(chip_info, 0, sizeof(chip_info));
	rk_get_bootrom_chip_version(chip_info);

	chip_class = (chip_info[0] & 0xFFFF0000) >> 16;
	if (chip_class == 0x3333) { /* RK33 */
		if (chip_info[0] == 0x33333041) /* 330A */
			return CONFIG_RK3368;
		if (chip_info[0] == 0x33333042) /* 330B */
			return CONFIG_RK3366;
		if (chip_info[0] == 0x33333043) /* 330C */
			return CONFIG_RK3399;
	}

	return RKCHIP_UNKNOWN;
}


#if defined(CONFIG_RKCHIP_RK3368)
	#define RKIO_DDR_LATENCY_BASE		(0xffac0000 + 0x14)
	#define DDR_READ_LATENCY_VALUE		(0x34)

	#define	RKIO_CPU_AXI_QOS_PRIORITY_BASE	0xffad0300
	#define CPU_AXI_QOS_PRIORITY		0x08
	#define QOS_PRIORITY_LEVEL_H		2
	#define QOS_PRIORITY_LEVEL_L		2
#endif


/* secure parameter init */
#if !defined(CONFIG_NORMAL_WORLD)
static inline void secure_parameter_init(void)
{
#if defined(CONFIG_RKCHIP_RK3368) || defined(CONFIG_RKCHIP_RK3366)
	/* ddr space set no secure mode */
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON8);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON9);
	writel(0xffff0000, RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON10);
#elif defined(CONFIG_RKCHIP_RK3399)
	/* ddr space set no secure mode */
	writel(0xffff0000, RKIO_PMU_SGRF_PHYS + SGRF_SOC_CON8);
	writel(0xffff0000, RKIO_PMU_SGRF_PHYS + SGRF_SOC_CON9);
	writel(0xffff0000, RKIO_PMU_SGRF_PHYS + SGRF_SOC_CON10);

	/* emmc master secure setting */
	writel(((3 << 7) << 16) | (0 << 7), RKIO_PMU_SGRF_PHYS + SGRF_SOC_CON7);
#else
	#error "PLS config platform for secure parameter init!"
#endif
}
#endif /* CONFIG_NORMAL_WORLD */


#if !defined(CONFIG_FPGA_BOARD) && defined(CONFIG_RKCHIP_RK3368)
/* ddr read latency configure */
static inline void ddr_read_latency_config(void)
{
	writel(DDR_READ_LATENCY_VALUE, RKIO_DDR_LATENCY_BASE);
}


/* cpu axi qos priority */
#define CPU_AXI_QOS_PRIORITY_LEVEL(h, l) \
	((((h) & 3) << 8) | (((h) & 3) << 2) | ((l) & 3))
static inline void cpu_axi_qos_prority_level_config(void)
{
	uint32_t level;

	/* set lcdc cpu axi qos priority level */
	level = CPU_AXI_QOS_PRIORITY_LEVEL(QOS_PRIORITY_LEVEL_H, QOS_PRIORITY_LEVEL_L);
	writel(level, RKIO_CPU_AXI_QOS_PRIORITY_BASE + CPU_AXI_QOS_PRIORITY);
}
#endif


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	rkclk_set_pll();
	gd->arch.chiptype = rk_get_chiptype();

#if !defined(CONFIG_NORMAL_WORLD)
	secure_parameter_init();
#endif

#if !defined(CONFIG_FPGA_BOARD) && defined(CONFIG_RKCHIP_RK3368)
	ddr_read_latency_config();
	cpu_axi_qos_prority_level_config();
#endif

#if defined(CONFIG_RKCHIP_RK3368)
	/* pwm select rk solution */
	grf_writel((0x01 << 12) | (0x01 << (12 + 16)), GRF_SOC_CON15);

	/* select 32KHz clock source */
	pmugrf_writel((1 << (7 + 16)) | (0 << 7), PMU_GRF_SOC_CON0);

	/* enable force to jtag */
	grf_writel((0x01 << 13) | (0x01 << (13 + 16)), GRF_SOC_CON15);
#endif /* CONFIG_RKCHIP_RK3368 */

#if defined(CONFIG_RKCHIP_RK3399)
	/* emmc core clock multiplier set not support */
	grf_writel((0x00 << 0) | (0xFF << (0 + 16)), GRF_EMMCCORE_CON(11));

	/* pwm3 select A mode */
	pmugrf_writel((1 << (5 + 16)) | (0 << 5), PMU_GRF_SOC_CON0);
#endif
	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	if (gd->arch.chiptype == RKCHIP_UNKNOWN)
		rk_get_chiptype();

#if defined(CONFIG_RKCHIP_RK3368)
	if (gd->arch.chiptype == CONFIG_RK3368)
		printf("CPU: rk3368\n");
#endif

#if defined(CONFIG_RKCHIP_RK3366)
	if (gd->arch.chiptype == CONFIG_RK3366)
		printf("CPU: rk3366\n");
#endif

#if defined(CONFIG_RKCHIP_RK3399)
	if (gd->arch.chiptype == CONFIG_RK3399)
		printf("CPU: rk3399\n");
#endif

	rkclk_get_pll();
	rkclk_dump_pll();

	return 0;
}
#endif
