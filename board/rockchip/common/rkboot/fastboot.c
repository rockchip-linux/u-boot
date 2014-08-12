/*
 * (C) Copyright 2008-2014 Rockchip Electronics
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
#include <malloc.h>
#include <fastboot.h>
#include <errno.h>
#include <version.h>
#include <asm/io.h>

#include <power/pmic.h>
#include <asm/arch/rkplat.h>
#include "../config.h"

extern uint32 GetVbus(void);
extern void change_cmd_for_recovery(PBootInfo boot_info , char * rec_cmd );
extern int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot);
extern int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

static int g_cold_boot = 0;

#if !defined(CONFIG_FASTBOOT_NO_FORMAT)
static int do_format(void)
{
#if 0
	struct ptable *ptbl = &the_ptable;
	unsigned next;
	int n;
	block_dev_desc_t *dev_desc;
	unsigned long blocks_to_write, result;

	dev_desc = get_dev_by_name(FASTBOOT_BLKDEV);
	if (!dev_desc) {
		printf("error getting device %s\n", FASTBOOT_BLKDEV);
		return -1;
	}
	if (!dev_desc->lba) {
		printf("device %s has no space\n", FASTBOOT_BLKDEV);
		return -1;
	}

	printf("blocks %lu\n", dev_desc->lba);

	start_ptbl(ptbl, dev_desc->lba);
	for (n = 0, next = 0; fbt_partitions[n].name; n++) {
		u64 sz = fbt_partitions[n].size_kb * 2;
		if (fbt_partitions[n].name[0] == '-') {
			next += sz;
			continue;
		}
		if (sz == 0)
			sz = dev_desc->lba - next;
		if (add_ptn(ptbl, next, next + sz - 1, fbt_partitions[n].name))
			return -1;
		next += sz;
	}
	end_ptbl(ptbl);

	blocks_to_write = DIV_ROUND_UP(sizeof(struct ptable), dev_desc->blksz);
	result = dev_desc->block_write(dev_desc->dev, 0, blocks_to_write, ptbl);
	if (result != blocks_to_write) {
		printf("\nFormat failed, block_write() returned %lu instead of %lu\n", result, blocks_to_write);
		return -1;
	}

	printf("\nnew partition table of %lu %lu-byte blocks\n",
			blocks_to_write, dev_desc->blksz);
	fbt_reset_ptn();
#endif
	//TODO:lowlevel format
	return -1;
}

int board_fbt_oem(const char *cmdbuf)
{
	if (!strcmp(cmdbuf,"format"))
		return do_format();
#ifdef CONFIG_ENABLE_ERASEKEY
	else if (!strcmp(cmdbuf,"erasekey"))
		return eraseDrmKey();
#endif
	return -1;
}
#endif /* !CONFIG_FASTBOOT_NO_FORMAT */

/**
 * return 1 if is a cold boot.
 */
int board_fbt_is_cold_boot(void)
{
	return g_cold_boot;
}

/**
 * return 1 if is charging.
 */
int board_fbt_is_charging(void)
{
	return is_charging();
}

void board_fbt_set_reboot_type(enum fbt_reboot_type frt)
{
	int boot = BOOT_NORMAL;
	switch(frt) {
		case FASTBOOT_REBOOT_BOOTLOADER:
			boot = BOOT_LOADER;
			break;
		case FASTBOOT_REBOOT_FASTBOOT:
			boot = BOOT_FASTBOOT;
			break;
		case FASTBOOT_REBOOT_RECOVERY:
			boot = BOOT_RECOVER;
			break;
		case FASTBOOT_REBOOT_RECOVERY_WIPE_DATA:
			boot = BOOT_WIPEDATA;
			break;
		default:
			printf("unknown reboot type %d\n", frt);
			frt = BOOT_NORMAL;
			break;
	}

	ISetLoaderFlag(SYS_LOADER_REBOOT_FLAG|boot);
}

enum fbt_reboot_type board_fbt_get_reboot_type(void)
{
	enum fbt_reboot_type frt = FASTBOOT_REBOOT_UNKNOWN;

	uint32_t loader_flag = IReadLoaderFlag();
	int reboot_mode = loader_flag ? (loader_flag & 0xFF) : BOOT_NORMAL;

