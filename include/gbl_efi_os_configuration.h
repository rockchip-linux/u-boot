/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2024 The Android Open Source Project
 */

/*
 * Original definition:
 * https://cs.android.com/android/platform/superproject/main/+/124321be6391247d71ee8737c3a49bd36213b3de:bootable/libbootloader/gbl/libefi_types/defs/protocols/gbl_efi_os_configuration_protocol.h
 *
 * Documentation:
 * https://cs.android.com/android/platform/superproject/main/+/124321be6391247d71ee8737c3a49bd36213b3de:bootable/libbootloader/gbl/docs/gbl_os_configuration_protocol.md
 *
 * Warning: API is UNSTABLE
 */

#ifndef __GBL_EFI_OS_CONFIG_H__
#define __GBL_EFI_OS_CONFIG_H__

#include <efi_api.h>

#define GBL_EFI_OS_CONFIGURATION_PROTOCOL_REVISION 0x00010000

enum gbl_efi_device_tree_type {
	GBL_EFI_DEVICE_TREE_TYPE_DEVICE_TREE,
	GBL_EFI_DEVICE_TREE_TYPE_OVERLAY,
	GBL_EFI_DEVICE_TREE_TYPE_PVM_DA_OVERLAY,
};

enum gbl_efi_device_tree_source {
	GBL_EFI_DEVICE_TREE_SOURCE_BOOT,
	GBL_EFI_DEVICE_TREE_SOURCE_VENDOR_BOOT,
	GBL_EFI_DEVICE_TREE_SOURCE_DTBO,
	GBL_EFI_DEVICE_TREE_SOURCE_DTB,
};

struct gbl_efi_device_tree_metadata {
	u32 source;
	u32 type;
	u32 id;
	u32 rev;
	size_t custom_size;
	const u8 *custom;
};

struct gbl_efi_verified_device_tree {
	struct gbl_efi_device_tree_metadata metadata;
	const void *device_tree;
	u8 selected;
};

struct gbl_efi_os_configuration_protocol {
	u64 revision;

	efi_status_t(EFIAPI *fixup_bootconfig)(
		struct gbl_efi_os_configuration_protocol *self,
		/* in */ size_t bootconfig_size,
		/* in */ const char *bootconfig,
		/* in-out */ size_t *fixup_buffer_size, /* out */ char *fixup);

	efi_status_t(EFIAPI *select_device_trees)(
		struct gbl_efi_os_configuration_protocol *self,
		/* in */ size_t num_device_trees,
		/* in-out */ struct gbl_efi_verified_device_tree *device_trees);

	efi_status_t(EFIAPI *select_fit_configuration)(
		struct gbl_efi_os_configuration_protocol *self,
		/* in */ size_t fit_size, /* in */ const u8 *fit,
		/* in */ size_t metadata_size, /* in */ const u8 *metadata,
		/* out */ size_t *selected_configuration_offset);
};

extern const efi_guid_t gbl_efi_os_config_guid;

efi_status_t gbl_efi_os_config_register(void);

#endif /* __GBL_EFI_OS_CONFIG_H__ */
