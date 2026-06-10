/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#include <blk.h>
#include <console.h>
#include <efi.h>
#include <efi_api.h>
#include <efi_loader.h>
#include <env.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_mmc.h>
#include <fs.h>
#include <gbl_efi_fastboot_protocol.h>
#include <log.h>
#include <part.h>

const efi_guid_t gbl_efi_fastboot_guid = GBL_EFI_FASTBOOT_PROTOCOL_GUID;
static struct gbl_efi_fastboot_protocol gbl_efi_fastboot_proto;

static size_t staged_offset;

static const char *const fastboot_vars[] = {
	"version",
	"filesize",
	"downloadsize",
	"serialno",
	"version-baseband",
	"product",
	"platform",
	"logical-block-size",
	"erase-block-size",
	"vboot-state",
	"flash-unlocked",
	"avb-state",
};

void fastboot_oem_board(char *cmd_parameter, void *data, u32 size,
			char *response);

static uintptr_t gbl_fastboot_buf_addr(void)
{
	ulong addr;

	addr = env_get_hex("gbl_fastboot_buf_addr", CONFIG_FASTBOOT_BUF_ADDR);
	if (!addr)
		addr = CONFIG_FASTBOOT_BUF_ADDR;

	return addr;
}

static size_t gbl_fastboot_buf_size(void)
{
	ulong size;

	size = env_get_hex("gbl_fastboot_buf_size", CONFIG_FASTBOOT_BUF_SIZE);
	if (!size)
		size = CONFIG_FASTBOOT_BUF_SIZE;

	return size;
}

static void prepare_fastboot_context(void)
{
	fastboot_init((void *)gbl_fastboot_buf_addr(), gbl_fastboot_buf_size());
}

static efi_status_t copy_response_payload(const char *response, size_t *bufsize,
					  char *buf)
{
	size_t payload_len;

	if (!strncmp(response, "FAIL", 4))
		return EFI_NOT_FOUND;

	if (strncmp(response, "OKAY", 4))
		return EFI_PROTOCOL_ERROR;

	payload_len = strlen(response + 4);
	if (payload_len > *bufsize) {
		*bufsize = payload_len;
		return EFI_BUFFER_TOO_SMALL;
	}

	if (payload_len)
		memcpy(buf, response + 4, payload_len);
	*bufsize = payload_len;

	return EFI_SUCCESS;
}

static void join_args(char *buf, size_t buf_size, size_t num_args,
		      const char *const *args)
{
	size_t i;
	size_t used = 0;

	if (!buf_size)
		return;

	buf[0] = '\0';

	for (i = 0; i < num_args; i++) {
		if (!args[i])
			break;
		if (i > 0 && used + 1 < buf_size)
			buf[used++] = ':';

		used += strlcpy(buf + used, args[i], buf_size - used);
		if (used >= buf_size) {
			buf[buf_size - 1] = '\0';
			return;
		}
	}
}

static efi_status_t send_fastboot_response(const char *response,
					   fastboot_message_sender sender,
					   void *ctx)
{
	gbl_efi_fastboot_message_type type;
	const char *msg;

	if (!strncmp(response, "INFO", 4)) {
		type = GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO;
		msg = response + 4;
	} else if (!strncmp(response, "FAIL", 4)) {
		type = GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL;
		msg = response + 4;
	} else if (!strncmp(response, "OKAY", 4)) {
		type = GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY;
		msg = response + 4;
	} else {
		return EFI_PROTOCOL_ERROR;
	}

	return sender(ctx, type, strlen(msg), msg);
}

static efi_status_t handle_oem_console(fastboot_message_sender sender, void *ctx)
{
	char line[FASTBOOT_RESPONSE_LEN] = { 0 };
	int ret;

	if (console_record_isempty())
		return sender(ctx, GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
			      strlen("Empty console"), "Empty console");

	while (!console_record_isempty()) {
		ret = console_record_readline(line, sizeof(line));
		if (ret < 0)
			return sender(ctx, GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
				      strlen("Error reading console"),
				      "Error reading console");

		ret = sender(ctx, GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO,
			     strlen(line), line);
		if (ret != EFI_SUCCESS)
			return ret;
	}

	console_record_reset();

	return sender(ctx, GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY, 0, "");
}

static efi_status_t handle_oem_board(const char *cmd, size_t download_buffer_size,
				     size_t download_buffer_used_size,
				     u8 *download_buffer,
				     fastboot_message_sender sender, void *ctx)
{
	char response[FASTBOOT_RESPONSE_LEN] = { 0 };
	size_t copy_size;
	void *buf = (void *)gbl_fastboot_buf_addr();
	size_t buf_size = gbl_fastboot_buf_size();

	if (download_buffer_used_size > download_buffer_size ||
	    download_buffer_used_size > buf_size)
		return sender(ctx, GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
			      strlen("Download buffer too small"),
			      "Download buffer too small");

	copy_size = download_buffer_used_size;
	if (copy_size && download_buffer)
		memcpy(buf, download_buffer, copy_size);

	env_set_hex("filesize", copy_size);
	fastboot_oem_board((char *)cmd, buf, copy_size, response);
	staged_offset = 0;

	return send_fastboot_response(response, sender, ctx);
}

static efi_status_t EFIAPI get_var(struct gbl_efi_fastboot_protocol *this,
				   size_t num_args, const char *const *fb_args,
				   size_t *bufsize, char *buf)
{
	char cmd[128];
	char response[FASTBOOT_RESPONSE_LEN] = { 0 };

	EFI_ENTRY("%p, %lu, %p, %p, %p", this, num_args, fb_args, bufsize, buf);
	if (this != &gbl_efi_fastboot_proto || fb_args == NULL || buf == NULL ||
	    bufsize == NULL || !num_args)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	prepare_fastboot_context();
	join_args(cmd, sizeof(cmd), num_args, fb_args);
	fastboot_getvar(cmd, response);

	return EFI_EXIT(copy_response_payload(response, bufsize, buf));
}

