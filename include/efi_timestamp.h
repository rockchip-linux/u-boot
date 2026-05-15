/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * EFI_TIMESTAMP_PROTOCOL
 *
 * Copyright (c) 2026 The Android Open Source Project
 */

#if !defined _EFI_TIMESTAMP_H_
#define _EFI_TIMESTAMP_H_

#include <efi.h>
#include <efi_api.h>

/**
 * struct efi_timestamp_properties - timestamp properties
 *
 * @frequency:	frequency of the timestamp counter in Hz
 * @end_value:	value that the timestamp counter ends with immediately before it rolls over
 */
struct efi_timestamp_properties {
	u64 frequency;
	u64 end_value;
};

/**
 * struct efi_timestamp_protocol - timestamp protocol
 *
 * @get_timestamp:	retrieve the current value of a 64-bit free-running timestamp counter
 * @get_properties:	retrieve the properties of the timestamp counter
 */
struct efi_timestamp_protocol {
	u64 (EFIAPI *get_timestamp)(void);
	efi_status_t (EFIAPI *get_properties)(struct efi_timestamp_properties *properties);
};

#endif /* _EFI_TIMESTAMP_H_ */
