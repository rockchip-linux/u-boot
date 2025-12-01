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

static void dump_crypto_info(const struct crypto_impl *impl)
{
	const char *driver_name;
	u32 algo_exist = false;
	u32 priority;
	u32 i, j;

	driver_name = crypto_get_driver_name(impl);
	if (!driver_name)
		driver_name = "Unknown";

	printf("--------------------------------------------------------------\n");
	printf("Driver Name: %s\n", driver_name);

	if (impl->type == CRYPTO_TYPE_HASH) {
#if defined(CONFIG_DM_HASH)
		printf("\t[HASH]\n");

		for (i = 0; i < HASH_ALGO_NUM; i++) {
			if (impl->check_valid && impl->check_valid(impl->dev, i, CRYPTO_MODE_NONE)) {
				priority = impl->dynamic_priority ?
					   impl->dynamic_priority(impl->dev, i, CRYPTO_MODE_NONE) :
					   impl->priority;
				printf("\t\t%-16s        (prio = %3d)\n",
				       hash_algo_name(i),
				       priority);
			}
		}

		printf("\n");
#endif
	} else if (impl->type == CRYPTO_TYPE_HMAC) {
#if defined(CONFIG_DM_HMAC)
		printf("\t[HMAC]\n");

		for (i = 0; i < HMAC_ALGO_NUM; i++) {
			if (impl->check_valid && impl->check_valid(impl->dev, i, CRYPTO_MODE_NONE)) {
				priority = impl->dynamic_priority ?
					   impl->dynamic_priority(impl->dev, i, CRYPTO_MODE_NONE) :
					   impl->priority;
				printf("\t\t%-16s        (prio = %3d)\n",
				       hmac_algo_name(i),
				       priority);
			}
		}

		printf("\n");
#endif
	} else if (impl->type == CRYPTO_TYPE_ASYM) {
		printf("\t[ASYM]\n");

		for (i = 0; i < ASYM_ALGO_NUM; i++) {
			if (impl->check_valid && impl->check_valid(impl->dev, i, CRYPTO_MODE_NONE)) {
				priority = impl->dynamic_priority ?
					   impl->dynamic_priority(impl->dev, i, CRYPTO_MODE_NONE) :
					   impl->priority;
				printf("\t\t%-16s        (prio = %3d)\n",
				       asym_algo_name(i),
				       priority);
			}
		}

		printf("\n");
	} else if (impl->type == CRYPTO_TYPE_CIPHER) {
#if defined(CONFIG_DM_CIPHER)
		printf("\t[CIPHER]\n");

		for (i = 0; i < CIPHER_ALGO_NUM; i++) {
			for (j = 0; j < CIPHER_MODE_NUM; j++) {
				if (impl->check_valid && impl->check_valid(impl->dev, i, j)) {
					if (!algo_exist) {
						algo_exist = true;
						printf("\t\t%-16s\n", cipher_algo_name(i));
					}

					priority = impl->dynamic_priority ?
						   impl->dynamic_priority(impl->dev, i, j) :
						   impl->priority;
					printf("\t\t\t %-14s (prio = %3d)\n",
					       cipher_mode_name(j),
					       priority);
				}
			}

			algo_exist = false;
		}

		printf("\n");
#endif
	}
}

static void dump_crypto_info_by_type(enum crypto_type type)
{
	const struct crypto_impl *impl = NULL;
	u32 index;

	for (index = 0; index < CRYPTO_DRIVER_MAX; index++) {
		impl = crypto_get_impl_by_index(type, index);
		if (!impl)
			break;

		dump_crypto_info(impl);
	}
}

static int do_crypto_info_all(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 type;

	printf("==============================================================\n");

	for (type = 0; type < CRYPTO_TYPE_MAX; type++) {
		dump_crypto_info_by_type(type);
	}

	return 0;
}

static int do_crypto_info_single(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret= 0;

	printf("==============================================================\n");

	if (strcmp(argv[1], "hash") == 0)
		dump_crypto_info_by_type(CRYPTO_TYPE_HASH);
	else if (strcmp(argv[1], "hmac") == 0)
		dump_crypto_info_by_type(CRYPTO_TYPE_HMAC);
	else if (strcmp(argv[1], "cipher") == 0)
		dump_crypto_info_by_type(CRYPTO_TYPE_CIPHER);
	else if (strcmp(argv[1], "asym") == 0)
		dump_crypto_info_by_type(CRYPTO_TYPE_ASYM);
	else
		ret = CMD_RET_USAGE;

	return ret;
}

static int do_crypto_info(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return do_crypto_info_all(cmdtp, flag, argc, argv);
	else if (argc == 2)
		return do_crypto_info_single(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}
U_BOOT_LONGHELP(crypto_info,
	"  crypto_info              - Show all algorithm categories\n"
	"  crypto_info hash         - Show hash algorithms\n"
	"  crypto_info hmac         - Show hmac algorithms\n"
	"  crypto_info cipher       - Show cipher algorithms\n"
	"  crypto_info asym         - Show asymmetric algorithms\n"
);

U_BOOT_CMD(
	crypto_info, 2, 0, do_crypto_info,
	"Show crypto algorithm info",
	crypto_info_help_text
);