static efi_status_t EFIAPI get_var_all(struct gbl_efi_fastboot_protocol *this,
				       void *ctx, get_var_all_callback cb)
{
	size_t i;
	char response[FASTBOOT_RESPONSE_LEN] = { 0 };
	const char *args[1];

	EFI_ENTRY("%p, %p, %p", this, ctx, cb);
	if (this != &gbl_efi_fastboot_proto || cb == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	prepare_fastboot_context();

	for (i = 0; i < ARRAY_SIZE(fastboot_vars); i++) {
		fastboot_getvar((char *)fastboot_vars[i], response);
		if (strncmp(response, "OKAY", 4))
			continue;

		args[0] = fastboot_vars[i];
		cb(ctx, ARRAY_SIZE(args), args, response + 4);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_staged(struct gbl_efi_fastboot_protocol *this,
				      size_t *bufsize, size_t *buffer_remains,
				      u8 *buffer)
{
	size_t total;
	size_t read_len;
	const u8 *src;

	EFI_ENTRY("%p, %p, %p, %p", this, bufsize, buffer_remains, buffer);
	if (this != &gbl_efi_fastboot_proto || bufsize == NULL ||
	    buffer_remains == NULL || buffer == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	total = env_get_hex("filesize", 0);
	if (staged_offset > total)
		staged_offset = 0;

	if (*bufsize > total - staged_offset)
		read_len = total - staged_offset;
	else
		read_len = *bufsize;

	src = (const u8 *)gbl_fastboot_buf_addr() + staged_offset;
	if (read_len)
		memcpy(buffer, src, read_len);

	staged_offset += read_len;
	*bufsize = read_len;
	*buffer_remains = total - staged_offset;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
	command_exec(struct gbl_efi_fastboot_protocol *this, size_t num_args,
	     const char *const *args, size_t download_buffer_size,
	     size_t download_buffer_used_size, u8 *download_buffer,
	     gbl_efi_fastboot_command_exec_result *implementation,
	     fastboot_message_sender sender, void *ctx)
{
	char cmd[128];

	EFI_ENTRY("%p, %zu, %p, %zu, %zu, %p, %p, %p, %p", this, num_args, args,
		  download_buffer_size, download_buffer_used_size,
		  download_buffer, implementation, sender, ctx);
	if (this != &gbl_efi_fastboot_proto || args == NULL ||
	    implementation == NULL || sender == NULL ||
	    (download_buffer_size > 0 && download_buffer == NULL))
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	*implementation = GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL;
	staged_offset = 0;

	if (!num_args)
		return EFI_EXIT(EFI_SUCCESS);

	join_args(cmd, sizeof(cmd), num_args, args);

	if (!strncmp(cmd, "oem board ", strlen("oem board "))) {
		*implementation = GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL;
		return EFI_EXIT(handle_oem_board(cmd + strlen("oem board "),
						 download_buffer_size,
						 download_buffer_used_size,
						 download_buffer, sender, ctx));
	}

	if (!strcmp(cmd, "oem console")) {
		*implementation = GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL;
		return EFI_EXIT(handle_oem_console(sender, ctx));
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI get_partition_type(
	struct gbl_efi_fastboot_protocol *this, const char *part_name,
	size_t *part_type_len, char *part_type)
{
	int part_num;
	int ret;
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
	const char *type = "raw";
	size_t type_len;
	char response[FASTBOOT_RESPONSE_LEN] = { 0 };

	EFI_ENTRY("%p, %p, %p, %p", this, part_name, part_type_len, part_type);
	if (this != &gbl_efi_fastboot_proto || part_name == NULL ||
	    part_type_len == NULL || part_type == NULL)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	prepare_fastboot_context();

	part_num = fastboot_mmc_get_part_info(part_name, &dev_desc, &part_info,
					      response);
	if (part_num < 0)
		return EFI_EXIT(EFI_NOT_FOUND);

	ret = fs_set_blk_dev_with_part(dev_desc, part_num);
	if (ret >= 0)
		type = fs_get_type_name();

	type_len = strlen(type);
	if (type_len > *part_type_len) {
		*part_type_len = type_len;
		return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
	}

	memcpy(part_type, type, type_len);
	*part_type_len = type_len;

	return EFI_EXIT(EFI_SUCCESS);
}

static struct gbl_efi_fastboot_protocol gbl_efi_fastboot_proto = {
	.revision = GBL_EFI_FASTBOOT_PROTOCOL_REVISION,
	.get_var = get_var,
	.get_var_all = get_var_all,
	.get_staged = get_staged,
	.command_exec = command_exec,
	.get_partition_type = get_partition_type,
};

efi_status_t gbl_efi_fastboot_register(void)
{
	const char *serial;
	efi_status_t ret;

	serial = env_get("serial#");
	memset(gbl_efi_fastboot_proto.serial_number, 0,
	       sizeof(gbl_efi_fastboot_proto.serial_number));
	if (serial && *serial)
		strlcpy(gbl_efi_fastboot_proto.serial_number, serial,
			sizeof(gbl_efi_fastboot_proto.serial_number));

	ret = efi_add_protocol(efi_root, &gbl_efi_fastboot_guid,
			       &gbl_efi_fastboot_proto);
	if (ret != EFI_SUCCESS)
		log_err("Failed to install GBL_EFI_FASTBOOT_PROTOCOL: 0x%lx\n",
			ret);

	return ret;
}
