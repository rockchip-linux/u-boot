/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include "test-rockchip.h"

#ifdef CONFIG_CMD_BOOT_ANDROID
static int do_test_android(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
	return run_command("boot_android ${devtype} ${devnum}", 0);
}
#endif

#ifdef CONFIG_AVB_VERIFY
static int do_test_android_avb(struct cmd_tbl *cmdtp, int flag,
			       int argc, char *const argv[])
{
	return run_command("boot_android ${devtype} ${devnum}", 0);
}
#endif

#ifdef CONFIG_CMD_BOOT_ROCKCHIP
static int do_test_bootrkp(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
	return run_command("bootrkp", 0);
}
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
static int do_test_distro(struct cmd_tbl *cmdtp, int flag,
			  int argc, char *const argv[])
{
	return run_command("run distro_bootcmd", 0);
}
#endif

static struct cmd_tbl sub_cmd[] = {
#ifdef CONFIG_CMD_BOOT_ANDROID
	UNIT_CMD_ATTR_DEFINE(android, 0, CMD_FLG_NORETURN),
#endif
#ifdef CONFIG_AVB_VERIFY
	UNIT_CMD_ATTR_DEFINE(android_avb, 0, CMD_FLG_NORETURN),
#endif
#ifdef CONFIG_CMD_BOOT_ROCKCHIP
	UNIT_CMD_ATTR_DEFINE(bootrkp, 0, CMD_FLG_NORETURN),
#endif
#ifdef CONFIG_DISTRO_DEFAULTS
	UNIT_CMD_ATTR_DEFINE(distro, 0, CMD_FLG_NORETURN),
#endif
};

static const char sub_cmd_help[] =
#ifdef CONFIG_CMD_BOOT_ANDROID
"    [n] rktest android                     - test android bootflow\n"
#endif
#ifdef CONFIG_AVB_VERIFY
"    [n] rktest android_avb                 - test android avb bootflow\n"
#endif
#ifdef CONFIG_CMD_BOOT_ROCKCHIP
"    [n] rktest bootrkp                     - test bootrkp bootflow\n"
#endif
#ifdef CONFIG_DISTRO_DEFAULTS
"    [n] rktest distro                      - test linux distro bootflow\n"
#endif
;

const struct cmd_group cmd_grp_boot = {
	.id	= TEST_ID_BOOT,
	.help	= sub_cmd_help,
	.cmd	= sub_cmd,
	.cmd_n	= ARRAY_SIZE(sub_cmd),
};
