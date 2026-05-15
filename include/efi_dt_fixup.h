/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * EFI_DT_FIXUP_PROTOCOL
 *
 * Copyright (c) 2020 Heinrich Schuchardt
 */

#include <efi_api.h>

#define EFI_DT_FIXUP_PROTOCOL_REVISION 0x00010000

struct efi_dt_fixup_protocol {
	u64 revision;
	efi_status_t(EFIAPI *fixup)(struct efi_dt_fixup_protocol *this,
				    void *dtb, efi_uintn_t *buffer_size);
};

extern struct efi_dt_fixup_protocol efi_dt_fixup_prot;
extern const efi_guid_t efi_guid_dt_fixup_protocol;
