/********************************************************************************
  COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
  --  ALL RIGHTS RESERVED  --
  File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
 ********************************************************************************/
#include <common.h>
#include <malloc.h>
#include <fastboot.h>
#include <asm/io.h>
#include <version.h>
#include <asm/arch/gpio.h>
#include <errno.h>

#include "config.h"
#include "rkimage.h"
#include "idblock.h"

extern uint32 GetVbus(void);
extern int do_booti(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);
extern void change_cmd_for_recovery(PBootInfo boot_info , char * rec_cmd );
extern int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot);
extern int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

struct fbt_partition fbt_partitions[FBT_PARTITION_MAX_NUM];
char PRODUCT_NAME[20] = FASTBOOT_PRODUCT_NAME;

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
	int boot = BOOT_NORMAL;
	if(SYS_LOADER_ERR_FLAG == loader_flag)
	{
		printf("reboot to rockusb.\n");
		loader_flag = SYS_LOADER_REBOOT_FLAG | BOOT_LOADER;
	}

	if((loader_flag&0xFFFFFF00) == SYS_LOADER_REBOOT_FLAG)
	{
		boot = loader_flag&0xFF;

		ISetLoaderFlag(SYS_KERNRL_REBOOT_FLAG|boot);
		switch(boot) {
			case BOOT_NORMAL:
				frt = FASTBOOT_REBOOT_NORMAL;
				break;
			case BOOT_LOADER:
				//startRockusb();
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
				printf("unsupport rk boot type %d\n", boot);
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
		printf("%s: recovery key pressed.\n",__func__);
		frt = FASTBOOT_REBOOT_RECOVERY;
	} else if (boot_rockusb && (vbus!=0)) {
		printf("%s: rockusb key pressed.\n",__func__);
		//    startRockusb();
		do_rockusb(NULL, 0, 0, NULL);
	} else if(boot_fastboot && (vbus!=0)){
		printf("%s: fastboot key pressed.\n",__func__);
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
int board_fbt_handle_erase(fbt_partition_t *ptn)
{
	return handleErase(ptn);
}
int board_fbt_handle_flash(const char *name, fbt_partition_t *ptn,
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

	if (!memcmp(BOOT_NAME, boot, sizeof(BOOT_NAME))) {
		printf("try to start recovery\n");
		char *const boot_cmd[] = {"booti", RECOVERY_NAME};
		do_booti(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
	} else if (!memcmp(RECOVERY_NAME, boot, sizeof(RECOVERY_NAME))) {
		printf("try to start backup\n");
		char *const boot_cmd[] = {"booti", BACKUP_NAME};
		do_booti(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
	}  
	printf("try to start rockusb\n");
	//startRockusb();
	do_rockusb(NULL, 0, 0, NULL);
}
