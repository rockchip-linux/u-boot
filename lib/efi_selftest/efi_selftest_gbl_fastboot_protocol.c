/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#include <efi.h>
#include <efi_api.h>
#include <gbl_efi_fastboot_protocol.h>
#include <efi_selftest.h>
#include <string.h>

static struct efi_boot_services *boot_services;
static struct gbl_efi_fastboot_protocol *protocol;

void cb_noop(void *context, size_t num_args, const char *const *args,
	     const char *val)
{
}

static int test_getvar(void)
{
	efi_status_t res;
	const size_t BUF_SIZE = 100;
	size_t bufsize = BUF_SIZE;
	char buf[BUF_SIZE];

	const char *args[] = { "not_found" };
	res = protocol->get_var(protocol, ARRAY_SIZE(args), args, &bufsize,
				buf);
	if (res != EFI_NOT_FOUND) {
		efi_st_error("Call to get_var expected to return NOT_FOUND\n");
		return EFI_ST_FAILURE;
	}

	res = protocol->get_var(protocol, 0, NULL, &bufsize, buf);
	if (res != EFI_INVALID_PARAMETER) {
		efi_st_error(
			"Call to get_var expected to return EFI_INVALID_PARAMETER\n");
		return EFI_ST_FAILURE;
	}

	res = protocol->get_var(protocol, ARRAY_SIZE(args), args, &bufsize,
				NULL);
	if (res != EFI_INVALID_PARAMETER) {
		efi_st_error(
			"Call to get_var expected to return EFI_INVALID_PARAMETER\n");
		return EFI_ST_FAILURE;
	}

	res = protocol->get_var(protocol, ARRAY_SIZE(args), args, NULL, buf);
	if (res != EFI_INVALID_PARAMETER) {
		efi_st_error(
			"Call to get_var expected to return EFI_INVALID_PARAMETER\n");
		return EFI_ST_FAILURE;
	}

	res = protocol->get_var_all(protocol, NULL, cb_noop);
	if (res != EFI_SUCCESS) {
		efi_st_error("Call to get_var_all failed unexpectedly\n");
		return EFI_ST_FAILURE;
	}

	return EFI_ST_SUCCESS;
}

static int setup(const efi_handle_t handle,
		 const struct efi_system_table *systable)
{
	boot_services = systable->boottime;
	efi_status_t res = boot_services->locate_protocol(
		&gbl_efi_fastboot_guid, NULL, (void **)&protocol);
	if (res != EFI_SUCCESS) {
		protocol = NULL;
		efi_st_error("Failed to locate GBL Fastboot protocol\n");
		return EFI_ST_FAILURE;
	}

	return EFI_ST_SUCCESS;
}

static int execute(void)
{
	int res;
	res = test_getvar();
	if (res != EFI_ST_SUCCESS) {
		return res;
	}
	return EFI_ST_SUCCESS;
}

static int teardown(void)
{
	return EFI_ST_SUCCESS;
}

EFI_UNIT_TEST(gbl_fastboot) = {
	.name = "GBL Fastboot Protocol",
	.phase = EFI_EXECUTE_BEFORE_BOOTTIME_EXIT,
	.setup = setup,
	.execute = execute,
	.teardown = teardown,
};
