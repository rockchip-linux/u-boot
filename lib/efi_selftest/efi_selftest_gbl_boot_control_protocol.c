// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2024 The Android Open Source Project
 */

#include <blk.h>
#include <efi_api.h>
#include <efi.h>
#include <android_avb/libavb_ab.h>
#include <gbl_efi_boot_control_protocol.h>
#include <efi_selftest.h>
#include <part.h>
#include <stdlib.h>
#include <string.h>

static struct efi_boot_services *boot_services;
static struct gbl_efi_boot_control_protocol *protocol;

static int setup(const efi_handle_t handle,
		 const struct efi_system_table *systable)
{
	boot_services = systable->boottime;

	efi_status_t res = boot_services->locate_protocol(&gbl_efi_boot_control_guid,
							  NULL, (void **)&protocol);
	if (res != EFI_SUCCESS) {
		protocol = NULL;
		efi_st_error("Failed to locate GBL AB boot protocol\n");
		return EFI_ST_FAILURE;
	}

	return EFI_ST_SUCCESS;
}

static int execute(void)
{
	u8 slot_count;
	efi_status_t res = protocol->get_slot_count(protocol, &slot_count);
	if (res != EFI_SUCCESS) {
		efi_st_error("Failed to get slot count: %lu\n", res);
		return EFI_ST_FAILURE;
	}

	if (slot_count != 2) {
		efi_st_error("Unexpected slot count: %u\n", slot_count);
		return EFI_ST_FAILURE;
	}

	struct gbl_efi_slot_info slot;
	res = protocol->get_current_slot(protocol, &slot);
	if (res != EFI_SUCCESS) {
		efi_st_error("Failed to get current slot: %lu\n", res);
		return EFI_ST_FAILURE;
	}

	/* Quick checks on the current slot */
	if (slot.suffix != 'a' || slot.priority != 15 || slot.successful != 0 ||
	    slot.remaining_tries != 7 ||
	    slot.unbootable_reason != GBL_EFI_UNBOOTABLE_REASON_UNKNOWN_REASON) {
		efi_st_error("Unexpected active slot:\n");
		efi_st_error("suffix = %u\n", slot.suffix);
		efi_st_error("priority = %u\n", slot.priority);
		efi_st_error("successful = %u\n", slot.successful);
		efi_st_error("remaining_tries = %u\n", slot.remaining_tries);
		efi_st_error("unbootable_reason = %u\n", slot.unbootable_reason);
	}

	for (int i = 0; i < slot_count; i++) {
		res = protocol->get_slot_info(protocol, i, &slot);
		if (res != EFI_SUCCESS) {
			efi_st_error("Could not get slot at index: %d, res = %lu\n",
				     i, res);
			return EFI_ST_FAILURE;
		}
		if (slot.suffix != (u32)('a' + i)) {
			efi_st_error("Unexpected slot suffix at index %d: %u\n", i, slot.suffix);
			return EFI_ST_FAILURE;
		}
	}

	res = protocol->set_active_slot(protocol, 1);
	if (res != EFI_SUCCESS) {
		efi_st_error("Failed to set active slot: %lu\n", res);
		return EFI_ST_FAILURE;
	}

	res = protocol->get_current_slot(protocol, &slot);
	if (res != EFI_SUCCESS) {
		efi_st_error("Failed to get current slot after setting active: %lu\n",
			     res);
		return EFI_ST_FAILURE;
	}

	if (slot.suffix != 'b') {
		efi_st_error("set_active_slot did not change current_slot\n");
		return EFI_ST_FAILURE;
	}

	/* Instead of rebooting to make sure changes persist,
	 * just cheat and read them straight off the disk
	 */
	const char *ab_partition_name = "misc";
	struct disk_partition ab_partition;
	struct blk_desc *block_device = blk_get_dev("virtio", 0);

	if (!block_device) {
		efi_st_error("Failed to get backing block device\n");
		return EFI_ST_FAILURE;
	}

	u8 *buffer = calloc(1, block_device->blksz);

	if (!buffer) {
		efi_st_error("Out of resources\n");
		return EFI_ST_FAILURE;
	}

	if (part_get_info_by_name(block_device, ab_partition_name,
				  &ab_partition) < 1) {
		efi_st_error("Couldn't find partition: %s\n",
			     ab_partition_name);
		free(buffer);
		return EFI_ST_FAILURE;
	}

	if (blk_dread(block_device,
		      ab_partition.start + (2048 / ab_partition.blksz),
		      1,
		      buffer) != 1) {
		efi_st_error("Couldn't read from disk\n");
		free(buffer);
		return EFI_ST_FAILURE;
	}

	int cmp = EFI_ST_SUCCESS;
	AvbABData expected_ab;
	AvbABData expected_disk_ab;

	avb_ab_data_init(&expected_ab);
	expected_ab.slots[0].priority = AVB_AB_MAX_PRIORITY - 1;
	expected_ab.slots[1].priority = AVB_AB_MAX_PRIORITY;
	avb_ab_data_update_crc_and_byteswap(&expected_ab, &expected_disk_ab);

	if (memcmp(&expected_disk_ab,
		   buffer + (2048 % ab_partition.blksz),
		   sizeof(expected_disk_ab)) != 0) {
		efi_st_error("Slot metadata block differs from disk\n");
		cmp = EFI_ST_FAILURE;
	}

	free(buffer);
	return cmp;
}

static int teardown(void)
{
	return EFI_ST_SUCCESS;
}

EFI_UNIT_TEST(gbl_ab) = {
	.name = "GBL AB Boot Slot Protocol",
	.phase = EFI_EXECUTE_BEFORE_BOOTTIME_EXIT,
	.setup = setup,
	.execute = execute,
	.teardown = teardown,
};
