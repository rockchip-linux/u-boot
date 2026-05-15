/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2026 The Android Open Source Project
 */

#include <efi.h>
#include <efi_api.h>
#include <gbl_efi_avb_protocol.h>
#include <efi_loader.h>
#include <log.h>
#include <android_bootloader_oemlock.h>
#include <avb_verify.h>

const efi_guid_t gbl_efi_avb_guid = GBL_EFI_AVB_PROTOCOL_GUID;
static struct gbl_efi_avb_protocol gbl_efi_avb_proto;

static efi_status_t EFIAPI read_partition_attributes(
	struct gbl_efi_avb_protocol *this, size_t *num_partitions,
	gbl_efi_avb_partition_attributes *partitions)
{
	EFI_ENTRY("%p, %p, %p", this, num_partitions, partitions);
	if (this != &gbl_efi_avb_proto || num_partitions == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	*num_partitions = 0;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
read_device_status(struct gbl_efi_avb_protocol *this,
		   gbl_efi_avb_device_status *status_flags)
{
	EFI_ENTRY("%p, %p", this, status_flags);
	if (this != &gbl_efi_avb_proto || status_flags == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	*status_flags = 0;

	int locked = oemlock_is_locked();
	if (locked == 0) {
		*status_flags |= GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED;
	}

	int allowed = oemlock_is_allowed();
	if (allowed == 1) {
		*status_flags |= GBL_EFI_AVB_DEVICE_STATUS_UNLOCKABLE;
	}

	// TODO(b/507132906): Support GBL_EFI_AVB_DEVICE_STATUS_DM_VERITY_FAILED when CF handles DM verity errors.
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI validate_vbmeta_public_key(
	struct gbl_efi_avb_protocol *this, size_t public_key_length,
	const u8 *public_key_data, size_t public_key_metadata_length,
	const u8 *public_key_metadata,
	gbl_efi_avb_key_validation_status *validation_status)
{
	EFI_ENTRY("%p, %zu, %p, %zu, %p, %p", this, public_key_length,
		  public_key_data, public_key_metadata_length,
		  public_key_metadata, validation_status);
	if (this != &gbl_efi_avb_proto || public_key_data == NULL ||
	    validation_status == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (avb_pubkey_is_trusted(public_key_data, public_key_length) ==
	    CMD_RET_SUCCESS) {
		*validation_status = GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID;
	} else {
		*validation_status = GBL_EFI_AVB_KEY_VALIDATION_STATUS_INVALID;
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI read_rollback_index(struct gbl_efi_avb_protocol *this,
					       size_t index_location,
					       u64 *rollback_index)
{
	EFI_ENTRY("%p, %zu, %p", this, index_location, rollback_index);
	if (this != &gbl_efi_avb_proto || rollback_index == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	// TODO(b/507133814): Cuttlefish does not yet support rollback protection. This is a temporary implementation for CF to boot with GBL
	*rollback_index = 0;
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
write_rollback_index(struct gbl_efi_avb_protocol *this, size_t index_location,
		     u64 rollback_index)
{
	EFI_ENTRY("%p, %zu, %llu", this, index_location, rollback_index);
	if (this != &gbl_efi_avb_proto) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	// TODO(b/507133814): Cuttlefish does not yet support rollback protection. This is a mock implementation.
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
read_persistent_value(struct gbl_efi_avb_protocol *this, const char *name,
		      size_t *value_size, u8 *value)
{
	EFI_ENTRY("%p, %s, %p, %p", this, name, value_size, value);
	if (this != &gbl_efi_avb_proto || name == NULL || value_size == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	// TODO(b/507132906): Cuttlefish does not yet support persistent values.
	// Return NOT_FOUND so that GBL/libavb handles it as "value not set" instead of failing.
	return EFI_EXIT(EFI_NOT_FOUND);
}

static efi_status_t EFIAPI
write_persistent_value(struct gbl_efi_avb_protocol *this, const char *name,
		       size_t value_size, const u8 *value)
{
	EFI_ENTRY("%p, %s, %zu, %p", this, name, value_size, value);
	if (this != &gbl_efi_avb_proto || name == NULL ||
	    (value_size > 0 && value == NULL)) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	// TODO(b/507132906): Cuttlefish does not yet support persistent values.
	return EFI_EXIT(EFI_UNSUPPORTED);
}

static efi_status_t EFIAPI
handle_verification_result(struct gbl_efi_avb_protocol *this,
			   const gbl_efi_avb_verification_result *result)
{
	EFI_ENTRY("%p, %p", this, result);
	if (this != &gbl_efi_avb_proto || result == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI write_lock_state(struct gbl_efi_avb_protocol *this,
					    gbl_efi_avb_lock_type type,
					    gbl_efi_avb_lock_state state)
{
	EFI_ENTRY("%p, %u, %u", this, type, state);
	if (this != &gbl_efi_avb_proto) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (type == GBL_EFI_AVB_LOCK_TYPE_CRITICAL) {
		return EFI_EXIT(EFI_UNSUPPORTED);
	}

	if (type != GBL_EFI_AVB_LOCK_TYPE_DEVICE) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (state == GBL_EFI_AVB_LOCK_STATE_UNLOCKED) {
		int allowed = oemlock_is_allowed();
		if (allowed < 0) {
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
		if (allowed == 0) {
			return EFI_EXIT(EFI_ACCESS_DENIED);
		}
		int ret = oemlock_set_locked(false);
		if (ret < 0) {
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
	} else if (state == GBL_EFI_AVB_LOCK_STATE_LOCKED) {
		int ret = oemlock_set_locked(true);
		if (ret < 0) {
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
	} else {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI factory_data_reset(struct gbl_efi_avb_protocol *this)
{
	EFI_ENTRY("%p", this);
	if (this != &gbl_efi_avb_proto) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	// TODO(b/507134176): Investigate whether any secure world data requires clearance during a Cuttlefish factory reset.
	return EFI_EXIT(EFI_SUCCESS);
}

static struct gbl_efi_avb_protocol gbl_efi_avb_proto = {
	.revision = GBL_EFI_AVB_PROTOCOL_REVISION,
	.read_partition_attributes = read_partition_attributes,
	.read_device_status = read_device_status,
	.validate_vbmeta_public_key = validate_vbmeta_public_key,
	.read_rollback_index = read_rollback_index,
	.write_rollback_index = write_rollback_index,
	.read_persistent_value = read_persistent_value,
	.write_persistent_value = write_persistent_value,
	.handle_verification_result = handle_verification_result,
	.write_lock_state = write_lock_state,
	.factory_data_reset = factory_data_reset,
};

efi_status_t gbl_efi_avb_register(void)
{
	efi_status_t ret = efi_add_protocol(efi_root, &gbl_efi_avb_guid,
					    &gbl_efi_avb_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install GBL_EFI_AVB_PROTOCOL: 0x%lx\n", ret);
	}

	return ret;
}
