/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include "test-rockchip.h"

typedef struct board_module {
	char *name;
	char *desc;
	int (*test)(int argc, char * const argv[]);
} board_module_t;

static int board_rockusb_test(int argc, char * const argv[])
{
	return run_command_list("rockusb 0 ${devtype} ${devnum}", -1, 0);
}

static int board_fastboot_test(int argc, char * const argv[])
{
	return run_command_list("fastboot usb 0", -1, 0);
}

static board_module_t g_board_modules[] = {
#if defined(CONFIG_IRQ)
	{
		.name = "timer",
		.desc = "test timer and interrupt",
		.test = board_timer_test
	},
#endif
	{
		.name = "brom",
		.desc = "enter bootrom download mode",
		.test = board_brom_dnl_test
	},
	{
		.name = "rockusb",
		.desc = "enter rockusb download mode",
		.test = board_rockusb_test
	},
	{
		.name = "fastboot",
		.desc = "enter fastboot download mode",
		.test = board_fastboot_test
	},
#if defined(CONFIG_DM_KEY)
	{
		.name = "key",
		.desc = "test board keys",
		.test = board_key_test
	},
#endif
#if defined(CONFIG_MMC)
	{
		.name = "emmc",
		.desc = "test emmc read/write speed",
		.test = board_emmc_test
	},
#endif
#if defined(CONFIG_RKNAND)
	{
		.name = "rknand",
		.desc = "test rknand read/write speed",
		.test = board_rknand_test
	},
#endif

#if defined(CONFIG_DM_REGULATOR)
	{
		.name = "regulator",
		.desc = "test regulator volatge set and show regulator status",
		.test = board_regulator_test
	},
#endif
#if defined(CONFIG_GMAC_ROCKCHIP)
	{
		.name = "eth",
		.desc = "test ethernet",
		.test = board_eth_test
	},
#endif
#if defined(CONFIG_RK_IR)
	{
		.name = "ir",
		.desc = "test pwm ir remoter for box product",
		.test = board_ir_test
	},
#endif
#if defined(CONFIG_ROCKCHIP_VENDOR_PARTITION)
	{
		.name = "vendor",
		.desc = "test vendor storage partition read/write",
		.test = board_vendor_storage_test
	},
#endif
};

static void help(void)
{
	int i;

	printf("Command: rktest [module] [args...]\n"
	       "  - module: timer|key|emmc|rknand|regulator|eth|ir|brom|rockusb|fastboot|vendor\n"
	       "  - args: depends on module, try 'rktest [module]' for test or more help\n\n");

	printf("  - Enabled modules:\n");
	for (i = 0; i < ARRAY_SIZE(g_board_modules); i++)
		printf("     - %10s%s %s\n",
		       g_board_modules[i].name,
		       g_board_modules[i].desc ? ":" : "",
		       g_board_modules[i].desc ? g_board_modules[i].desc : "");
}

static int do_rockchip_test(cmd_tbl_t *cmdtp, int flag,
			    int argc, char * const argv[])
{
	ulong ms_start = 0, ms = 0, sec = 0;
	board_module_t *module = NULL;
	char *module_name = NULL;
	int index = 0, err = 0;
	bool found = false;

	if (argc >= 2) {
		module_name = argv[1];
	} else {
		help();
		return 0;
	}

	if (!module_name)
		return 0;

	for (index = 0; index < ARRAY_SIZE(g_board_modules); index++) {
		module = &g_board_modules[index];
		if (module && !strcmp(module->name, module_name)) {
			found = true;

			printf("***********************************************************\n");
			printf("Rockchip Board Module [%s] Test start.\n", module_name);
			printf("***********************************************************\n");

			ms_start = get_timer(0);

			err = module->test(argc, argv);

			ms = get_timer(ms_start);
			if (ms >= 1000) {
				sec = ms / 1000;
				ms = ms % 1000;
			}

			printf("-----------------------------------------------------------\n");
			printf("Rockchip Board Module [%s] Test end <%s>.. Total: %lu.%lus\n",
			       module->name, err ? "FAILED" : "PASS", sec, ms);
			printf("-----------------------------------------------------------\n\n\n");
		}
	}

	if (!found)
		printf("Rockchip Board Module [%s] is not support !\n",
		       module_name);

	return 0;
}

U_BOOT_CMD(
	rktest, 10, 0, do_rockchip_test,
	"Rockchip Board Module Test",
	""
);
