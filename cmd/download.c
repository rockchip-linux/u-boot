/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <console.h>
#include <asm/io.h>

#define BOOT_BROM_DOWNLOAD	0xEF08A53C

__weak void do_board_download(void)
{
}

static int do_rbrom(struct cmd_tbl *cmdtp, int flag,
		    int argc, char *const argv[])
{
	writel(BOOT_BROM_DOWNLOAD, (void *)CONFIG_ROCKCHIP_BOOT_MODE_REG);
	do_reset(NULL, 0, 0, NULL);

	return 0;
}

static int do_download(struct cmd_tbl *cmdtp, int flag,
		       int argc, char *const argv[])
{
	disable_ctrlc(1);

	/* Allow board specific download, maybe noreturn */
	do_board_download();

	/* Generic download */
#ifdef CONFIG_CMD_ROCKUSB
	run_command("rockusb 0 $devtype $devnum", 0);
#endif
	printf("Enter rockusb failed, fallback to bootrom...\n");
	flush();
	run_command("rbrom", 0);

	return 0;
}

U_BOOT_CMD(
	rbrom, 1, 0,	do_rbrom,
	"reboot to bootrom",
	""
);

U_BOOT_CMD(
	download, 1, 1, do_download,
	"enter rockusb/bootrom download mode", ""
);
