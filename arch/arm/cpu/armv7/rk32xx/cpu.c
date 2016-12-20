/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_RK_EFUSE
extern int32 FtEfuseRead(void *base, void *buff, uint32 addr, uint32 size);
#endif


#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif


/*
 * rk3066 chip info:		{0x33303041, 0x32303131, 0x31313131, 0x56313031} - 300A20111111V101
 * rk3168 chip info:		{0x33303042, 0x32303132, 0x31303031, 0x56313030} - 300B20121011V100
 * rk3036 chip info:		{0x33303141, 0x32303134, 0x30343231, 0x56313031} - 301A20140421V101
 * rk3188 chip info:		{0x33313042, 0x32303132, 0x31313330, 0x56313030} - 310B20121130V100
 * rk3188_plus chip info:	{0x33313042, 0x32303133, 0x30313331, 0x56313031} - 310B20130131V101
 * rk312x chip info:		{0x33313043, 0x32303134, 0x30343239, 0x56313030} - 310C20140429V100
 * rk312xb chip info:		{0x33313044, 0x32303134, 0x30373330, 0x56313030} - 310D20140730V100
 * rk3288 chip info:		{0x33323041, 0x32303133, 0x31313136, 0x56313030} - 320A20131116V100
 * rk322x chip info:		{0x33323042, 0x32303135, 0x31313136, 0x56313030} - 320B20151116V100
 */
int rk_get_bootrom_chip_version(unsigned int chip_info[])
{
	if (chip_info == NULL)
		return -1;

#if defined(CONFIG_NORMAL_WORLD)
	/* bootrom is secure, normal world can't read */
#if defined(CONFIG_RKCHIP_RK322X)
	chip_info[0] = 0x33323042;
#elif defined(CONFIG_RKCHIP_RK3288)
	chip_info[0] = 0x33323041;
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
	if (chip_class == 0x3330) { /* RK30 */
		if (chip_info[0] == 0x33303041) /* 300A */
			return CONFIG_RK3066;
		if (chip_info[0] == 0x33303042) /* 300B */
			return CONFIG_RK3168;
		if (chip_info[0] == 0x33303141) /* 301A */
			return CONFIG_RK3036;
	} else if (chip_class == 0x3331) { /* RK31 */
		if (chip_info[0] == 0x33313042) { /* 310B */
			if (chip_info[3] == 0x56313030)
				return CONFIG_RK3188;
			if (chip_info[3] == 0x56313031)
				return CONFIG_RK3188_PLUS;
		}

		if (chip_info[0] == 0x33313043) { /* 310C */
#if defined(CONFIG_RKCHIP_RK3126)
			return CONFIG_RK3126;
#else
			return CONFIG_RK3128;
#endif
		}

#if defined(CONFIG_RKCHIP_RK3126)
		if (chip_info[0] == 0x33313044) /* 310D */
			return CONFIG_RK3126;
#endif
	} else if (chip_class == 0x3332) { /* 32 */
		if (chip_info[0] == 0x33323041) /* 320A */
			return CONFIG_RK3288;
		if (chip_info[0] == 0x33323042) /* 320B */
			return CONFIG_RK322X;
	}

	return RKCHIP_UNKNOWN;
}

/* get cpu eco version */
static uint8 rk_get_cpu_eco_version(void)
{
	uint8 cpu_version;

	cpu_version = 0;
#ifdef CONFIG_RK_EFUSE
#if defined(CONFIG_RKCHIP_RK322X)
	FtEfuseRead((void *)(unsigned long)RKIO_EFUSE_256BITS_PHYS, &cpu_version, 6, 1);
	cpu_version = (cpu_version >> 4) & 0x3;
#endif
#endif /* CONFIG_RK_EFUSE */

	return cpu_version;
}

uint8 rk_get_cpu_version(void)
{
	return (uint8)gd->arch.cpuversion;
}

/* cpu axi qos priority */
#define CPU_AXI_QOS_PRIORITY    0x08
#define CPU_AXI_QOS_PRIORITY_LEVEL(h, l) \
	((((h) & 3) << 8) | (((h) & 3) << 2) | ((l) & 3))

#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	gd->arch.chiptype = rk_get_chiptype();

#if defined(CONFIG_RKCHIP_RK3288)
	/* read latency configure */
	writel(0x34, 0xffac0000 + 0x14);
	writel(0x34, 0xffac0080 + 0x14);

	/* set vop qos to highest priority */
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(2, 2), 0xffad0408);
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(2, 2), 0xffad0008);

	/* use rk pwm */
	grf_writel(0x00010001, 0x024c);
