/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <asm/io.h>
#include <cli.h>
#include <common.h>
#include <linux/compat.h>
#include <malloc.h>
#include "test-rockchip.h"

int board_emmc_test(int argc, char * const argv[])
{
	u8 *write_buffer, *read_buffer;
	u32 i, blocks = 0, lba;
	unsigned long ts;
	int err = 0;
	char cmd_mmc[512] = {0};

	if (argc < 4) {
		printf("Usage: rktest emmc start_lba blocks\n");
		printf("blocks should be from 8129 to 30000\n");
		err = -EINVAL;
		goto err_wb;
	}

	lba = simple_strtoul(argv[2], NULL, 0);
	blocks = simple_strtoul(argv[3], NULL, 0);

	if (blocks % 2)
		/* Round up */
		blocks += 1;

	if (blocks < 8192) {
		printf("Round up to 8192 blocks compulsively\n");
		blocks = 8192;
	}

	if (blocks > 30000) {
		printf("Round down to 30000 blocks compulsively\n");
		blocks = 30000;
	}

	/* 1. Prepare memory */

	write_buffer = (u8 *)kmalloc(sizeof(u8) * blocks * 512, GFP_KERNEL);
	if (!write_buffer) {
		printf("No memory for write_buffer!\n");
		err = -ENOMEM;
		goto err_wb;
	}

	read_buffer = (u8 *)kmalloc(sizeof(u8) * blocks * 512, GFP_KERNEL);
	if (!read_buffer) {
		printf("No memory for read_buffer!\n");
		err = -ENOMEM;
		goto err_rb;
	}

	for (i = 0; i < blocks * 512; i++) {
		write_buffer[i] = i;
		read_buffer[i] = 0;
	}

	/* 2. Prepare and start cli command */

	snprintf(cmd_mmc, sizeof(cmd_mmc), "mmc write 0x%x 0x%x 0x%x",
		 (u32)(ulong)write_buffer, lba, blocks);
	ts = get_timer(0);
	err = cli_simple_run_command(cmd_mmc, 0);
	ts = get_timer(0) - ts;
	if (!err)
		goto err_mw;

	printf("eMMC write: size %dMB, used %ldms, speed %ldMB/s\n",
		blocks / 2048, ts, (blocks >> 1) / ts);

	snprintf(cmd_mmc, sizeof(cmd_mmc), "mmc read 0x%x 0x%x 0x%x",
		 (u32)(ulong)read_buffer, lba, blocks);
	ts = get_timer(0);
	err = cli_simple_run_command(cmd_mmc, 0);
	ts = get_timer(0) - ts;
	if (!err)
		goto err_mw;

	printf("eMMC read: size %dMB, used %ldms, speed %ldMB/s\n",
		blocks / 2048, ts, (blocks >> 1) / ts);

	/* 3. Verify the context */

	err = 0;
	for (i = 0; i < blocks * 512; i++) {
		if (write_buffer[i] != read_buffer[i]) {
			printf("eMMC context compare err!\n");
			err = -EINVAL;
			goto err_mw;
		}
	}

err_mw:
	kfree(read_buffer);
	read_buffer = NULL;
err_rb:
	kfree(write_buffer);
	write_buffer = NULL;
err_wb:
	return err;
}
