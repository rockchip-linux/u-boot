/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
 * rk3066 chip info:		{0x33303041, 0x32303131, 0x31313131, 0x56313031} - 300A20111111V101
 * rk3168 chip info:		{0x33303042, 0x32303132, 0x31303031, 0x56313030} - 300B20121011V100
 * rk3036 chip info:		{0x33303141, 0x32303134, 0x30343231, 0x56313031} - 301A20140421V101
 * rk3188 chip info:		{0x33313042, 0x32303132, 0x31313330, 0x56313030} - 310B20121130V100
 * rk3188_plus chip info:	{0x33313042, 0x32303133, 0x30313331, 0x56313031} - 310B20130131V101
 * rk312x chip info:		{0x33313043, 0x32303134, 0x30343239, 0x56313030} - 310C20140429V100
 * rk312xb chip info:		{0x33313044, 0x32303134, 0x30373330, 0x56313030} - 310D20140730V100
 * rk3288 chip info:		{0x33323041, 0x32303133, 0x31313136, 0x56313030} - 320A20131116V100
 */
int rk_get_chiptype(void)
{
	unsigned int chip_info[4];
	unsigned int chip_class;

	memset(chip_info, 0, sizeof(chip_info));
	memcpy((char *)chip_info, (char *)RKIO_ROM_CHIP_VER_ADDR, RKIO_ROM_CHIP_VER_SIZE);

	chip_class = (chip_info[0] & 0xFFFF0000) >> 16;
	if (chip_class == 0x3330) { // 30
		if (chip_info[0] == 0x33303041) { // 300A
			return CONFIG_RK3066;
		}
		if (chip_info[0] == 0x33303042) { // 300B
			return CONFIG_RK3168;
		}
		if (chip_info[0] == 0x33303141) { // 301A
			return CONFIG_RK3036;
		}
	} else if (chip_class == 0x3331) { // 31
		if (chip_info[0] == 0x33313042) { // 310B
			if ((chip_info[1] == 0x32303132) && (chip_info[2] == 0x31313330) && (chip_info[3] == 0x56313030)) {
				return CONFIG_RK3188;
			}
			if ((chip_info[1] == 0x32303133) && (chip_info[2] == 0x30313331) && (chip_info[3] == 0x56313031)) {
				return CONFIG_RK3188_PLUS;
			}
		}

		if (chip_info[0] == 0x33313043) { // 310C
#if defined(CONFIG_RKCHIP_RK3126)
			return CONFIG_RK3126;
#else
			return CONFIG_RK3128;
#endif
		}

		if (chip_info[0] == 0x33313044) { // 310D
#if defined(CONFIG_RKCHIP_RK3126)
			return CONFIG_RK3126;
#endif
		}
	} else if (chip_class == 0x3332) { // 32
		if (chip_info[0] == 0x33323041) { // 320A
			return CONFIG_RK3288;
		}
	}

	return RKCHIP_UNKNOWN;
}


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	gd->arch.chiptype = rk_get_chiptype();

	/* read latency configure */
#if defined(CONFIG_RKCHIP_RK3288)
	writel(0x34, 0xffac0000 + 0x14);
	writel(0x34, 0xffac0080 + 0x14);
#endif

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	writel(0x3f, 0x10128000 + 0x14);
#endif

#if defined(CONFIG_RKCHIP_RK3036)
	writel(0x80, 0x10128000 + 0x14);
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

#if defined(CONFIG_RKCHIP_RK3036)
	if (gd->arch.chiptype == CONFIG_RK3036) {
		printf("CPU: rk3036\n");
	}
#endif

#if defined(CONFIG_RKCHIP_RK3066)
	if (gd->arch.chiptype == CONFIG_RK3066) {
		printf("CPU: rk3066\n");
	}
#endif

#if defined(CONFIG_RKCHIP_RK3126)
	if (gd->arch.chiptype == CONFIG_RK3126) {
		if (grf_readl(GRF_CHIP_TAG) == 0x3136) {
			printf("CPU: rk3126b\n");
		} else {
			printf("CPU: rk3126\n");
		}
	}
#endif

#if defined(CONFIG_RKCHIP_RK3128)
	if (gd->arch.chiptype == CONFIG_RK3128) {
		printf("CPU: rk3128\n");
	}
#endif

#if defined(CONFIG_RKCHIP_RK3168)
	if (gd->arch.chiptype == CONFIG_RK3168) {
		printf("CPU: rk3168\n");
	}
#endif

#if defined(CONFIG_RKCHIP_RK3188)
	if (gd->arch.chiptype == CONFIG_RK3188) {
		printf("CPU: rk3188\n");
	} else if (gd->arch.chiptype == CONFIG_RK3188_PLUS) {
		printf("CPU: rk3188 plus\n");
	}
#endif

#if defined(CONFIG_RKCHIP_RK3288)
	if (gd->arch.chiptype == CONFIG_RK3288) {
		printf("CPU: rk3288\n");
	}
#endif

	rkclk_get_pll();
	rkclk_dump_pll();

	return 0;
}
#endif

