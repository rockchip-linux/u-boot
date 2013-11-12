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

#if !defined(CONFIG_FASTBOOT_NO_FORMAT)

extern struct fbt_partition fbt_partitions[];

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

#define SYS_LOADER_REBOOT_FLAG   0x5242C300 
#define SYS_KERNRL_REBOOT_FLAG   0xC3524200
#define SYS_LOADER_ERR_FLAG      0X1888AAFF

enum {
    BOOT_NORMAL=                  0,
    BOOT_LOADER,     /* enter loader rockusb mode */
    BOOT_MASKROM,    /* enter maskrom rockusb mode*/
    BOOT_RECOVER,    /* enter recover */
    BOOT_NORECOVER,  /* do not enter recover */
    BOOT_WINCE,      /* FOR OTHER SYSTEM */
    BOOT_WIPEDATA,   /* enter recover and wipe data. */
    BOOT_WIPEALL,    /* enter recover and wipe all data. */
    BOOT_CHECKIMG,   /* check firmware img with backup part(in loader mode)*/
    BOOT_FASTBOOT,
    BOOT_SECUREBOOT_DISABLE,
    BOOT_CHARGING,
    BOOT_MAX         /* MAX VALID BOOT TYPE.*/
};

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
                startRockusb();
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

