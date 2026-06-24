/*
 * (C) Copyright 2026 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <android_bootloader.h>
#include <command.h>
#include <part.h>

/*
 * Example: create a 16MB size android_esp.img:
 *
 *	./scripts/esp_fat_copy.py --efi gbl_aarch64_prod.efi --output esp.img --size 16M
 */
static int do_boot_gbl(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct blk_desc *dev_desc;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("dev_desc is NULL!\n");
		return CMD_RET_FAILURE;
	}

	if (!is_gbl_bootflow(dev_desc)) {
		printf("android_esp partition not found\n");
		return CMD_RET_FAILURE;
	}

	printf("## Booting GBL\n");

	run_command("bootefi bootmgr", 0);

	return CMD_RET_FAILURE;
}

/* U_BOOT_CMD_ALWAYS( */
U_BOOT_CMD(
	boot_gbl,  1,     1,	do_boot_gbl,
	"Boot GBL from android_esp partition",
	""
);
