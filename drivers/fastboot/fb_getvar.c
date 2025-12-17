// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2016 The Android Open Source Project
 */

#include <env.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_mmc.h>
#include <fb_nand.h>
#include <fs.h>
#include <part.h>
#include <tee.h>
#include <tee/optee.h>
#include <version.h>
#include <vsprintf.h>
#include <linux/printk.h>

static void getvar_version(char *var_parameter, char *response);
static void getvar_version_bootloader(char *var_parameter, char *response);
static void getvar_downloadsize(char *var_parameter, char *response);
static void getvar_serialno(char *var_parameter, char *response);
static void getvar_version_baseband(char *var_parameter, char *response);
static void getvar_product(char *var_parameter, char *response);
static void getvar_platform(char *var_parameter, char *response);
static void getvar_current_slot(char *var_parameter, char *response);
static void getvar_has_slot(char *var_parameter, char *response);
static void getvar_partition_type(char *part_name, char *response);
static void getvar_partition_size(char *part_name, char *response);
static void getvar_is_userspace(char *var_parameter, char *response);
static void getvar_logical_blocksize(char *var_parameter, char *response);
static void getvar_erase_blocksize(char *var_parameter, char *response);
static void getvar_vboot_state(char *var_parameter, char *response);
static void getvar_unlocked(char *var_parameter, char *response);
static void getvar_flash_unlocked(char *var_parameter, char *response);
static void getvar_slot_suffixes(char *var_parameter, char *response);
static void getvar_slot_successful(char *var_parameter, char *response);
static void getvar_slot_unbootable(char *var_parameter, char *response);
static void getvar_slot_retry_count(char *var_parameter, char *response);
static void getvar_avb_state(char *var_parameter, char *response);
static void getvar_snapshot_update_status(char *var_parameter, char *response);

static const struct {
	const char *variable;
	bool list;
	void (*dispatch)(char *var_parameter, char *response);
} getvar_dispatch[] = {
	{
		.variable = "version",
		.dispatch = getvar_version,
		.list = true,
	}, {
		.variable = "version-bootloader",
		.dispatch = getvar_version_bootloader,
		.list = true
	}, {
		.variable = "downloadsize",
		.dispatch = getvar_downloadsize,
		.list = true
	}, {
		.variable = "max-download-size",
		.dispatch = getvar_downloadsize,
		.list = true
	}, {
		.variable = "serialno",
		.dispatch = getvar_serialno,
		.list = true
	}, {
		.variable = "version-baseband",
		.dispatch = getvar_version_baseband,
		.list = true
	}, {
		.variable = "product",
		.dispatch = getvar_product,
		.list = true
	}, {
		.variable = "platform",
		.dispatch = getvar_platform,
		.list = true
	}, {
		.variable = "current-slot",
		.dispatch = getvar_current_slot,
		.list = true
#if IS_ENABLED(CONFIG_FASTBOOT_FLASH)
	}, {
		.variable = "has-slot",
		.dispatch = getvar_has_slot,
		.list = false
#endif
#if IS_ENABLED(CONFIG_FASTBOOT_FLASH_MMC)
	}, {
		.variable = "partition-type",
		.dispatch = getvar_partition_type,
		.list = false
#endif
#if IS_ENABLED(CONFIG_FASTBOOT_FLASH)
	}, {
		.variable = "partition-size",
		.dispatch = getvar_partition_size,
		.list = false
#endif
	}, {
		.variable = "is-userspace",
		.dispatch = getvar_is_userspace,
		.list = true
	}, {
		.variable = "logical-block-size",
		.dispatch = getvar_logical_blocksize,
		.list = true
	}, {
		.variable = "erase-block-size",
		.dispatch = getvar_erase_blocksize,
		.list = true
	}, {
		.variable = "vboot-state",
		.dispatch = getvar_vboot_state,
		.list = true
	}, {
		.variable = "unlocked",
		.dispatch = getvar_unlocked,
		.list = true
	}, {
		.variable = "flash-unlocked",
		.dispatch = getvar_flash_unlocked,
		.list = true
	}, {
		.variable = "slot-suffixes",
		.dispatch = getvar_slot_suffixes,
		.list = true
	}, {
		.variable = "slot-successful",
		.dispatch = getvar_slot_successful,
		.list = true
	}, {
		.variable = "slot-unbootable",
		.dispatch = getvar_slot_unbootable,
		.list = true
	}, {
		.variable = "slot-retry-count",
		.dispatch = getvar_slot_retry_count,
		.list = true
	}, {
		.variable = "avb-state",
		.dispatch = getvar_avb_state,
		.list = true
	}, {
		.variable = "snapshot-update-status",
		.dispatch = getvar_snapshot_update_status,
		.list = true
	}
};