	if (!g_cold_boot)
		g_cold_boot = !loader_flag;

	//set to non-0.
	ISetLoaderFlag(SYS_KERNRL_REBOOT_FLAG | reboot_mode);

	if(SYS_LOADER_ERR_FLAG == loader_flag)
	{
		printf("reboot to rockusb.\n");
		loader_flag = SYS_LOADER_REBOOT_FLAG | BOOT_LOADER;
		reboot_mode = BOOT_LOADER;
	}

	if((loader_flag&0xFFFFFF00) == SYS_LOADER_REBOOT_FLAG)
	{
		switch(reboot_mode) {
			case BOOT_NORMAL:
				frt = FASTBOOT_REBOOT_NORMAL;
				break;
			case BOOT_LOADER:
				do_rockusb(NULL, 0, 0, NULL);
				break;
			case BOOT_FASTBOOT:
				frt = FASTBOOT_REBOOT_FASTBOOT;
				break;
			case BOOT_RECOVER:
				frt = FASTBOOT_REBOOT_RECOVERY;
				break;
			case BOOT_WIPEDATA:
			case BOOT_WIPEALL:
				frt = FASTBOOT_REBOOT_RECOVERY_WIPE_DATA;
				break;
			case BOOT_CHARGING:
				frt = FASTBOOT_REBOOT_CHARGE;
				break;
			default:
				printf("unsupport rk boot type %d\n", reboot_mode);
				break;
		}
	}

	return frt;
}

int board_fbt_key_pressed(void)
{
	int boot_rockusb = 0, boot_recovery = 0, boot_fastboot = 0; 
	enum fbt_reboot_type frt = FASTBOOT_REBOOT_NONE;
	int vbus = GetVbus();

	checkKey((uint32 *)&boot_rockusb, (uint32 *)&boot_recovery, (uint32 *)&boot_fastboot);
	printf("vbus = %d\n", vbus);
	if(boot_recovery && (vbus==0)) {
		printf("recovery key pressed.\n");
		frt = FASTBOOT_REBOOT_RECOVERY;
	} else if (boot_rockusb && (vbus!=0)) {
		printf("rockusb key pressed.\n");
		/* rockusb key press, set flag = 1 for rockusb timeout check */
		if (do_rockusb(NULL, 1, 0, NULL) == 1) {
			/* if rockusb return 1, boot recovery */
			frt = FASTBOOT_REBOOT_RECOVERY;
		}
	} else if(boot_fastboot && (vbus!=0)){
		printf("fastboot key pressed.\n");
		frt = FASTBOOT_REBOOT_FASTBOOT;
	}

	return frt;
}

