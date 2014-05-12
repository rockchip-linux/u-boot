/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
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
#include <version.h>
#include <fastboot.h>
#include <asm/io.h>
#include <power/pmic.h>
#include <asm/arch/rkplat.h>

#include "config.h"

DECLARE_GLOBAL_DATA_PTR;


extern char bootloader_ver[];


#ifdef CONFIG_DISPLAY_BOARDINFO
/**
 * Print board information
 */
int checkboard(void)
{
	puts("Board:\t\tRK32xx platform Board\n");
	return 0;
}
#endif


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


#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	stdio_print_current_devices();

	printf("board_late_init\n");
#ifdef CONFIG_RK_I2C 
	rk_i2c_init();
#endif
	key_init();
#ifdef CONFIG_POWER_RK808
	charger_init(0);
#elif CONFIG_POWER_ACT8846
	pmic_init(0);
#endif
	SecureBootCheck();
	get_bootloader_ver(NULL);
	//printf("#Boot ver: %s\n", bootloader_ver);

	//TODO:set those buffers in a better way, and use malloc?
	setup_space(gd->arch.rk_extra_buf_addr);

	char tmp_buf[30];
	if (getSn(tmp_buf)) {
		tmp_buf[sizeof(tmp_buf)-1] = 0;
		setenv("fbt_sn#", tmp_buf);
	}
#ifdef CONFIG_CMD_FASTBOOT
	fbt_preboot();
#endif
	return 0;
}
#endif