/**
 * Get partition number and size for any storage type.
 *
 * Can be used to check if partition with specified name exists.
 *
 * If error occurs, this function guarantees to fill @p response with fail
 * string. @p response can be rewritten in caller, if needed.
 *
 * @param[in] part_name Info for which partition name to look for
 * @param[in,out] response Pointer to fastboot response buffer
 * @param[out] size If not NULL, will contain partition size
 * Return: Partition number or negative value on error
 */
static int getvar_get_part_info(const char *part_name, char *response,
				size_t *size)
{
	int r;
	struct blk_desc *dev_desc;
	struct disk_partition disk_part;
	struct part_info *part_info;

	if (IS_ENABLED(CONFIG_FASTBOOT_FLASH_MMC)) {
		r = fastboot_mmc_get_part_info(part_name, &dev_desc, &disk_part,
					       response);
		if (r >= 0 && size)
			*size = disk_part.size * disk_part.blksz;
	} else if (IS_ENABLED(CONFIG_FASTBOOT_FLASH_NAND)) {
		r = fastboot_nand_get_part_info(part_name, &part_info, response);
		if (r >= 0 && size)
			*size = part_info->size;
	} else {
		fastboot_fail("this storage is not supported in bootloader", response);
		r = -ENODEV;
	}

	return r;
}

static void getvar_version(char *var_parameter, char *response)
{
	fastboot_okay(FASTBOOT_VERSION, response);
}

static void getvar_version_bootloader(char *var_parameter, char *response)
{
	fastboot_okay(U_BOOT_VERSION, response);
}

static void getvar_downloadsize(char *var_parameter, char *response)
{
	fastboot_response("OKAY", response, "0x%08x", fastboot_buf_size);
}

static void getvar_serialno(char *var_parameter, char *response)
{
	const char *tmp = env_get("serial#");

	if (tmp)
		fastboot_okay(tmp, response);
	else
		fastboot_fail("Value not set", response);
}

static void getvar_version_baseband(char *var_parameter, char *response)
{
	fastboot_okay("N/A", response);
}

static void getvar_product(char *var_parameter, char *response)
{
	const char *board = env_get("board");

	if (board)
		fastboot_okay(board, response);
	else
		fastboot_fail("Board not set", response);
}

static void getvar_platform(char *var_parameter, char *response)
{
	const char *p = env_get("platform");

	if (p)
		fastboot_okay(p, response);
	else
		fastboot_fail("platform not set", response);
}

static void getvar_current_slot(char *var_parameter, char *response)
{
	/* A/B not implemented, for now always return "a" */
	fastboot_okay("a", response);
}

static void __maybe_unused getvar_has_slot(char *part_name, char *response)
{
	char part_name_wslot[PART_NAME_LEN];
	size_t len;
	int r;

	if (!part_name || part_name[0] == '\0')
		goto fail;

	/* part_name_wslot = part_name + "_a" */
	len = strlcpy(part_name_wslot, part_name, PART_NAME_LEN - 3);
	if (len >= PART_NAME_LEN - 3)
		goto fail;
	strcat(part_name_wslot, "_a");

	r = getvar_get_part_info(part_name_wslot, response, NULL);
	if (r >= 0) {
		fastboot_okay("yes", response); /* part exists and slotted */
		return;
	}

	r = getvar_get_part_info(part_name, response, NULL);
	if (r >= 0)
		fastboot_okay("no", response); /* part exists but not slotted */

	/* At this point response is filled with okay or fail string */
	return;

fail:
	fastboot_fail("invalid partition name", response);
}

static void __maybe_unused getvar_partition_type(char *part_name, char *response)
{
	int r;
	struct blk_desc *dev_desc;
	struct disk_partition part_info;

	r = fastboot_mmc_get_part_info(part_name, &dev_desc, &part_info,
				       response);
	if (r >= 0) {
		r = fs_set_blk_dev_with_part(dev_desc, r);
		if (r < 0)
			/* If we don't know then just default to raw */
			fastboot_okay("raw", response);
		else
			fastboot_okay(fs_get_type_name(), response);
	}
}

static void __maybe_unused getvar_partition_size(char *part_name, char *response)
{
	int r;
	size_t size;

	r = getvar_get_part_info(part_name, response, &size);
	if (r >= 0)
		fastboot_response("OKAY", response, "0x%016zx", size);
}

static void getvar_is_userspace(char *var_parameter, char *response)
{
	fastboot_okay("no", response);
}

static void __maybe_unused getvar_logical_blocksize(char *var_parameter, char *response)
{
	struct blk_desc *dev_desc;

	dev_desc = plat_bootdev();
	if (!dev_desc)
		fastboot_fail("Block device not found", response);
	else
		fastboot_response("OKAY", response, "0x%lx", dev_desc->blksz);
}

