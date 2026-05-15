/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2024 The Android Open Source Project
 */

#ifndef __GBL_EFI_BOOT_CONTROL_H__
#define __GBL_EFI_BOOT_CONTROL_H__

#include <efi_api.h>

#define GBL_EFI_BOOT_CONTROL_REVISION 0x00010000

enum gbl_efi_unbootable_reason {
	GBL_EFI_UNBOOTABLE_REASON_UNKNOWN_REASON = 0,
	GBL_EFI_UNBOOTABLE_REASON_NO_MORE_TRIES,
	GBL_EFI_UNBOOTABLE_REASON_SYSTEM_UPDATE,
	GBL_EFI_UNBOOTABLE_REASON_USER_REQUESTED,
	GBL_EFI_UNBOOTABLE_REASON_VERIFICATION_FAILURE,
};

enum gbl_efi_one_shot_boot_mode {
	GBL_EFI_ONE_SHOT_BOOT_MODE_NONE = 0,
	GBL_EFI_ONE_SHOT_BOOT_MODE_BOOTLOADER,
	GBL_EFI_ONE_SHOT_BOOT_MODE_RECOVERY,
};

struct gbl_efi_slot_info {
	/* One UTF-8 encoded single character */
	u32 suffix;
	/* Any value other than those explicitly enumerated in EFI_UNBOOTABLE_REASON
	 will be interpreted as UNKNOWN_REASON. */
	u8 unbootable_reason;
	u8 priority;
	/* Number of remaining tries to attempt to boot the slot */
	u8 remaining_tries;
	/* Value of 1 if slot has successfully booted. */
	u8 successful;
};

struct gbl_efi_loaded_os {
	size_t kernel_size;
	efi_physical_addr_t kernel;
	size_t ramdisk_size;
	efi_physical_addr_t ramdisk;
	size_t device_tree_size;
	efi_physical_addr_t device_tree;
	u64 reserved[8];
};

extern const efi_guid_t gbl_efi_boot_control_guid;

struct gbl_efi_boot_control_protocol {
	u64 revision;
	/* Slot metadata query methods */
	efi_status_t(EFIAPI *get_slot_count)(
		struct gbl_efi_boot_control_protocol *self,
		/* out */ u8 *slot_count);
	efi_status_t(EFIAPI *get_slot_info)(
		/* in */ struct gbl_efi_boot_control_protocol *self,
		/* in */ u8 idx,
		/* out */ struct gbl_efi_slot_info *info);
	efi_status_t(EFIAPI *get_current_slot)(
		/* in */ struct gbl_efi_boot_control_protocol *self,
		/* out */ struct gbl_efi_slot_info *info);
	/* Slot metadata manipulation methods */
	efi_status_t(EFIAPI *set_active_slot)(
		/* in */ struct gbl_efi_boot_control_protocol *self,
		/* in */ u8 idx);
	/* Boot control methods */
	efi_status_t(EFIAPI *get_one_shot_boot_mode)(
		struct gbl_efi_boot_control_protocol *self,
		/* out */ enum gbl_efi_one_shot_boot_mode *mode);
	efi_status_t(EFIAPI *handle_loaded_os)(
		struct gbl_efi_boot_control_protocol *self,
		/* in */ const struct gbl_efi_loaded_os *os);
};

efi_status_t gbl_efi_boot_control_register(void);

#endif /* __GBL_EFI_BOOT_CONTROL_H__ */