#endif

#if defined(CONFIG_RKCHIP_RK3126)
	/* read latency configure */
	writel(0x3f, 0x10128000 + 0x14);

	/* set lcdc cpu axi qos priority level */
	#define	CPU_AXI_QOS_PRIORITY_BASE	0x1012f180
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(3, 3), CPU_AXI_QOS_PRIORITY_BASE +
	       CPU_AXI_QOS_PRIORITY);

	/* set GPIO1_C1 iomux to gpio, default sdcard_detn */
	grf_writel(0x00040000, 0x0c0);
#endif

#if defined(CONFIG_RKCHIP_RK3128)
	/* read latency configure */
	writel(0x3f, 0x10128000 + 0x14);

	/* set lcdc cpu axi qos priority level */
	#define	CPU_AXI_QOS_PRIORITY_BASE	0x1012f180
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(3, 3), CPU_AXI_QOS_PRIORITY_BASE + CPU_AXI_QOS_PRIORITY);
#endif

#if defined(CONFIG_RKCHIP_RK3036)
	/* read latency configure */
	writel(0x80, 0x10128000 + 0x14);

	/* set lcdc cpu axi qos priority level */
	#define	CPU_AXI_QOS_PRIORITY_BASE	0x1012f000
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(3, 3), CPU_AXI_QOS_PRIORITY_BASE + CPU_AXI_QOS_PRIORITY);
#endif

#if defined(CONFIG_RKCHIP_RK322X)
	/* use rk pwm */
	grf_writel((1 << 16) | (1 << 0), GRF_SOC_CON2);

	/* pwm select PWMx_1 */
	grf_writel((0xf << 16) | (0xf << 0), GRF_COM_IOMUX);
	/* uart select uartx_1 */
	grf_writel(((1 << 27) | (1 << 24)) | ((1 << 11) | (1 << 8)), GRF_COM_IOMUX);

	/* hdmi phy clock source select HDMIPHY clock out */
	cru_writel((1 << 29) | (0 << 13), CRU_MISC_CON);

#if !defined(CONFIG_NORMAL_WORLD)
	/* emmc sdmmc sdio set secure mode */
	writel((3 << (1 + 16)) | (0 << 1), RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON2);
	/* otg set secure mode */
	writel((1 << (7 + 16)) | (0 << 7), RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON2);
#endif
#endif /* CONFIG_RKCHIP_RK322X */

	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	if (gd->arch.chiptype == RKCHIP_UNKNOWN)
		rk_get_chiptype();

#if defined(CONFIG_RKCHIP_RK3036)
	if (gd->arch.chiptype == CONFIG_RK3036)
		printf("CPU: rk3036\n");
#endif

#if defined(CONFIG_RKCHIP_RK3066)
	if (gd->arch.chiptype == CONFIG_RK3066)
		printf("CPU: rk3066\n");
#endif

#if defined(CONFIG_RKCHIP_RK3126)
	if (gd->arch.chiptype == CONFIG_RK3126) {
		if (grf_readl(GRF_CHIP_TAG) == 0x3136)
			printf("CPU: rk3126b\n");
		else
			printf("CPU: rk3126\n");
	}
#endif

#if defined(CONFIG_RKCHIP_RK3128)
	if (gd->arch.chiptype == CONFIG_RK3128)
		printf("CPU: rk3128\n");
#endif

#if defined(CONFIG_RKCHIP_RK3168)
	if (gd->arch.chiptype == CONFIG_RK3168)
		printf("CPU: rk3168\n");
#endif

#if defined(CONFIG_RKCHIP_RK3188)
	if (gd->arch.chiptype == CONFIG_RK3188)
		printf("CPU: rk3188\n");
	else if (gd->arch.chiptype == CONFIG_RK3188_PLUS)
		printf("CPU: rk3188 plus\n");
#endif

#if defined(CONFIG_RKCHIP_RK3288)
	if (gd->arch.chiptype == CONFIG_RK3288)
		printf("CPU: rk3288\n");
#endif

#if defined(CONFIG_RKCHIP_RK322X)
	if (gd->arch.chiptype == CONFIG_RK322X)
		printf("CPU: rk322x\n");
#endif

	/* get cpu eco version */
	gd->arch.cpuversion = rk_get_cpu_eco_version();
	printf("cpu version = %ld\n", gd->arch.cpuversion);

	rkclk_get_pll();
	rkclk_dump_pll();

	return 0;
}
#endif