static void __maybe_unused getvar_erase_blocksize(char *var_parameter, char *response)
{
	lbaint_t erase_grp_size;

#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	erase_grp_size = fb_mmc_get_erase_grp_size();

	if (erase_grp_size < 0)
		fastboot_fail("Block device not found", response);
	else
		fastboot_response("OKAY", response, "0x"LBAF"", erase_grp_size);
#else
	fastboot_fail("Not implemented, please enable CONFIG_FASTBOOT_FLASH_MMC_DEV",
		      response);
#endif
}

static void __maybe_unused getvar_vboot_state(char *var_parameter, char *response)
{
	uint8_t vboot_flag = 0;

#ifdef CONFIG_OPTEE
	if (optee_read_vbootkey_enable_flag(&vboot_flag)) {
		fastboot_fail("Can't read vboot flag", response);
		return;
	}

	if (vboot_flag)
		fastboot_okay("Yes", response);
	else
		fastboot_okay("No", response);
#else
	fastboot_fail("Not implemented, please enable CONFIG_OPTEE", response);
#endif
}

static void __maybe_unused getvar_unlocked(char *var_parameter, char *response)
{
#ifdef CONFIG_LIBAVB_USER
	uint8_t lock_state = 0;

	if (!avb_read_lock_state(&lock_state)) {
		fastboot_fail("Read lock_state failed", response);
		return;
	}
	if (lock_state)
		fastboot_okay("AVB unlock", response);
	else
		fastboot_okay("AVB lock", response);
#else
	fastboot_fail("Not implemented, please enable CONFIG_LIBAVB_USER", response);
#endif
}

static void __maybe_unused getvar_flash_unlocked(char *var_parameter, char *response)
{
#ifdef CONFIG_LIBAVB_USER
	uint8_t flash_lock_state = 0;

	if (!avb_read_flash_lock_state(&flash_lock_state)) {
		fastboot_fail("Read flash_lock_state failed", response);
		return;
	}
	if (flash_lock_state)
		fastboot_okay("flash unlock", response);
	else
		fastboot_okay("flash unlock", response);
#else
	fastboot_fail("Not implemented, please enable CONFIG_LIBAVB_USER", response);
#endif
}

static void __maybe_unused getvar_slot_suffixes(char *var_parameter, char *response)
{
	char slot_suffixes_temp[4] = {0};
	char slot_suffixes[9] = {0};
	int slot_cnt = 0;

	if (ab_get_current_slot(slot_suffixes_temp)) {
		fastboot_fail("Get current_slot failed", response);
		return;
	}

	while (slot_suffixes_temp[slot_cnt] != '\0') {
		slot_suffixes[slot_cnt * 2]
			= slot_suffixes_temp[slot_cnt];
		slot_suffixes[slot_cnt * 2 + 1] = ',';
		slot_cnt++;
	}

	slot_suffixes[(slot_cnt - 1) * 2 + 1] = '\0';
	fastboot_response("OKAY", response, "%s", slot_suffixes);
}

static void __maybe_unused getvar_slot_successful(char *var_parameter, char *response)
{
	char *slot_name = var_parameter;
	AvbABData ab_info;

	if (!var_parameter || !slot_name) {
		fastboot_fail("Argument Invalid", response);
		return;
	}

	if (ab_get_slot_data(&ab_info) < 0) {
		fastboot_fail("Get A/B system info failed", response);
		return;
	}

	if (!strcmp(slot_name, "_a")) {
		if (ab_info.slots[0].successful_boot)
			fastboot_okay("Yes", response);
		else
			fastboot_okay("No", response);
	} else if (!strcmp(slot_name, "_b")) {
		if (ab_info.slots[1].successful_boot)
			fastboot_okay("Yes", response);
		else
			fastboot_okay("No", response);
	} else {
		fastboot_fail("Argument Invalid", response);
	}
}

static void __maybe_unused getvar_slot_unbootable(char *var_parameter, char *response)
{
	char *slot_name = var_parameter;
	AvbABData ab_info;

	if (!var_parameter || !slot_name) {
		fastboot_fail("Argument Invalid", response);
		return;
	}

	if (ab_get_slot_data(&ab_info) < 0) {
		fastboot_fail("Get A/B system info failed", response);
		return;
	}

	if (!strcmp(slot_name, "_a")) {
		if (!ab_info.slots[0].successful_boot &&
			!ab_info.slots[0].tries_remaining &&
			!ab_info.slots[0].priority)
			fastboot_okay("Yes", response);
		else
			fastboot_okay("No", response);
	} else if (!strcmp(slot_name, "_b")) {
		if (!ab_info.slots[1].successful_boot &&
			!ab_info.slots[1].tries_remaining &&
			!ab_info.slots[1].priority)
			fastboot_okay("Yes", response);
		else
			fastboot_okay("No", response);
	} else {
		fastboot_fail("Argument Invalid", response);
	}
}

