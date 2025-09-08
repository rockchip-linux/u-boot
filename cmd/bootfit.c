/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <android_ab.h>
#include <bootm.h>
#include <image.h>
#include <malloc.h>
#include <sysmem.h>
#include <asm/arch-rockchip/common.h>
#include <asm/arch-rockchip/fit.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

static void *do_boot_fit_storage(ulong *size)
{
	return fit_image_load_bootables(size);
}

static void *do_boot_fit_ram(char *const argv[], ulong *data_size)
{
	void *fit;
	int size;

	fit = (void *)simple_strtoul(argv[1], NULL, 16);
	if (!fit || fdt_check_header(fit)) {
		FIT_I("Invalid header\n");
		return NULL;
	}

	size = fit_image_get_bootables_size(fit);
	if (!size) {
		FIT_I("Failed to get bootable image size\n");
		return NULL;
	}

	/* reserve this full FIT image */
	if (!sysmem_alloc_base(MEM_FIT_USER,
			       (phys_addr_t)fit, ALIGN(size, 512)))
		return NULL;

	*data_size = size;

	return fit;
}

/*
 * argc == 1:
 *	FIT image is loaded from storage(eg. CONFIG_BOOTCOMMAND).
 *
 * argc == 2:
 *	FIT image is already in ram, the booflow is:
 *		CLI cmd "bootm <fit_addr>" => do_bootm() =>
 *		board_do_bootm() => boot_fit <fit_addr>
 */
static int do_boot_fit(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct bootm_info bmi;
	char fit_addr[12];
	ulong size;
	void *fit;

	if (argc > 2)
		return CMD_RET_USAGE;

	printf("## Booting FIT Image ");

	if (argc == 1)
		fit = do_boot_fit_storage(&size);
	else
		fit = do_boot_fit_ram(argv, &size);

	if (!fit) {
		FIT_I("No FIT image\n");
		goto fail;
	}

	if (fdt_check_header(fit)) {
		FIT_I("Invalid FIT format\n");
		goto fail;
	}

	/* fixup entry/load and alloc sysmem */
	if (fit_image_pre_process(fit))
		goto fail;

	printf("at 0x%08lx with size 0x%08lx\n", (ulong)fit, size);

#ifdef CONFIG_ANDROID_AB
	char slot_suffix[3] = {0};
	char slot_info[21] = "android_slotsufix=";

	if (ab_get_slot_suffix(slot_suffix))
		goto fail;

	strcat(slot_info, slot_suffix);
	env_update("bootargs", slot_info);
#endif

	/* boot! */
	snprintf(fit_addr, sizeof(fit_addr), "0x%lx", (ulong)fit);
	bootm_init(&bmi);
	bmi.addr_img = fit_addr;
	if (!bootm_run(&bmi))
		return CMD_RET_SUCCESS;

	fit_image_fail_process(fit);
fail:
	return CMD_RET_FAILURE;
}

/* U_BOOT_CMD_ALWAYS( */
U_BOOT_CMD(
	boot_fit,  2,     1,      do_boot_fit,
	"Boot FIT Image from memory or boot/recovery partition",
	"boot_fit [addr]"
);
