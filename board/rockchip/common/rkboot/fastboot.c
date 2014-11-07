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
#include <linux/input.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_CMD_FASTBOOT
extern void fbt_fastboot_init(void);
#endif

extern uint32 GetVbus(void);
extern void rkloader_change_cmd_for_recovery(PBootInfo boot_info , char * rec_cmd );
extern int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot);
#ifdef CONFIG_LCD
extern void rk_backlight_ctrl(int brightness);
extern int lcd_enable_logo(bool enable);
extern int drv_lcd_init(void);
extern void lcd_standby(int enable);
#endif
extern int is_charging(void);
extern void powerOn(void);

static int g_cold_boot = 0;

#if defined(CONFIG_RK_PWM_REMOTE)
extern int g_ir_keycode;
#endif


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
#ifdef CONFIG_POWER_RK
	return is_charging();
#else
	return 0;
#endif
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

	if (!g_cold_boot) {
		g_cold_boot = !loader_flag;
	}

	//set to non-0.
	ISetLoaderFlag(SYS_KERNRL_REBOOT_FLAG | reboot_mode);

	if(SYS_LOADER_ERR_FLAG == loader_flag)
	{
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
				printf("reboot to rockusb.\n");
				do_rockusb(NULL, 0, 0, NULL);
				break;
#ifdef CONFIG_CMD_FASTBOOT
			case BOOT_FASTBOOT:
				frt = FASTBOOT_REBOOT_FASTBOOT;
				break;
#endif
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

#ifdef CONFIG_RK_SDCARD_BOOT_EN
	if ((reboot_mode == BOOT_NORMAL) && (StorageSDCardUpdateMode() != 0)) {
		// Normal boot mode, when detect sd card update£¬audo entern recovery
		frt = FASTBOOT_REBOOT_RECOVERY;
	}
#endif

	return frt;
}

int board_fbt_key_pressed(void)
{
	int boot_rockusb = 0, boot_recovery = 0, boot_fastboot = 0; 
	enum fbt_reboot_type frt = FASTBOOT_REBOOT_UNKNOWN;
	int vbus = GetVbus();
	int ir_keycode = 0;

	checkKey((uint32 *)&boot_rockusb, (uint32 *)&boot_recovery, (uint32 *)&boot_fastboot);
#if defined(CONFIG_RK_PWM_REMOTE)
	ir_keycode = g_ir_keycode;
	RemotectlDeInit();
#endif
	printf("vbus = %d\n", vbus);
	if((boot_recovery && (vbus==0)) || (ir_keycode  == KEY_POWER)) {
		printf("recovery key pressed.\n");
		frt = FASTBOOT_REBOOT_RECOVERY;
	} else if ((boot_rockusb && (vbus!=0)) || (ir_keycode  == KEY_HOME)) {
		printf("rockusb key pressed.\n");
		/* rockusb key press, set flag = 1 for rockusb timeout check */
		if (do_rockusb(NULL, 1, 0, NULL) == 1) {
			/* if rockusb return 1, boot recovery */
			frt = FASTBOOT_REBOOT_RECOVERY;
		}
#ifdef CONFIG_CMD_FASTBOOT
	} else if(boot_fastboot && (vbus!=0)) {
		printf("fastboot key pressed.\n");
		frt = FASTBOOT_REBOOT_FASTBOOT;
#endif
	} else if(ir_keycode == KEY_DOWN) {
		printf("recovery wipe data key pressed.\n");
		frt = FASTBOOT_REBOOT_RECOVERY_WIPE_DATA;
	}

#if defined(CONFIG_RK_PWM_REMOTE)
	printf("%s: ir_keycode = 0x%x, frt = %d\n", __func__, ir_keycode, frt);
#endif

	return frt;
}

void board_fbt_finalize_bootargs(char* args, int buf_sz,
		int ramdisk_addr, int ramdisk_sz, int recovery)
{
	char recv_cmd[2]={0};

	rkloader_fixInitrd(&gBootInfo, ramdisk_addr, ramdisk_sz);
	if (recovery) {
		rkloader_change_cmd_for_recovery(&gBootInfo, recv_cmd);
	}
	snprintf(args, buf_sz, "%s", gBootInfo.cmd_line);

}

