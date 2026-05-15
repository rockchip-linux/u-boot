// SPDX-License-Identifier: BSD-2-Clause
/*
 * efi_selftest_timestamp
 *
 * Copyright (c) 2026 The Android Open Source Project
 *
 * Test the timestamp protocol.
 */

#include <efi_selftest.h>
#include <efi_timestamp.h>

static struct efi_boot_services *boottime;
static efi_guid_t efi_timestamp_guid = EFI_TIMESTAMP_PROTOCOL_GUID;

/*
 * Setup unit test.
 *
 * @handle:	handle of the loaded image
 * @systable:	system table
 * Return:	EFI_ST_SUCCESS for success
 */
static int setup(const efi_handle_t handle,
		 const struct efi_system_table *systable)
{
	boottime = systable->boottime;
	return EFI_ST_SUCCESS;
}

/*
 * Execute unit test.
 *
 * Retrieve timestamp properties.
 * Retrieve two timestamp values and ensure they are monotonic.
 *
 * Return:	EFI_ST_SUCCESS for success
 */
static int execute(void)
{
	efi_status_t ret;
	struct efi_timestamp_protocol *timestamp;
	struct efi_timestamp_properties properties;
	u64 ts1, ts2;

	/* Get timestamp protocol */
	ret = boottime->locate_protocol(&efi_timestamp_guid, NULL, (void **)&timestamp);
	if (ret != EFI_SUCCESS) {
		efi_st_error("Timestamp protocol not available\n");
		return EFI_ST_FAILURE;
	}

	ret = timestamp->get_properties(&properties);
	if (ret != EFI_SUCCESS) {
		efi_st_error("Could not retrieve timestamp properties\n");
		return EFI_ST_FAILURE;
	}

	if (properties.frequency == 0) {
		efi_st_error("Timestamp frequency is zero\n");
		return EFI_ST_FAILURE;
	}

	ts1 = timestamp->get_timestamp();
	/* Wait a bit (1ms) */
	boottime->stall(1000);
	ts2 = timestamp->get_timestamp();

	if (ts2 < ts1) {
		efi_st_error("Timestamp is not monotonic\n");
		return EFI_ST_FAILURE;
	}

	return EFI_ST_SUCCESS;
}

EFI_UNIT_TEST(timestamp) = {
	.name = "timestamp protocol",
	.phase = EFI_EXECUTE_BEFORE_BOOTTIME_EXIT,
	.setup = setup,
	.execute = execute,
};
