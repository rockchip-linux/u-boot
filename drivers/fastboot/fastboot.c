/*
 * Copyright (C) 2011 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
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
	return 0;
}

int board_fbt_oem(const char *cmdbuf)
{
	if (!strcmp(cmdbuf,"format"))
		return do_format();
	return -1;
}
#endif /* !CONFIG_FASTBOOT_NO_FORMAT */

void board_fbt_set_reboot_type(enum fbt_reboot_type frt)
{
    switch(frt) {
        case FASTBOOT_REBOOT_NORMAL:
        case FASTBOOT_REBOOT_BOOTLOADER:
        case FASTBOOT_REBOOT_RECOVERY:
        case FASTBOOT_REBOOT_RECOVERY_WIPE_DATA:
            break;
        default:
            printf("unknown reboot type %d\n", frt);
            frt = FASTBOOT_REBOOT_UNKNOWN;
            break;
    }
    ISetLoaderFlag(frt);
}

enum fbt_reboot_type board_fbt_get_reboot_type(void)
{
    enum fbt_reboot_type frt = IReadLoaderFlag();

    /* clear before next boot */
    ISetLoaderFlag(FASTBOOT_REBOOT_UNKNOWN);

    switch(frt) {
    case FASTBOOT_REBOOT_NORMAL:
    case FASTBOOT_REBOOT_BOOTLOADER:
    case FASTBOOT_REBOOT_RECOVERY:
    case FASTBOOT_REBOOT_RECOVERY_WIPE_DATA:
      break;
    default:
      printf("unknown reboot type %d\n", frt);
      frt = FASTBOOT_REBOOT_UNKNOWN;
      break;
    }

    return frt;
}

