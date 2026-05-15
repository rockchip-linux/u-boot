// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2026 The Android Open Source Project
 */

#define LOG_CATEGORY LOGC_EFI

#include <efi_loader.h>
#include <efi_timestamp.h>
#include <log.h>
#include <time.h>

const efi_guid_t efi_guid_timestamp_protocol = EFI_TIMESTAMP_PROTOCOL_GUID;

/**
 * efi_timestamp_get() - get current timestamp
 *
 * This function implements the GetTimestamp service of the EFI timestamp
 * protocol. See the UEFI spec for details.
 *
 * Return:	current timestamp value
 */
static u64 EFIAPI efi_timestamp_get(void)
{
	EFI_ENTRY();

	return EFI_EXIT(get_ticks());
}

/**
 * efi_timestamp_get_properties() - get timestamp properties
 *
 * This function implements the GetProperties service of the EFI timestamp
 * protocol. See the UEFI spec for details.
 *
 * @properties:		timestamp properties
 * Return:		status code
 */
static efi_status_t EFIAPI
efi_timestamp_get_properties(struct efi_timestamp_properties *properties)
{
	EFI_ENTRY("%p", properties);

	if (!properties)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	properties->frequency = get_tbclk();
	properties->end_value = 0xffffffffffffffffULL;

	return EFI_EXIT(EFI_SUCCESS);
}

static const struct efi_timestamp_protocol efi_timestamp_protocol = {
	.get_timestamp = efi_timestamp_get,
	.get_properties = efi_timestamp_get_properties,
};

/**
 * efi_timestamp_register() - register EFI_TIMESTAMP_PROTOCOL
 *
 * Return:	status code
 */
efi_status_t efi_timestamp_register(void)
{
	efi_status_t ret;

	ret = efi_add_protocol(efi_root, &efi_guid_timestamp_protocol,
			       (void *)&efi_timestamp_protocol);
	if (ret != EFI_SUCCESS)
		log_err("Cannot install EFI_TIMESTAMP_PROTOCOL\n");

	return ret;
}
