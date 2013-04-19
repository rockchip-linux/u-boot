/*
 * (C) Copyright 2013-2013
 * Peter <superpeter.cai@gmail.com>
 *
 * Configuation settings for the rk30xx board.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <fastboot.h>
#include "../common/armlinux/config.h"

DECLARE_GLOBAL_DATA_PTR;


/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	/* Set Initial global variables */

	gd->bd->bi_arch_number = MACH_TYPE_RK30XX;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x88000;

	return 0;
}


/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
	gd->ram_size = get_ram_size(
			(void *)CONFIG_SYS_SDRAM_BASE,
			CONFIG_SYS_SDRAM_SIZE);

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
/**
 * Print board information
 */
int checkboard(void)
{
	puts("Board:\tRK30xx platform Board\n");
	return 0;
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

int board_fbt_key_pressed(void)
{
    int boot_loader = 0, boot_recovery = 0; 
    enum fbt_reboot_type frt = FASTBOOT_REBOOT_NONE;
    int recovery_key = checkKey(&boot_loader, &boot_recovery);

    if (recovery_key) {
        printf("\n%s: recovery_key=%d.\n",
                __func__, recovery_key);
    }

    if(boot_recovery) {
        printf("\n%s: recovery key pressed.\n",
                __func__);
        frt = FASTBOOT_REBOOT_RECOVERY;
    } else if (boot_loader) {
        printf("\n%s: loader key pressed.\n",
                __func__);
		#if 0
		g_BootRockusb = 1;
		FW_SDRAM_ExcuteAddr = 0;
		g_BootRockusb = 0;
		UsbBoot();
		RkPrintf("UsbHook,%d\n" , RkldTimerGetTick());
		UsbHook();
		#endif
        frt = FASTBOOT_REBOOT_BOOTLOADER;
    }

    return frt;
}

struct fbt_partition fbt_partitions[FBT_PARTITION_MAX_NUM];
//TODO:use empty table, then fill with parameter.

void board_fbt_finalize_bootargs(char* args, size_t buf_sz) {
//TODO:use parameter's cmdline? or setup serial_no/device_id/mac here?
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	BootInfo gBootInfo;
	char *dst = malloc(1025);
	memset(dst, 0, 1025);
	printf("board_late_init\n");
    recoveryKeyInit(&key_recover);
	if (!GetParam(0, dst)) {
	    ParseParam( &gBootInfo, ((PLoaderParam)dst)->parameter, ((PLoaderParam)dst)->length );
        //TODO:setup partitions base on ParseParam result.
    }
#if 0
	char tmp_buf[17];
	u64 id_64;

	dieid_num_r();

	generate_default_64bit_id(serial_no_salt, &id_64);
	snprintf(tmp_buf, sizeof(tmp_buf), "%016llx", id_64);
	tmp_buf[sizeof(tmp_buf)-1] = 0;
	setenv("fbt_id#", tmp_buf);

#endif

    //TODO:generate serial no
	fbt_preboot();
	return 0;
}
#endif




