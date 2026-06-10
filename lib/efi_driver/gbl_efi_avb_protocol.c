/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2026 The Android Open Source Project
 */

#include <efi.h>
#include <efi_api.h>
#include <android_avb/avb.h>
#include <gbl_efi_avb_protocol.h>
#include <charset.h>
#include <efi_loader.h>
#include <efi_variable.h>
#include <log.h>
#include <malloc.h>
#include <android_image.h>
#include <android_avb/libavb_ab.h>
#include <android_avb/avb_ops_user.h>
#include <avb_verify.h>

const efi_guid_t gbl_efi_avb_guid = GBL_EFI_AVB_PROTOCOL_GUID;
static struct gbl_efi_avb_protocol gbl_efi_avb_proto;
static gbl_efi_avb_partition_attributes partition_attributes[] = {
	{
		.base_name_len = sizeof(ANDROID_PARTITION_RESOURCE) - 1,
		.base_name = ANDROID_PARTITION_RESOURCE,
		.flags = GBL_EFI_AVB_PARTITION_FLAG_VERIFY_IF_EXISTS,
	},
};

static u16 *persistent_name_to_efi_name(const char *name)
{
	u16 *name16, *pos;
	size_t size;

	size = sizeof(*name16) * (utf8_utf16_strlen(name) + 1);
	name16 = calloc(1, size);
	if (!name16)
		return NULL;

	pos = name16;
	utf8_utf16_strcpy(&pos, name);

	return name16;
}

