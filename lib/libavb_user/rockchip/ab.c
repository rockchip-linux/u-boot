/*
 * (C) Copyright 2020 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <mapmem.h>
#include <errno.h>
#include <command.h>
#include <part.h>
#include <android_avb/ab.h>

AvbABFlowResult ab_slot_select(AvbABOps* ab_ops,char* select_slot)
{
	AvbABFlowResult ret = AVB_AB_FLOW_RESULT_OK;
	AvbIOResult io_ret = AVB_IO_RESULT_OK;
	AvbABData ab_data;
	size_t slot_index_to_boot;
	static int last_slot_index = -1;

	io_ret = ab_ops->read_ab_metadata(ab_ops, &ab_data);
	if (io_ret != AVB_IO_RESULT_OK) {
		printf("I/O error while loading A/B metadata.\n");
		ret = AVB_AB_FLOW_RESULT_ERROR_IO;
		goto out;
	}
	if (slot_is_bootable(&ab_data.slots[0]) && slot_is_bootable(&ab_data.slots[1])) {
		if (ab_data.slots[1].priority > ab_data.slots[0].priority) {
			slot_index_to_boot = 1;
		} else {
			slot_index_to_boot = 0;
		}
	} else if(slot_is_bootable(&ab_data.slots[0])) {
		slot_index_to_boot = 0;
	} else if(slot_is_bootable(&ab_data.slots[1])) {
		slot_index_to_boot = 1;
	} else {
		printf("No bootable slots found.\n");
		ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
		goto out;
	}

	if (slot_index_to_boot == 0) {
		strcpy(select_slot, "_a");
	} else if(slot_index_to_boot == 1) {
		strcpy(select_slot, "_b");
	}

	if (last_slot_index != slot_index_to_boot) {
		last_slot_index = slot_index_to_boot;
		printf("A/B-slot: %s, successful: %d, tries-remain: %d (select)\n",
		       select_slot,
		       ab_data.slots[slot_index_to_boot].successful_boot,
		       ab_data.slots[slot_index_to_boot].tries_remaining);

		printf("A/B-slot: %s, successful: %d, tries-remain: %d\n",
		       (!slot_index_to_boot) == 0 ? "_a" : "_b",
		       ab_data.slots[!slot_index_to_boot].successful_boot,
		       ab_data.slots[!slot_index_to_boot].tries_remaining);
	}
out:
	return ret;
}

AvbABFlowResult ab_get_lastboot(void)
{

	AvbIOResult io_ret = AVB_IO_RESULT_OK;
	AvbABData ab_data;
	int lastboot = -1;
	AvbOps* ops;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	io_ret = ops->ab_ops->read_ab_metadata(ops->ab_ops, &ab_data);
	if (io_ret != AVB_IO_RESULT_OK) {
		printf("I/O error while loading A/B metadata.\n");
		goto out;
	}

	lastboot = ab_data.last_boot;
out:
	avb_ops_user_free(ops);

	return lastboot;
}

AvbABFlowResult ab_get_current_slot(char *select_slot)
{
	AvbOps* ops;
	int ret = AVB_AB_FLOW_RESULT_OK;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	if (ab_slot_select(ops->ab_ops, select_slot) != 0) {
#ifndef CONFIG_AVB_VERIFY
		printf("###There is no bootable slot, bring up last_boot!###\n");
		if (ab_get_lastboot() == 1)
			avb_memcpy(select_slot, "_b", 2);
		else if(ab_get_lastboot() == 0)
			avb_memcpy(select_slot, "_a", 2);
		else {
			printf("No valid last_boot. Boot from slot-A by default.\n");
			avb_memcpy(select_slot, "_a", 2);
		}
#endif
		ret = AVB_AB_FLOW_RESULT_OK;
	}

	avb_ops_user_free(ops);
	return ret;
}

bool ab_have_bootable_slot(void)
{
	char slot[3] = {0};

	if (ab_get_current_slot(slot))
		return false;
	else
		return true;
}

AvbABFlowResult ab_append_part_slot(const char *part_name, char *new_name)
{
	char slot_suffix[3] = {0};

	if (!strcmp(part_name, "misc")) {
		strcat(new_name, part_name);
		return AVB_AB_FLOW_RESULT_OK;
	}

	if (ab_get_current_slot(slot_suffix)) {
		printf("%s: failed to get slot suffix !\n", __func__);
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	strcpy(new_name, part_name);
	strcat(new_name, slot_suffix);

	return AVB_AB_FLOW_RESULT_OK;
}

AvbABFlowResult ab_get_part_has_slot_info(const char *base_name)
{
	char *part_name;
	int part_num;
	size_t part_name_len;
	struct disk_partition part_info;
	struct blk_desc *dev_desc;
	const char *slot_suffix = "_a";

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device!\n", __func__);
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	if (base_name == NULL) {
		printf("The base_name is NULL!\n");
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	part_name_len = strlen(base_name) + 1;
	part_name_len += strlen(slot_suffix);
	part_name = malloc(part_name_len);
	if (!part_name) {
		printf("%s can not malloc a buffer!\n", __FILE__);
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	avb_memset(part_name, 0, part_name_len);
	snprintf(part_name, part_name_len, "%s%s", base_name, slot_suffix);
	part_num = part_get_info_by_name(dev_desc, part_name, &part_info);
	if (part_num < 0) {
		printf("Could not find partition \"%s\"\n", part_name);
		part_num = -1;
	}

	free(part_name);
	return part_num;
}

AvbABFlowResult ab_update_stored_rollback_indexes_for_slot(AvbOps* ops, AvbSlotVerifyData* slot_data)
{
	uint64_t rollback_index = slot_data->rollback_indexes[0];
	uint64_t current_stored_rollback_index;
	AvbIOResult io_ret;

	if (rollback_index > 0) {
		io_ret = ops->read_rollback_index(ops, 0, &current_stored_rollback_index);
		if (io_ret != AVB_IO_RESULT_OK)
			return AVB_AB_FLOW_RESULT_ERROR_IO;

		if (rollback_index > current_stored_rollback_index) {
			io_ret = ops->write_rollback_index(ops, 0, rollback_index);
			if (io_ret != AVB_IO_RESULT_OK)
				return AVB_AB_FLOW_RESULT_ERROR_IO;
		}
	}

	return 0;
}

AvbABFlowResult ab_get_slot_data(AvbABData* ab_data)
{
	AvbOps* ops;
	AvbIOResult io_ret;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	io_ret = ops->ab_ops->read_ab_metadata(ops->ab_ops, ab_data);
	if (io_ret != AVB_IO_RESULT_OK) {
		printf("Could not read ab data!\n");
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	avb_ops_user_free(ops);
	return 0;
}

AvbABFlowResult ab_set_slot_active(unsigned int *slot_number)
{
	AvbOps* ops;
	AvbIOResult ret = 0;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	debug("set_slot_active\n");
	if (avb_ab_mark_slot_active(ops->ab_ops, *slot_number) != 0) {
		printf("Could not set slot active!\n");
		ret = AVB_AB_FLOW_RESULT_ERROR_IO;
	}

	avb_ops_user_free(ops);
	return ret;
}

int ab_init_metadata(void)
{
	AvbOps *ops;
	AvbABData ab_data;

	memset(&ab_data, 0, sizeof(AvbABData));
	debug("sizeof(AvbABData) = %d\n", (int)(size_t)sizeof(AvbABData));

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return -1;
	}

	avb_ab_data_init(&ab_data);
	if (ops->ab_ops->write_ab_metadata(ops->ab_ops, &ab_data) != 0) {
		printf("do_avb_init_ab_metadata error!\n");
		avb_ops_user_free(ops);
		return -1;
	}

	printf("Initialize ab data to misc partition success.\n");
	avb_ops_user_free(ops);

	return 0;
}
