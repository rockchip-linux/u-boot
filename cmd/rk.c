// SPDX-License-Identifier: GPL-2.0+
/*
 * Implements the 'bd' command to show board information
 *
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <dm.h>
#include <blk.h>

DECLARE_GLOBAL_DATA_PTR;

int do_rockchip(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct blk_desc *desc;

	desc = plat_bootdev();
	if (!desc)
		return -1;

	printf("bdesc: %s\n", desc->bdev->name);

	return 0;
}

U_BOOT_CMD(
	rockchip,	1,	1,	do_rockchip,
	"print Board Info structure",
	""
);
