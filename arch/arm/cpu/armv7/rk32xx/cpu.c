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


#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif


uint32 CacheFlushDRegion(uint32 adr, uint32 size)
{
#ifndef CONFIG_SYS_DCACHE_OFF
	flush_cache(adr, size);
#endif
}


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	rkclk_get_pll();

	printf("CPU:\t\tRK32xx\n");
	printf("arm pll:\t%d\n", rkclk_get_arm_pll());
	printf("general pll:\t%d\n", rkclk_get_general_pll());
	printf("codec pll:\t%d\n", rkclk_get_codec_pll());
	printf("ddr pll:\t%d\n", rkclk_get_ddr_pll());
	printf("new pll:\t%d\n", rkclk_get_new_pll());
	return 0;
}
#endif

