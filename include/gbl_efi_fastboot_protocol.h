/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#ifndef __GBL_EFI_FASTBOOT_H__
#define __GBL_EFI_FASTBOOT_H__

#include <efi.h>
#include <efi_api.h>

#define GBL_EFI_FASTBOOT_PROTOCOL_REVISION 0x00010000

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8 32

#define GBL_EFI_FASTBOOT_PARTITION_TYPE_BUF_LEN 56

// Callback function pointer passed to GblEfiFastbootProtocol.get_var_all.
//
// context: Caller specific context.
// num_args: The number of elements in the Args array.
// args: An array of NULL-terminated strings that contains the variable name
//       followed by additional arguments if any.
// val: A NULL-terminated string representing the value.
typedef void (*get_var_all_callback)(void *context, size_t num_args,
				     const char *const *args, const char *val);

typedef enum {
	GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY = 0,
	GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
	GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO,
} gbl_efi_fastboot_message_type;

typedef efi_status_t (*fastboot_message_sender)(
	void *context, gbl_efi_fastboot_message_type msg_type, size_t msg_len,
	const char *msg);

typedef enum {
	GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_PROHIBITED = 0,
	GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL,
	GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL,
} gbl_efi_fastboot_command_exec_result;

extern const efi_guid_t gbl_efi_fastboot_guid;

struct gbl_efi_fastboot_protocol {
	// Revision of the protocol supported.
	u64 revision;
	// Null-terminated UTF-8 encoded string
	char serial_number[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];

	// Fastboot variable methods
	efi_status_t(EFIAPI *get_var)(
		/* in */ struct gbl_efi_fastboot_protocol *self,
		/* in */ size_t num_args,
		/* in */ const char *const *args,
		/* in out */ size_t *bufsize,
		/* out */ char *buf);
	efi_status_t(EFIAPI *get_var_all)(
		/* in */ struct gbl_efi_fastboot_protocol *self,
		/* in */ void *ctx,
		/* in */ get_var_all_callback cb);

	// Fastboot get_staged backend
	efi_status_t(EFIAPI *get_staged)(
		/* in */ struct gbl_efi_fastboot_protocol *self,
		/* in out */ size_t *bufsize,
		/* out */ size_t *buffer_remains,
		/* out */ u8 *buffer);

	efi_status_t(EFIAPI *command_exec)(
		/* in */ struct gbl_efi_fastboot_protocol *self,
		/* in */ size_t num_args,
		/* in */ const char *const *args,
		/* in */ size_t download_buffer_size,
		/* in */ size_t download_buffer_used_size,
		/* in */ u8 *download_buffer,
		/* out */ gbl_efi_fastboot_command_exec_result *implementation,
		/* in */ fastboot_message_sender sender,
		/* in */ void *context);

	efi_status_t(EFIAPI *get_partition_type)(
		/* in */ struct gbl_efi_fastboot_protocol *self,
		/* in */ const char *part_name,
		/* in out */ size_t *part_type_len,
		/* out */ char *part_type);
};

efi_status_t gbl_efi_fastboot_register(void);

#endif /* __GBL_EFI_FASTBOOT_H__ */
