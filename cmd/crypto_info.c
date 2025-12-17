// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 */
#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <crypto_manager.h>
#include <dm.h>
#include <hexdump.h>

static int do_crypto_info_single(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;

	if (strcmp(argv[1], "tree") == 0) {
		crypto_dump_tree();
	} else {
		printf("==============================================================\n");
		if (strcmp(argv[1], "hash") == 0)
			crypto_dump_info_by_type(CRYPTO_TYPE_HASH);
		else if (strcmp(argv[1], "hmac") == 0)
			crypto_dump_info_by_type(CRYPTO_TYPE_HMAC);
		else if (strcmp(argv[1], "cipher") == 0)
			crypto_dump_info_by_type(CRYPTO_TYPE_CIPHER);
		else if (strcmp(argv[1], "asym") == 0)
			crypto_dump_info_by_type(CRYPTO_TYPE_ASYM);
		else
			ret = CMD_RET_USAGE;
	}

	return ret;
}

static int do_crypto_info(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 2)
		return CMD_RET_USAGE;

	return do_crypto_info_single(cmdtp, flag, argc, argv);
}
U_BOOT_LONGHELP(crypto_info,
	"tree         - Show all algorithm categories\n"
	"crypto_info hash         - Show hash algorithms\n"
	"crypto_info hmac         - Show hmac algorithms\n"
	"crypto_info cipher       - Show cipher algorithms\n"
	"crypto_info asym         - Show asymmetric algorithms\n"
);

U_BOOT_CMD(
	crypto_info, 2, 0, do_crypto_info,
	"Show crypto algorithm info",
	crypto_info_help_text
);