void board_fbt_boot_failed(const char* boot)
{
	printf("Unable to boot:%s\n", boot);

#ifdef CONFIG_CMD_BOOTRK
	if (!memcmp(BOOT_NAME, boot, sizeof(BOOT_NAME))) {
		printf("try to start recovery\n");
		char *const boot_cmd[] = {"bootrk", RECOVERY_NAME};
		do_bootrk(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
	} else if (!memcmp(RECOVERY_NAME, boot, sizeof(RECOVERY_NAME))) {
		printf("try to start backup\n");
		char *const boot_cmd[] = {"bootrk", BACKUP_NAME};
		do_bootrk(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
	}  
#endif
	printf("try to start rockusb\n");
	do_rockusb(NULL, 0, 0, NULL);
}


#ifdef CONFIG_CMD_FASTBOOT
static void board_fbt_request_start_fastboot(void)
{
	char buf[512];
	char *old_preboot = getenv("preboot");
	FBTDBG("old preboot env = %s\n", old_preboot);

	if (old_preboot) {
		snprintf(buf, sizeof(buf),
				"setenv preboot %s; fastboot", old_preboot);
		setenv("preboot", buf);
	} else
		setenv("preboot", "setenv preboot; fastboot");

	FBTDBG("%s: setting preboot env to %s\n", __func__, getenv("preboot"));
}

int board_fbt_oem(const char *cmdbuf)
{
#ifdef CONFIG_ENABLE_ERASEKEY
	if (!strcmp(cmdbuf, "erasekey"))
		return rkidb_erase_drm_key();
#endif
	return -1;
}

int board_fbt_handle_erase(const disk_partition_t *ptn)
{
	return rkimage_partition_erase(ptn);
}

int board_fbt_handle_flash(const char *name, const disk_partition_t *ptn,
		struct cmd_fastboot_interface *priv)
{
	return rkimage_store_image(name, ptn, priv);
}

int board_fbt_handle_download(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv)
{
	return rkimage_handleDownload(buffer, length, priv);
}
#endif /* CONFIG_CMD_FASTBOOT */


int board_fbt_load_partition_table(void)
{
	return load_disk_partitions();
}

const disk_partition_t* board_fbt_get_partition(const char* name)
{
	return get_disk_partition(name);
}


#ifdef CONFIG_CMD_CHARGE_ANIM
static void board_fbt_run_charge(void)
{
	char *const boot_charge_cmd[] = {"bootrk", "charge"};
#ifdef CONFIG_CMD_BOOTRK
	do_bootrk(NULL, 0, ARRAY_SIZE(boot_charge_cmd), boot_charge_cmd);
#endif

	/* returns if boot.img is bad */
	FBTERR("\nfastboot: Error: Invalid boot img\n");
}
#endif


static void board_fbt_run_recovery(void)
{
#ifdef CONFIG_CMD_BOOTRK
	char *const boot_recovery_cmd[] = {"bootrk", "recovery"};
	do_bootrk(NULL, 0, ARRAY_SIZE(boot_recovery_cmd), boot_recovery_cmd);
#endif

	/* returns if recovery.img is bad */
	FBTERR("\nfastboot: Error: Invalid recovery img\n");
}


void board_fbt_run_recovery_wipe_data(void)
{
	struct bootloader_message bmsg;

	FBTDBG("Rebooting into recovery to do wipe_data\n");

	if (!board_fbt_get_partition("misc"))
	{
		FBTERR("not found misc partition, just run recovery.\n");
		board_fbt_run_recovery();
	}

	memset((char *)&bmsg, 0, sizeof(struct bootloader_message));
	strcpy(bmsg.command, "boot-recovery");
	bmsg.status[0] = 0;
	strcpy(bmsg.recovery, "recovery\n--wipe_data");
	rkloader_set_bootloader_msg(&bmsg);
	/* now reboot to recovery */
	board_fbt_run_recovery();
}



/*
 * Determine if we should enter fastboot mode based on board specific
 * key press or parameter left in memory from previous boot.
 *
 * This is also where we initialize fbt private data.  Even if we
 * don't enter fastboot mode, we need our environment setup for
 * things like unlock state, etc.
 */
void board_fbt_preboot(void)
{
	enum fbt_reboot_type frt;

#ifdef CONFIG_CMD_FASTBOOT
	/* need to init this ASAP so we know the unlocked state */
	fbt_fastboot_init();
#endif

	frt = board_fbt_get_reboot_type();
	if ((frt == FASTBOOT_REBOOT_UNKNOWN) || (frt == FASTBOOT_REBOOT_NORMAL)) {
		FBTDBG("\n%s: no spec reboot type, check key press.\n", __func__);
		frt = board_fbt_key_pressed();
	} else {
		//clear reboot type.
		board_fbt_set_reboot_type(FASTBOOT_REBOOT_NORMAL);
	}

#ifdef CONFIG_POWER_RK
	if (is_power_extreme_low()) {
		while (is_charging()) {
			FBTERR("extreme low power, charging...\n");
			udelay(1000000); /* 1 sec */
			if (!is_power_extreme_low()) {
				FBTERR("extreme low power charge done\n");
				break;
			}
		}
	}

	if (is_power_extreme_low()) {
		//it should be extreme low power without charger connected.
		FBTERR("extreme low power, shutting down...\n");
		shut_down();
		printf("not reach here.\n");
	}
#endif

	int logo_on = 0;
	if (gd->fdt_blob) {
		int node = fdt_path_offset(gd->fdt_blob, "/fb");
		logo_on = fdtdec_get_int(gd->fdt_blob, node, "rockchip,uboot-logo-on", 0);
	}
	printf("read logo_on switch from dts [%d]\n", logo_on);

#ifdef CONFIG_LCD
	if (logo_on) {
		drv_lcd_init();   //move backlight enable to board_init_r, for don't show logo in rockusb
	}
#endif

#ifdef CONFIG_POWER_RK
	if (is_power_low()) {
		if (!is_charging()) {
			FBTERR("low power, shutting down...\n");
#ifdef CONFIG_LCD
			//TODO: show warning logo.
			show_resource_image("images/battery_fail.bmp");

			lcd_standby(0);
			//TODO: set backlight in better way.
			rk_backlight_ctrl(CONFIG_BRIGHTNESS_DIM);

			udelay(1000000);//1 sec

			rk_backlight_ctrl(0);
			lcd_standby(1);
#endif
			shut_down();
			printf("not reach here.\n");
		}
	}
#endif

#ifdef CONFIG_UBOOT_CHARGE
	//check charge mode when no key pressed.
	int cold_boot = board_fbt_is_cold_boot();
	if ((cold_boot && board_fbt_is_charging())
			|| frt == FASTBOOT_REBOOT_CHARGE) {
#ifdef CONFIG_CMD_CHARGE_ANIM
		char *charge[] = { "charge" };
		if (logo_on && do_charge(NULL, 0, ARRAY_SIZE(charge), charge)) {
			//boot from charge animation.
			frt = FASTBOOT_REBOOT_NORMAL;
		}
#else
		return fbt_run_charge();
#endif
	}
#endif //CONFIG_UBOOT_CHARGE

	powerOn();

#ifdef CONFIG_LCD
	if (logo_on) {
		lcd_enable_logo(true);
		lcd_standby(0);
		mdelay(100);
		rk_backlight_ctrl(-1); /*use defaut brightness in dts*/
	}
#endif

	if (frt == FASTBOOT_REBOOT_RECOVERY) {
		FBTDBG("\n%s: starting recovery img because of reboot flag\n", __func__);
		return board_fbt_run_recovery();
	} else if (frt == FASTBOOT_REBOOT_RECOVERY_WIPE_DATA) {
		FBTDBG("\n%s: starting recovery img to wipe data "
				"because of reboot flag\n", __func__);
		/* we've not initialized most of our state so don't
		 * save env in this case
		 */
		return board_fbt_run_recovery_wipe_data();
	}
#ifdef CONFIG_CMD_FASTBOOT
	else if (frt == FASTBOOT_REBOOT_FASTBOOT) {
		FBTDBG("\n%s: starting fastboot because of reboot flag\n", __func__);
		board_fbt_request_start_fastboot();
	}
#endif
	else {
		FBTDBG("\n%s: check misc command.\n", __func__);
		/* unknown reboot cause (typically because of a cold boot).
		 * check if we had misc command to boot recovery.
		 */
		rkloader_run_misc_cmd();
	}
}