static efi_status_t EFIAPI read_partition_attributes(
	struct gbl_efi_avb_protocol *this, size_t *num_partitions,
	gbl_efi_avb_partition_attributes *partitions)
{
	size_t required_num_partitions = ARRAY_SIZE(partition_attributes);

	EFI_ENTRY("%p, %p, %p", this, num_partitions, partitions);
	if (this != &gbl_efi_avb_proto || num_partitions == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (*num_partitions < required_num_partitions || partitions == NULL) {
		*num_partitions = required_num_partitions;
		return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
	}

	memcpy(partitions, partition_attributes, sizeof(partition_attributes));
	*num_partitions = required_num_partitions;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
read_device_status(struct gbl_efi_avb_protocol *this,
		   gbl_efi_avb_device_status *status_flags)
{
	uint8_t is_unlock_state;

	EFI_ENTRY("%p, %p", this, status_flags);
	if (this != &gbl_efi_avb_proto || status_flags == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	*status_flags = 0;
	if (avb_read_lock_state(&is_unlock_state))
		return EFI_EXIT(EFI_DEVICE_ERROR);

	if (is_unlock_state & LOCK_MASK)
		*status_flags |= GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED;

	if (!(is_unlock_state & DISABLE_UNLOCK_MASK))
		*status_flags |= GBL_EFI_AVB_DEVICE_STATUS_UNLOCKABLE;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI validate_vbmeta_public_key(
	struct gbl_efi_avb_protocol *this, size_t public_key_length,
	const u8 *public_key_data, size_t public_key_metadata_length,
	const u8 *public_key_metadata,
	gbl_efi_avb_key_validation_status *validation_status)
{
	AvbOps *ops;
	bool trusted;
	AvbIOResult ret;

	EFI_ENTRY("%p, %zu, %p, %zu, %p, %p", this, public_key_length,
		  public_key_data, public_key_metadata_length,
		  public_key_metadata, validation_status);
	if (this != &gbl_efi_avb_proto || public_key_data == NULL ||
	    validation_status == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	ops = avb_ops_user_new();
	if (!ops)
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);

	ret = ops->validate_vbmeta_public_key(ops, public_key_data,
					      public_key_length,
					      public_key_metadata,
					      public_key_metadata_length,
					      &trusted);
	if (ret != AVB_IO_RESULT_OK) {
		avb_ops_user_free(ops);

		switch (ret) {
		case AVB_IO_RESULT_ERROR_OOM:
			return EFI_EXIT(EFI_OUT_OF_RESOURCES);
		case AVB_IO_RESULT_ERROR_NO_SUCH_VALUE:
			return EFI_EXIT(EFI_NOT_FOUND);
		case AVB_IO_RESULT_ERROR_INVALID_VALUE_SIZE:
			return EFI_EXIT(EFI_INVALID_PARAMETER);
		case AVB_IO_RESULT_ERROR_INSUFFICIENT_SPACE:
			return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
		default:
			return EFI_EXIT(EFI_DEVICE_ERROR);
		}
	}

	if (trusted)
		*validation_status = GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID;
	else
		*validation_status = GBL_EFI_AVB_KEY_VALIDATION_STATUS_INVALID;

	avb_ops_user_free(ops);

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI read_rollback_index(struct gbl_efi_avb_protocol *this,
					       size_t index_location,
					       u64 *rollback_index)
{
	AvbOps *ops;
	AvbIOResult ret;

	EFI_ENTRY("%p, %zu, %p", this, index_location, rollback_index);
	if (this != &gbl_efi_avb_proto || rollback_index == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	ops = avb_ops_user_new();
	if (!ops)
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);

	ret = ops->read_rollback_index(ops, index_location, rollback_index);
	avb_ops_user_free(ops);

	return EFI_EXIT(ret == AVB_IO_RESULT_OK ? EFI_SUCCESS : EFI_DEVICE_ERROR);
}

static efi_status_t EFIAPI
write_rollback_index(struct gbl_efi_avb_protocol *this, size_t index_location,
		     u64 rollback_index)
{
	AvbOps *ops;
	AvbIOResult ret;

	EFI_ENTRY("%p, %zu, %llu", this, index_location, rollback_index);
	if (this != &gbl_efi_avb_proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	ops = avb_ops_user_new();
	if (!ops)
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);

	ret = ops->write_rollback_index(ops, index_location, rollback_index);
	avb_ops_user_free(ops);

	return EFI_EXIT(ret == AVB_IO_RESULT_OK ? EFI_SUCCESS : EFI_DEVICE_ERROR);
}

static efi_status_t EFIAPI
read_persistent_value(struct gbl_efi_avb_protocol *this, const char *name,
		      size_t *value_size, u8 *value)
{
	efi_status_t ret;
	efi_uintn_t data_size;
	u16 *name16;

	EFI_ENTRY("%p, %s, %p, %p", this, name, value_size, value);
	if (this != &gbl_efi_avb_proto || name == NULL || value_size == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	name16 = persistent_name_to_efi_name(name);
	if (!name16)
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);

	data_size = *value_size;
	ret = efi_get_variable_int(name16, &gbl_efi_avb_guid, NULL,
				   &data_size, value, NULL);
	*value_size = data_size;
	free(name16);

	return EFI_EXIT(ret);
}

static efi_status_t EFIAPI
write_persistent_value(struct gbl_efi_avb_protocol *this, const char *name,
		       size_t value_size, const u8 *value)
{
	efi_status_t ret;
	u16 *name16;
	const u32 attrs = EFI_VARIABLE_NON_VOLATILE |
			  EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS;

	EFI_ENTRY("%p, %s, %zu, %p", this, name, value_size, value);
	if (this != &gbl_efi_avb_proto || name == NULL ||
	    (value_size > 0 && value == NULL)) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	name16 = persistent_name_to_efi_name(name);
	if (!name16)
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);

	ret = efi_set_variable_int(name16, &gbl_efi_avb_guid, attrs,
				   value_size, value, false);
	free(name16);

	return EFI_EXIT(ret);
}

static efi_status_t EFIAPI
handle_verification_result(struct gbl_efi_avb_protocol *this,
			   const gbl_efi_avb_verification_result *result)
{
	AvbOps *ops;
	AvbABData ab_data, ab_data_orig;
	char verify_state[sizeof(ANDROID_VERIFY_STATE) + 8] = { 0 };
	size_t slot_index_to_boot;
	efi_status_t status = EFI_SUCCESS;

	EFI_ENTRY("%p, %p", this, result);
	if (this != &gbl_efi_avb_proto || result == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (result->color_flags & GBL_EFI_AVB_BOOT_COLOR_GREEN)
		strcpy(verify_state, ANDROID_VERIFY_STATE "green");
	else if (result->color_flags & GBL_EFI_AVB_BOOT_COLOR_YELLOW)
		strcpy(verify_state, ANDROID_VERIFY_STATE "yellow");
	else if (result->color_flags & GBL_EFI_AVB_BOOT_COLOR_ORANGE)
		strcpy(verify_state, ANDROID_VERIFY_STATE "orange");
	else if (result->color_flags &
		 (GBL_EFI_AVB_BOOT_COLOR_RED | GBL_EFI_AVB_BOOT_COLOR_RED_EIO))
		strcpy(verify_state, ANDROID_VERIFY_STATE "red");

	if (verify_state[0] != '\0')
		env_update("bootargs", verify_state);

	if (!(result->color_flags & GBL_EFI_AVB_BOOT_COLOR_RED))
		return EFI_EXIT(EFI_SUCCESS);

	if (result->color_flags & GBL_EFI_AVB_BOOT_COLOR_RED_EIO)
		return EFI_EXIT(EFI_SUCCESS);

	ops = avb_ops_user_new();
	if (!ops)
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);

	if (!ops->ab_ops || load_metadata(ops->ab_ops, &ab_data, &ab_data_orig)) {
		status = EFI_DEVICE_ERROR;
		goto out;
	}

	if (slot_is_bootable(&ab_data.slots[0]) &&
	    slot_is_bootable(&ab_data.slots[1]))
		slot_index_to_boot =
			ab_data.slots[1].priority > ab_data.slots[0].priority;
	else if (slot_is_bootable(&ab_data.slots[0]))
		slot_index_to_boot = 0;
	else if (slot_is_bootable(&ab_data.slots[1]))
		slot_index_to_boot = 1;
	else
		goto out;

	slot_set_unbootable(&ab_data.slots[slot_index_to_boot]);
	if (save_metadata_if_changed(ops->ab_ops, &ab_data, &ab_data_orig))
		status = EFI_DEVICE_ERROR;

out:
	avb_ops_user_free(ops);
	return EFI_EXIT(status);
}

static efi_status_t EFIAPI write_lock_state(struct gbl_efi_avb_protocol *this,
					    gbl_efi_avb_lock_type type,
					    gbl_efi_avb_lock_state state)
{
	AvbOps *ops;
	bool unlocked;
	AvbIOResult ret;

	EFI_ENTRY("%p, %u, %u", this, type, state);
	if (this != &gbl_efi_avb_proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (type == GBL_EFI_AVB_LOCK_TYPE_CRITICAL)
		return EFI_EXIT(EFI_UNSUPPORTED);

	if (type != GBL_EFI_AVB_LOCK_TYPE_DEVICE)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (state != GBL_EFI_AVB_LOCK_STATE_UNLOCKED &&
	    state != GBL_EFI_AVB_LOCK_STATE_LOCKED) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	ops = avb_ops_user_new();
	if (!ops)
		return EFI_EXIT(EFI_OUT_OF_RESOURCES);

	unlocked = state == GBL_EFI_AVB_LOCK_STATE_UNLOCKED;
	ret = ops->write_is_device_unlocked(ops, &unlocked);
	avb_ops_user_free(ops);

	return EFI_EXIT(ret == AVB_IO_RESULT_OK ? EFI_SUCCESS : EFI_DEVICE_ERROR);
}

static efi_status_t EFIAPI factory_data_reset(struct gbl_efi_avb_protocol *this)
{
	EFI_ENTRY("%p", this);
	if (this != &gbl_efi_avb_proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

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
	if (ret != EFI_SUCCESS)
		log_err("Failed to install GBL_EFI_AVB_PROTOCOL: 0x%lx\n", ret);

	return ret;
}
