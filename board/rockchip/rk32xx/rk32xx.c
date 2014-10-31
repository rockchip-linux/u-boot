/*
 * (C) Copyright 2008-2014 Rockchip Electronics
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
#include <version.h>
#include <errno.h>
#include <fastboot.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <power/pmic.h>

#include <asm/io.h>
#include <asm/arch/rkplat.h>

#include "../common/config.h"

DECLARE_GLOBAL_DATA_PTR;
static ulong get_sp(void)
{
	ulong ret;

	asm("mov %0, sp" : "=r"(ret) : );
	return ret;
}

void board_lmb_reserve(struct lmb *lmb) {
	ulong sp;
	sp = get_sp();
	debug("## Current stack ends at 0x%08lx ", sp);

	/* adjust sp by 64K to be safe */
	sp -= 64<<10;
	lmb_reserve(lmb, sp,
			gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size - sp);

	//reserve 48M for kernel & 8M for nand api.
	lmb_reserve(lmb, gd->bd->bi_dram[0].start, CONFIG_LMB_RESERVE_SIZE);
}

int board_storage_init(void)
{
	int ret = 0;

	if (StorageInit() == 0) {
		printf("storage init OK!\n");
		ret = 0;
	} else {
		printf("storage init fail!\n");
		ret = -1;
	}

	/* audi and audi-s uart2 and sdcard iomux */
#if defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
#ifdef RK_SDCARD_BOOT_EN
	if ((SdmmcGetSDCardBootMode() == 0) && (CONFIG_UART_NUM == UART_CH2)) {
		rk_iomux_config(RK_UART2_IOMUX);
	}
#endif /* RK_SDCARD_BOOT_EN */
#endif

	return ret;
}


/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	/* Set Initial global variables */

	gd->bd->bi_arch_number = MACH_TYPE_RK30XX;
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x88000;

	return 0;
}


#ifdef CONFIG_DISPLAY_BOARDINFO
/**
 * Print board information
 */
int checkboard(void)
{
	puts("Board:\tRockchip platform Board\n");
	return 0;
}
#endif


#ifdef CONFIG_ARCH_EARLY_INIT_R
int arch_early_init_r(void)
{
	debug("arch_early_init_r\n");

	 /* set up exceptions */
	interrupt_init();
	/* enable exceptions */
	enable_interrupts();

	/* rk pl330 dmac init */
#ifdef CONFIG_RK_DMAC
#ifdef CONFIG_RK_DMAC_0
	rk_pl330_dmac_init(0);
#endif
#ifdef CONFIG_RK_DMAC_1
	rk_pl330_dmac_init(1);
#endif
#endif /* CONFIG_RK_DMAC*/

	return 0;
}
#endif


#ifdef CONFIG_BOARD_LATE_INIT
extern char bootloader_ver[24];
int board_late_init(void)
{
	debug("board_late_init\n");
#if (CONFIG_BOOTDELAY > 0)
	setenv("bootdelay", simple_itoa(CONFIG_BOOTDELAY));
#endif
	setenv("bootcmd", CONFIG_BOOTCOMMAND);

	load_disk_partitions();

#ifdef CONFIG_RK_PWM_REMOTE
        RemotectlInit();
#endif
	rkimage_prepare_fdt();
	key_init();
#ifdef CONFIG_POWER_RK
	pmic_init(0);
	fg_init(0); /*fuel gauge init*/
#endif

	//TODO:set those buffers in a better way, and use malloc?
	rkidb_setup_space(gd->arch.rk_global_buf_addr);

	/* after setup space, get id block data first */
	rkidb_get_idblk_data();

	/* Secure boot check after idb data get */
	SecureBootCheck();

	if (rkidb_get_bootloader_ver() == 0) {
		printf("\n#Boot ver: %s\n", bootloader_ver);
	}

	char tmp_buf[32];
	/* rk sn size 30bytes, zero buff */
	memset(tmp_buf, 0, 32);
	if (rkidb_get_sn(tmp_buf)) {
		setenv("fbt_sn#", tmp_buf);
	}

	board_fbt_preboot();

	return 0;
}
#endif