void board_fbt_finalize_bootargs(char* args, int buf_sz,
		int ramdisk_addr, int ramdisk_sz, int recovery)
{
	char recv_cmd[2]={0};
	fixInitrd(&gBootInfo, ramdisk_addr, ramdisk_sz);
	if (recovery) {
		change_cmd_for_recovery(&gBootInfo, recv_cmd);
	}
	snprintf(args, buf_sz, "%s", gBootInfo.cmd_line);
	//TODO:setup serial_no/device_id/mac here?
}
int board_fbt_handle_erase(const disk_partition_t *ptn)
{
	return handleErase(ptn);
}
int board_fbt_handle_flash(const char *name, const disk_partition_t *ptn,
		struct cmd_fastboot_interface *priv)
{
	return handleRkFlash(name, ptn, priv);
}
int board_fbt_handle_download(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv)
{
	return handleDownload(buffer, length, priv);
}
int board_fbt_check_misc()
{
	//return true if we got recovery cmd from misc.
	return checkMisc();
}
int board_fbt_set_bootloader_msg(struct bootloader_message* bmsg)
{
	return setBootloaderMsg(bmsg);
}
int board_fbt_boot_check(struct fastboot_boot_img_hdr *hdr, int unlocked)
{
	return secureCheck(hdr, unlocked);
}
void board_fbt_boot_failed(const char* boot)
{
	printf("Unable to boot:%s\n", boot);

#ifdef CONFIG_CMD_BOOTI
	if (!memcmp(BOOT_NAME, boot, sizeof(BOOT_NAME))) {
		printf("try to start recovery\n");
		char *const boot_cmd[] = {"booti", RECOVERY_NAME};
		do_booti(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
	} else if (!memcmp(RECOVERY_NAME, boot, sizeof(RECOVERY_NAME))) {
		printf("try to start backup\n");
		char *const boot_cmd[] = {"booti", BACKUP_NAME};
		do_booti(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
	}  
#endif
	printf("try to start rockusb\n");
	do_rockusb(NULL, 0, 0, NULL);
}


int board_fbt_load_partition_table(void)
{
	return load_disk_partitions();
}

const disk_partition_t* board_fbt_get_partition(const char* name)
{
	return get_disk_partition(name);
}

#ifndef CONFIG_CMD_FASTBOOT
static void fbt_handle_reboot(const char *cmdbuf)
{
	if (!strcmp(&cmdbuf[6], "-bootloader")) {
		FBTDBG("%s\n", cmdbuf);
		board_fbt_set_reboot_type(FASTBOOT_REBOOT_BOOTLOADER);
	}
	if (!strcmp(&cmdbuf[6], "-recovery")) {
		FBTDBG("%s\n", cmdbuf);
		board_fbt_set_reboot_type(FASTBOOT_REBOOT_RECOVERY);
	}
	if (!strcmp(&cmdbuf[6], "-recovery:wipe_data")) {
		FBTDBG("%s\n", cmdbuf);
		board_fbt_set_reboot_type(FASTBOOT_REBOOT_RECOVERY_WIPE_DATA);
	}

	udelay(1000000); /* 1 sec */

	do_reset(NULL, 0, 0, NULL);
}

static void fbt_run_recovery(void)
{
#ifdef CONFIG_CMD_BOOTI
	char *const boot_recovery_cmd[] = {"booti", "recovery"};
	do_booti(NULL, 0, ARRAY_SIZE(boot_recovery_cmd), boot_recovery_cmd);
#endif

	/* returns if recovery.img is bad */
	FBTERR("\nfastboot: Error: Invalid recovery img\n");
}

static void fbt_run_recovery_wipe_data(void)
{
	struct bootloader_message bmsg;

	FBTDBG("Rebooting into recovery to do wipe_data\n");

	if (!board_fbt_get_partition("misc"))
	{
		FBTERR("not found misc partition, just run recovery.\n");
		fbt_run_recovery();
	}
	strcpy(bmsg.command, "boot-recovery");
	bmsg.status[0] = 0;
	strcpy(bmsg.recovery, "recovery\n--wipe_data");
	if (board_fbt_set_bootloader_msg(&bmsg))
	{
		FBTERR("set bootloader msg failed, retry!\n");
		fbt_handle_reboot("reboot-recovery:wipe_data");
	}
	/* now reboot to recovery */
	fbt_run_recovery();
}

void rk_preboot(void)
{
	enum fbt_reboot_type frt;

	frt = board_fbt_key_pressed();
	if (frt == FASTBOOT_REBOOT_NONE) {
		FBTDBG("\n%s: no spec key pressed, get requested reboot type.\n", __func__);
		frt = board_fbt_get_reboot_type();
	} else {
		//clear reboot type when key pressed.
		board_fbt_set_reboot_type(FASTBOOT_REBOOT_NONE);
	}

	if (frt == FASTBOOT_REBOOT_RECOVERY) {
		return fbt_run_recovery();
	} else if (frt == FASTBOOT_REBOOT_RECOVERY_WIPE_DATA) {
		FBTDBG("\n%s: starting recovery img to wipe data "
			"because of reboot flag\n", __func__);
		/* we've not initialized most of our state so don't
		 * save env in this case
		 */
		return fbt_run_recovery_wipe_data();
	} else {
		FBTDBG("\n%s: check misc command.\n", __func__);
		/* unknown reboot cause (typically because of a cold boot).
		 * check if we had misc command to boot recovery.
		 */
		int run_recovery = board_fbt_check_misc();
		if (run_recovery) {
			FBTDBG("\n%s: starting recovery because of misc command\n", __func__);
			return fbt_run_recovery();
		}
		FBTDBG("\n%s: no special reboot flags, doing normal boot\n", __func__);
	}
}
#endif
