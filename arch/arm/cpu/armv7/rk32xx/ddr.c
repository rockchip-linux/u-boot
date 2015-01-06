/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * hcy, Software Engineering.
 *
 * Configuation settings for the rk3xxx chip platform.
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
#include <asm/byteorder.h>

DECLARE_GLOBAL_DATA_PTR;


/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
	gd->ram_size = get_ram_size(
			(void *)CONFIG_SYS_SDRAM_BASE,
			CONFIG_SYS_SDRAM_SIZE);

#if defined(CONFIG_RKDDR_PARAM_ADDR)
	u64* buf = (u64*)CONFIG_RKDDR_PARAM_ADDR;
	u32 count = ((u32*)buf)[0];
	u64 start = 0, end = 0, size = 0;

	debug("\n");

	buf ++;
	if (count >= CONFIG_RK_MAX_DRAM_BANKS) {
		end = PHYS_SDRAM;
	} else {
		int i;
		for (i = 0; i < count; i++) {
			start = le64_to_cpu(buf[i]);
			size = le64_to_cpu(buf[count + i]);
			if (start < CONFIG_MAX_MEM_ADDR) {
				end = start + size;
				if (end > CONFIG_MAX_MEM_ADDR) {
					end = CONFIG_MAX_MEM_ADDR;
					break;
				}
			}
		}
	}

	gd->arch.ddr_end = (unsigned long)end;
	debug("DDR End Address: 0x%08lx\n", gd->arch.ddr_end);
#endif
	return 0;
}

void dram_init_banksize(void)
{
#if defined(CONFIG_RKDDR_PARAM_ADDR)
	u64* buf = (u64*)CONFIG_RKDDR_PARAM_ADDR;
	u32 count = ((u32*)buf)[0];
	buf ++;
	printf("Found dram banks:%d\n", count);
	gd->bd->rk_dram[0].start = gd->bd->rk_dram[0].size = 0;
	if (count >= CONFIG_RK_MAX_DRAM_BANKS){
		printf("Wrong bank count: %d(%d)\n",
				count, CONFIG_RK_MAX_DRAM_BANKS);
		goto failed;
	}
	int i;
	for (i = 0; i < count; i++) {
		gd->bd->rk_dram[i].start = le64_to_cpu(buf[i]);
		gd->bd->rk_dram[i].size = le64_to_cpu(buf[count + i]);
		//TODO: add check, if start|size not valide, goto failed.
		/*
		if (check) {
			gd->bd->rk_dram[0].start = gd->bd->rk_dram[0].size = 0;
			goto failed;
		}*/
		printf("Adding bank:%016llx(%016llx)\n",
				gd->bd->rk_dram[i].start,
				gd->bd->rk_dram[i].size);
		gd->bd->rk_dram[i+1].start = gd->bd->rk_dram[i+1].size = 0;
	}
failed:
#endif

	gd->bd->bi_dram[0].start = PHYS_SDRAM;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_SIZE;
}