static void __maybe_unused getvar_slot_retry_count(char *var_parameter, char *response)
{
	char *slot_name = var_parameter;
	AvbABData ab_info;

	if (!var_parameter || !slot_name) {
		fastboot_fail("Argument Invalid", response);
		return;
	}

	if (ab_get_slot_data(&ab_info) < 0) {
		fastboot_fail("Get A/B system info failed", response);
		return;
	}

	if (!strcmp(slot_name, "_a"))
		fastboot_response("OKAY", response, "%d", ab_info.slots[0].tries_remaining);
	else if (!strcmp(slot_name, "_b"))
		fastboot_response("OKAY", response, "%d", ab_info.slots[1].tries_remaining);
	else
		fastboot_fail("Argument Invalid", response);
}

static void __maybe_unused getvar_avb_state(char *var_parameter, char *response)
{
	char vbst[AVB_STATE_SIZE] = {0};
	char *p_vbst;

	avb_get_state(vbst);
	p_vbst = vbst;
	do {
		var_parameter = strsep(&p_vbst, "\n");
		if (strlen(var_parameter) > 0)
			fastboot_response("OKAY", response, "%s", var_parameter);
	} while (strlen(var_parameter));
}

static void __maybe_unused getvar_snapshot_update_status(char *var_parameter, char *response)
{
#ifdef CONFIG_ANDROID_AB
	struct misc_virtual_ab_message state;

	memset(&state, 0x0, sizeof(state));
	if (read_misc_virtual_ab_message(&state) != 0) {
		fastboot_fail("Get virtual A/B system info failed", response);
		return;
	}

	if (state.magic != MISC_VIRTUAL_AB_MAGIC_HEADER) {
		fastboot_fail("Virtual A/B system info has incorrect magic", response);
		return;
	}

	if (state.merge_status == ENUM_MERGE_STATUS_MERGING)
		fastboot_okay("Merging", response);
	else if (state.merge_status == ENUM_MERGE_STATUS_SNAPSHOTTED)
		fastboot_okay("Snapshotted", response);
	else
		fastboot_okay("None", response);
#else
	fastboot_fail("Not implemented, please enable CONFIG_ANDROID_AB", response);
#endif
}

static int current_all_dispatch;
void fastboot_getvar_all(char *response)
{
	/*
	 * Find a dispatch getvar that can be listed and send
	 * it as INFO until we reach the end.
	 */
	while (current_all_dispatch < ARRAY_SIZE(getvar_dispatch)) {
		if (!getvar_dispatch[current_all_dispatch].list) {
			current_all_dispatch++;
			continue;
		}

		char envstr[FASTBOOT_RESPONSE_LEN] = { 0 };

		getvar_dispatch[current_all_dispatch].dispatch(NULL, envstr);

		char *envstr_start = envstr;

		if (!strncmp("OKAY", envstr, 4) || !strncmp("FAIL", envstr, 4))
			envstr_start += 4;

		fastboot_response("INFO", response, "%s: %s",
				  getvar_dispatch[current_all_dispatch].variable,
				  envstr_start);

		current_all_dispatch++;
		return;
	}

	fastboot_response("OKAY", response, NULL);
	current_all_dispatch = 0;
}

/**
 * fastboot_getvar() - Writes variable indicated by cmd_parameter to response.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 *
 * Look up cmd_parameter first as an environment variable of the form
 * fastboot.<cmd_parameter>, if that exists return use its value to set
 * response.
 *
 * Otherwise lookup the name of variable and execute the appropriate
 * function to return the requested value.
 */
void fastboot_getvar(char *cmd_parameter, char *response)
{
	if (!cmd_parameter) {
		fastboot_fail("missing var", response);
	} else if (!strncmp("all", cmd_parameter, 3) && strlen(cmd_parameter) == 3) {
		current_all_dispatch = 0;
		fastboot_response(FASTBOOT_MULTIRESPONSE_START, response, NULL);
	} else {
#define FASTBOOT_ENV_PREFIX	"fastboot."
		int i;
		char *var_parameter = cmd_parameter;
		char envstr[FASTBOOT_RESPONSE_LEN];
		const char *s;

		snprintf(envstr, sizeof(envstr) - 1,
			 FASTBOOT_ENV_PREFIX "%s", cmd_parameter);
		s = env_get(envstr);
		if (s) {
			fastboot_response("OKAY", response, "%s", s);
			return;
		}

		strsep(&var_parameter, ":");
		for (i = 0; i < ARRAY_SIZE(getvar_dispatch); ++i) {
			if (!strcmp(getvar_dispatch[i].variable,
				    cmd_parameter)) {
				getvar_dispatch[i].dispatch(var_parameter,
							    response);
				return;
			}
		}
		pr_warn("WARNING: unknown variable: %s\n", cmd_parameter);
		fastboot_fail("Variable not implemented", response);
	}
}
