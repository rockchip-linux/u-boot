/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#ifndef __GBL_EFI_FASTBOOT_TRANSPORT_H__
#define __GBL_EFI_FASTBOOT_TRANSPORT_H__

#include <efi.h>
#include <efi_api.h>

#define GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_REVISION 0x00010000

extern const efi_guid_t gbl_efi_fastboot_transport_guid;

typedef enum {
	// Single packet receive mode
	GBL_EFI_FASTBOOT_RX_MODE_SINGLE_PACKET = 0,
	// Fixed length receive mode
	GBL_EFI_FASTBOOT_RX_MODE_FIXED_LENGTH = 1,
} gbl_efi_fastboot_rx_mode;

struct gbl_efi_fastboot_transport_protocol {
	// Revision of the protocol supported.
	u64 revision;
	const char *description;

	efi_status_t(EFIAPI *start)(
		struct gbl_efi_fastboot_transport_protocol *this);
	efi_status_t(EFIAPI *stop)(
		struct gbl_efi_fastboot_transport_protocol *this);
	efi_status_t(EFIAPI *receive)(
		struct gbl_efi_fastboot_transport_protocol *this,
		size_t *bufsize, void *buf, gbl_efi_fastboot_rx_mode mode);
	efi_status_t(EFIAPI *send)(
		struct gbl_efi_fastboot_transport_protocol *this,
		size_t *bufsize, const void *buf);
	efi_status_t(EFIAPI *flush)(
		struct gbl_efi_fastboot_transport_protocol *this);
};

efi_status_t gbl_efi_fastboot_transport_register(void);

#endif /* __GBL_EFI_FASTBOOT_TRANSPORT_H__ */
