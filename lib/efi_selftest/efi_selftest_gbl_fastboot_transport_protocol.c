/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#include <efi.h>
#include <efi_api.h>
#include <gbl_efi_fastboot_transport.h>
#include <gbl_efi_fastboot_transport_interactive_serial.h>
#include <efi_selftest.h>
#include <string.h>

static struct gbl_efi_fastboot_transport_protocol *protocol;

static int test_interactive_serial(void)
{
	efi_status_t res;

	res = protocol->start(protocol);
	if (res != EFI_SUCCESS) {
		efi_st_error("Call to start() failed unexpectedly\n");
		return EFI_ST_FAILURE;
	}

	res = protocol->flush(protocol);
	if (res != EFI_SUCCESS) {
		efi_st_error("Call to flush() failed unexpectedly\n");
		return EFI_ST_FAILURE;
	}

	char buf[32];
	size_t bufsize = sizeof(buf);
	const gbl_efi_fastboot_rx_mode mode =
		GBL_EFI_FASTBOOT_RX_MODE_SINGLE_PACKET;
	res = protocol->receive(protocol, &bufsize, NULL, mode);
	if (res != EFI_INVALID_PARAMETER) {
		efi_st_error(
			"Call to receive() should have failed with NULL buffer\n");
		return EFI_ST_FAILURE;
	}

	res = protocol->receive(protocol, NULL, buf, mode);
	if (res != EFI_INVALID_PARAMETER) {
		efi_st_error(
			"Call to receive() should have failed with NULL bufsize\n");
		return EFI_ST_FAILURE;
	}

	res = protocol->receive(protocol, &bufsize, buf, mode);
	if (res != EFI_SUCCESS) {
		efi_st_error("Call to receive() failed unexpectedly\n");
		return EFI_ST_FAILURE;
	}

	if (0 != strcmp(protocol->description, "serial-interactive")) {
		efi_st_error("Description string '%s' is bad\n",
			     protocol->description);
		return EFI_ST_FAILURE;
	}

	res = protocol->stop(protocol);
	if (res != EFI_SUCCESS) {
		efi_st_error("Call to stop() failed unexpectedly\n");
		return EFI_ST_FAILURE;
	}

	return EFI_ST_SUCCESS;
}

static int setup(const efi_handle_t handle_in,
		 const struct efi_system_table *systable)
{
	int i;
	efi_handle_t *handle;
	efi_handle_t *handles = NULL;
	efi_uintn_t no_handles;

	efi_status_t ret = EFI_CALL(efi_locate_handle_buffer(
		BY_PROTOCOL, &gbl_efi_fastboot_transport_guid, NULL,
		&no_handles, (efi_handle_t **)&handles));
	if (ret != EFI_SUCCESS)
		return EFI_ST_FAILURE;

	protocol = NULL;
	for (i = 0, handle = handles; i < no_handles; i++, handle++) {
		struct efi_handler *cur_handler;

		ret = efi_search_protocol(*handle,
					  &gbl_efi_fastboot_transport_guid,
					  &cur_handler);
		if (ret != EFI_SUCCESS)
			continue;

		struct gbl_efi_fastboot_transport_protocol *proto =
			cur_handler->protocol_interface;

		if (0 == strcmp(proto->description, "serial-interactive")) {
			protocol = proto;
		}
	}

	efi_free_pool(handles);

	if (protocol == NULL) {
		efi_st_error(
			"'serial-interactive' GBL Fastboot Transport protocol not found\n");
		return EFI_ST_FAILURE;
	}

	return EFI_ST_SUCCESS;
}

static int execute(void)
{
	return test_interactive_serial();
}

static int teardown(void)
{
	return EFI_ST_SUCCESS;
}

EFI_UNIT_TEST(gbl_fastboot_transport) = {
	.name = "GBL Fastboot Transport Protocol",
	.phase = EFI_EXECUTE_BEFORE_BOOTTIME_EXIT,
	.setup = setup,
	.execute = execute,
	.teardown = teardown,
};
