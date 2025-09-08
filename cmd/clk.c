// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2013 Xilinx, Inc.
 */
#include <command.h>
#include <clk.h>
#include <dm.h>
#include <dm/device.h>
#include <dm/root.h>
#include <dm/device-internal.h>
#include <linux/clk-provider.h>

static int do_clk_dump(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	int ret;

	ret = soc_clk_dump();
	if (ret < 0) {
		printf("Clock dump error %d\n", ret);
		ret = CMD_RET_FAILURE;
	}

	return ret;
}

static int do_clk_setfreq(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	struct clk *clk = NULL;
	s32 freq;
	struct udevice *dev;

	if (argc != 3)
		return CMD_RET_USAGE;

	freq = dectoul(argv[2], NULL);

	if (!uclass_get_device_by_name(UCLASS_CLK, argv[1], &dev))
		clk = dev_get_clk_ptr(dev);

	if (!clk) {
		printf("clock '%s' not found.\n", argv[1]);
		return CMD_RET_FAILURE;
	}

	freq = clk_set_rate(clk, freq);
	if (freq < 0) {
		printf("set_rate failed: %d\n", freq);
		return CMD_RET_FAILURE;
	}

	printf("set_rate returns %u\n", freq);
	return 0;
}

static struct cmd_tbl cmd_clk_sub[] = {
	U_BOOT_CMD_MKENT(dump, 1, 1, do_clk_dump, "", ""),
	U_BOOT_CMD_MKENT(setfreq, 3, 1, do_clk_setfreq, "", ""),
};

static int do_clk(struct cmd_tbl *cmdtp, int flag, int argc,
		  char *const argv[])
{
	struct cmd_tbl *c;

	if (argc < 2)
		return CMD_RET_USAGE;

	/* Strip off leading 'clk' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_clk_sub[0], ARRAY_SIZE(cmd_clk_sub));

	if (c)
		return c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}

U_BOOT_LONGHELP(clk,
	"dump - Print clock frequencies\n"
	"clk setfreq [clk] [freq] - Set clock frequency");

U_BOOT_CMD(clk, 4, 1, do_clk, "CLK sub-system", clk_help_text);
