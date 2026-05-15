/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2026 The Android Open Source Project
 */

#ifndef __GBL_EFI_AVB_H__
#define __GBL_EFI_AVB_H__

#include <efi.h>
#include <efi_api.h>

#define GBL_EFI_AVB_PROTOCOL_REVISION 0x00010000

typedef u64 gbl_efi_avb_partition_flags;
#define GBL_EFI_AVB_PARTITION_FLAG_VERIFY (1ULL << 0)
#define GBL_EFI_AVB_PARTITION_FLAG_VERIFY_IF_EXISTS (1ULL << 1)
#define GBL_EFI_AVB_PARTITION_FLAG_FLASH_CRITICAL (1ULL << 2)
#define GBL_EFI_AVB_PARTITION_FLAG_FDR (1ULL << 3)

typedef struct {
	size_t base_name_len;
	char *base_name;
	gbl_efi_avb_partition_flags flags;
} gbl_efi_avb_partition_attributes;

typedef u64 gbl_efi_avb_device_status;
#define GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED (1ULL << 0)
#define GBL_EFI_AVB_DEVICE_STATUS_DM_VERITY_FAILED (1ULL << 1)
#define GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED_CRITICAL (1ULL << 2)
#define GBL_EFI_AVB_DEVICE_STATUS_UNLOCKABLE (1ULL << 3)

typedef enum {
	GBL_EFI_AVB_KEY_VALIDATION_STATUS_INVALID = 0,
	GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID_CUSTOM_KEY,
	GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID,
} gbl_efi_avb_key_validation_status;

typedef u64 gbl_efi_avb_boot_color_flags;
#define GBL_EFI_AVB_BOOT_COLOR_RED (1ULL << 0)
#define GBL_EFI_AVB_BOOT_COLOR_ORANGE (1ULL << 1)
#define GBL_EFI_AVB_BOOT_COLOR_YELLOW (1ULL << 2)
#define GBL_EFI_AVB_BOOT_COLOR_GREEN (1ULL << 3)
#define GBL_EFI_AVB_BOOT_COLOR_RED_EIO (1ULL << 4)

typedef struct {
	const char *base_partition_name;
	const char *key;
	size_t value_size;
	const u8 *value;
} gbl_efi_avb_property;

typedef struct {
	const char *base_name;
	size_t data_size;
	const u8 *data;
} gbl_efi_avb_loaded_partition;

typedef struct {
	gbl_efi_avb_boot_color_flags color_flags;
	const char *digest;
	size_t num_partitions;
	const gbl_efi_avb_loaded_partition *partitions;
	size_t num_properties;
	const gbl_efi_avb_property *properties;
	u32 reserved[8];
} gbl_efi_avb_verification_result;

typedef enum {
	GBL_EFI_AVB_LOCK_TYPE_DEVICE = 0,
	GBL_EFI_AVB_LOCK_TYPE_CRITICAL,
} gbl_efi_avb_lock_type;

typedef enum {
	GBL_EFI_AVB_LOCK_STATE_UNLOCKED = 0,
	GBL_EFI_AVB_LOCK_STATE_LOCKED,
} gbl_efi_avb_lock_state;

extern const efi_guid_t gbl_efi_avb_guid;

struct gbl_efi_avb_protocol {
	u64 revision;

	efi_status_t(EFIAPI *read_partition_attributes)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in out */ size_t *num_partitions,
		/* in out */ gbl_efi_avb_partition_attributes *partitions);

	efi_status_t(EFIAPI *read_device_status)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* out */ gbl_efi_avb_device_status *status_flags);

	efi_status_t(EFIAPI *validate_vbmeta_public_key)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in */ size_t public_key_length,
		/* in */ const u8 *public_key_data,
		/* in */ size_t public_key_metadata_length,
		/* in */ const u8 *public_key_metadata,
		/* out */ gbl_efi_avb_key_validation_status *validation_status);

	efi_status_t(EFIAPI *read_rollback_index)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in */ size_t index_location,
		/* out */ u64 *rollback_index);

	efi_status_t(EFIAPI *write_rollback_index)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in */ size_t index_location,
		/* in */ u64 rollback_index);

	efi_status_t(EFIAPI *read_persistent_value)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in */ const char *name,
		/* in out */ size_t *value_size,
		/* out */ u8 *value);

	efi_status_t(EFIAPI *write_persistent_value)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in */ const char *name,
		/* in */ size_t value_size,
		/* in */ const u8 *value);

	efi_status_t(EFIAPI *handle_verification_result)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in */ const gbl_efi_avb_verification_result *result);

	efi_status_t(EFIAPI *write_lock_state)(
		/* in */ struct gbl_efi_avb_protocol *self,
		/* in */ gbl_efi_avb_lock_type type,
		/* in */ gbl_efi_avb_lock_state state);

	efi_status_t(EFIAPI *factory_data_reset)(
		/* in */ struct gbl_efi_avb_protocol *self);
};

efi_status_t gbl_efi_avb_register(void);

#endif /* __GBL_EFI_AVB_H__ */
