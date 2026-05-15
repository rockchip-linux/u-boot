/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#include <efi.h>
#include <efi_api.h>
#include <gbl_efi_fastboot_protocol.h>
#include <efi_loader.h>
#include <log.h>

const efi_guid_t gbl_efi_fastboot_guid = GBL_EFI_FASTBOOT_PROTOCOL_GUID;
static struct gbl_efi_fastboot_protocol gbl_efi_fastboot_proto;

// Deliberately simplified fastboot variable representation.
struct fastboot_var {
	// NULL terminated array of strings
	// representing a variable-argument tuple.
	char const *const *const args;
	// String representation of the variable's value.
	char const *const val;
};

// Array of fastboot variables with a NULL sentinel.
static struct fastboot_var vars[] = {
	{ .args = NULL, .val = NULL }, // Sentinel
};

size_t args_len(struct fastboot_var *var)
{
	size_t i = 0;
	while (var->args[i]) {
		i++;
	}
	return i;
}

static bool args_match_var(const char *const *args, size_t num_args,
			   const struct fastboot_var *var)
{
	size_t i;
	for (i = 0; i < num_args && var->args[i]; i++) {
		if (strcmp(args[i], var->args[i])) {
			return false;
		}
	}

	return (i == num_args && !var->args[i]);
}

static efi_status_t EFIAPI get_var(struct gbl_efi_fastboot_protocol *this,
				   size_t num_args, const char *const *fb_args,
				   size_t *bufsize, char *buf)
{
	EFI_ENTRY("%p, %lu, %p, %p, %p", this, num_args, fb_args, bufsize, buf);
	if (this != &gbl_efi_fastboot_proto || fb_args == NULL || buf == NULL ||
	    bufsize == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	for (struct fastboot_var *var = &vars[0]; var->val; var++) {
		if (args_match_var(fb_args, num_args, var)) {
			size_t val_len = strlen(var->val);
			efi_status_t ret;
			if (val_len <= *bufsize) {
				memcpy(buf, var->val, val_len);
				ret = EFI_SUCCESS;
			} else {
				ret = EFI_BUFFER_TOO_SMALL;
			}
			*bufsize = val_len;
			return EFI_EXIT(ret);
		}
	}

	return EFI_EXIT(EFI_NOT_FOUND);
}

static efi_status_t EFIAPI get_var_all(struct gbl_efi_fastboot_protocol *this,
				       void *ctx, get_var_all_callback cb)
{
	EFI_ENTRY("%p, %p, %p", this, ctx, cb);
	if (this != &gbl_efi_fastboot_proto || cb == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	for (struct fastboot_var *var = &vars[0]; var->args; var++) {
		cb(ctx, args_len(var), var->args, var->val);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_staged(struct gbl_efi_fastboot_protocol *this,
				      size_t *bufsize, size_t *buffer_remains,
				      u8 *buffer)
{
	EFI_ENTRY("%p, %p, %p, %p", this, bufsize, buffer_remains, buffer);
	if (this != &gbl_efi_fastboot_proto || bufsize == NULL ||
	    buffer_remains == NULL || buffer == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	return EFI_EXIT(EFI_UNSUPPORTED);
}

static efi_status_t EFIAPI
command_exec(struct gbl_efi_fastboot_protocol *this, size_t num_args,
	     const char *const *args, size_t download_buffer_size,
	     size_t download_buffer_used_size, u8 *download_buffer,
	     gbl_efi_fastboot_command_exec_result *implementation,
	     fastboot_message_sender sender, void *ctx)
{
	EFI_ENTRY("%p, %zu, %p, %zu, %zu, %p, %p, %p, %p", this, num_args, args,
		  download_buffer_size, download_buffer_used_size,
		  download_buffer, implementation, sender, ctx);
	if (this != &gbl_efi_fastboot_proto || args == NULL ||
	    implementation == NULL || sender == NULL ||
	    (download_buffer_size > 0 && download_buffer == NULL)) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	*implementation = GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL;
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_partition_type(
	struct gbl_efi_fastboot_protocol *this, const char *part_name,
	size_t *part_type_len, char *part_type)
{
	EFI_ENTRY("%p, %p, %p, %p", this, part_name, part_type_len, part_type);
	if (this != &gbl_efi_fastboot_proto || part_name == NULL ||
	    part_type_len == NULL || part_type == NULL) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	return EFI_EXIT(EFI_UNSUPPORTED);
}

static struct gbl_efi_fastboot_protocol gbl_efi_fastboot_proto = {
	.revision = GBL_EFI_FASTBOOT_PROTOCOL_REVISION,
	.serial_number = "cuttlefish-0xCAFED00D",
	.get_var = get_var,
	.get_var_all = get_var_all,
	.get_staged = get_staged,
	.command_exec = command_exec,
	.get_partition_type = get_partition_type,
};

efi_status_t gbl_efi_fastboot_register(void)
{
	efi_status_t ret = efi_add_protocol(efi_root, &gbl_efi_fastboot_guid,
					    &gbl_efi_fastboot_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install GBL_EFI_FASTBOOT_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}
