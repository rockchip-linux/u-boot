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
#include "parameter.h"

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
        frt = FASTBOOT_REBOOT_BOOTLOADER;
    }

    return frt;
}

void board_fbt_start(void)
{
#if 0
	/* in case we entered fastboot by request from ADB or other
	 * means that we couldn't detect in board_fbt_key_command(),
	 * make sure the LEDs are set to red to indicate fastboot mode
	 */
	if (avr_detected) {
		avr_led_set_mute(&red);
		avr_led_set_all(&red);
	}
#endif
}

void board_fbt_end(void)
{
#if 0
	if (avr_detected) {
		/* to match spec, put avr into boot animation mode. */
		avr_led_set_mode(AVR_LED_MODE_BOOT_ANIMATION);
	}
#endif
}

/* For the 16GB eMMC part used in Tungsten, the erase group size is 512KB.
 * So every partition should be at least 512KB to make it possible to use
 * the mmc erase operation when doing 'fastboot erase'.
 * However, the xloader is an exception because in order for the OMAP4 ROM
 * bootloader to find it, it must be at offset 0KB, 128KB, 256KB, or 384KB.
 * Since the partition table is at 0KB, we choose 128KB.  Special care
 * must be kept to prevent erase the partition table when/if the xloader
 * partition is being erased.
 */
struct fbt_partition fbt_partitions[] = {
#if 0
	{ "--ptable", NULL,  17},  /* partition table in
					* first 34 sectors */
	{ "environment", "raw", 95 },  /* partition used to u-boot environment,
					* which is also where we store
					* oem lock/unlock state.  size
					* must match CONFIG_ENV_SIZE.
					*/
	{ "crypto", "raw", 16},        /* 16KB partition for crypto keys.
					* used when userdata is encrypted.
					*/
	{ "xloader", "raw", 384 },	/* must start at 128KB offset into eMMC
					 * for ROM bootloader to find it.
					 * pad out to fill whole erase group */
	{ "bootloader", "raw", 512 },  /* u-boot, one erase group in size */
	{ "device_info", "raw", 512 }, /* device specific info like MAC
					* addresses.  read-only once it has
					* been written to.  bootloader parses
					* this at boot and sends the contents
					* to the kernel via cmdline args.
					*/
	{ "bootloader2", "raw", 512 }, /* u-boot, alternate copy */
	{ "misc", "raw", 512 }, 	/* misc partition used by recovery for
					 * storing parameters in the case of a
					 * power failure during recovery
					 * operation.
					 */
	{ "recovery", "boot", 8*1024 },
	{ "boot", "boot", 8*1024 },
	{ "efs", "ext4", 8*1024 },      /* for factory programmed keys,
					 * minimum size for a ext4 fs is
					 * about 8MB
					 */
	{ "system", "ext4", 1024*1024 },
	{ "cache", "ext4", 512*1024 },
	{ "userdata", "ext4", 0},
	{ 0, 0, 0 },
#endif
};
//TODO:use empty table, then fill with parameter.

void board_fbt_finalize_bootargs(char* args, size_t buf_sz) {
#if 0
	int used = strlen(args);
	int i;
	int bgap_threshold_t_hot  = 83000; /* 83 deg C */
	int bgap_threshold_t_cold = 76000; /* 76 deg C */

	for (i = 0; i < ARRAY_SIZE(mac_defaults); ++i) {
		u8 m[6];
		char mac[18];

		if (strstr(args, mac_defaults[i].name))
			continue;

		generate_default_mac_addr(mac_defaults[i].salt, m);
		snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
				m[5], m[4], m[3], m[2], m[1], m[0]);
		mac[sizeof(mac) - 1] = 0;
		used += snprintf(args + used,
				buf_sz - used,
				" %s=%s",
				mac_defaults[i].name,
				mac);
	}

	/* Add board_id */
	used += snprintf(args + used,
			 buf_sz - used,
			 " board_steelhead.steelhead_hw_rev=%d",
			 steelhead_hw_rev);

	/* Add temperature thresholds for throttle control */
	snprintf(args + used,
		 buf_sz - used,
		 " omap_temp_sensor.bgap_threshold_t_hot=%d"
		 " omap_temp_sensor.bgap_threshold_t_cold=%d",
		 bgap_threshold_t_hot, bgap_threshold_t_cold);

	args[buf_sz-1] = 0;

	/* this is called just before booting normal image.  we
	 * use opportunity to start boot animation.
	 */
	board_fbt_end();
#endif
}

#if 0
int board_fbt_handle_flash(disk_partition_t *ptn,
			   struct cmd_fastboot_interface *priv)
{
	return 0;
}
#endif
#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#if 0
	char tmp_buf[17];
	u64 id_64;

	dieid_num_r();

	generate_default_64bit_id(serial_no_salt, &id_64);
	snprintf(tmp_buf, sizeof(tmp_buf), "%016llx", id_64);
	tmp_buf[sizeof(tmp_buf)-1] = 0;
	setenv("fbt_id#", tmp_buf);

#ifdef CONFIG_MFG
	set_default_mac_env_vars();
#endif

#endif
    //TODO:generate serial no, call fbt_preboot
    recoveryKeyInit(&key_recover);
	fbt_preboot();
	return 0;
}
#endif

#define LOADER_FLAG_REG         ((REG32*)(PMU_BASE_ADDR+0x40)) //PMU_OS_REG0

uint32 IReadLoaderFlag(void)
{
    return (*LOADER_FLAG_REG);
}

void ISetLoaderFlag(uint32 flag)
{
    if(*LOADER_FLAG_REG == flag)
        return;
    *LOADER_FLAG_REG = flag;
}


