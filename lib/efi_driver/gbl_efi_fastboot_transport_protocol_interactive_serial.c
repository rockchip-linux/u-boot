/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#include <cyclic.h>
#include <efi.h>
#include <efi_api.h>
#include <gbl_efi_fastboot_transport.h>
#include <gbl_efi_fastboot_transport_interactive_serial.h>
#include <efi_loader.h>
#include <log.h>
#include <membuff.h>

#define DESCRIPTION "serial-interactive"

static struct gbl_efi_fastboot_transport_protocol
	gbl_efi_fastboot_transport_interactive_serial_proto;

static void print_help(void)
{
	printf("Fastboot Interactive Serial options:\n"
	       "\t'a': send 'getvar:all'\n"
	       "\t'r': send 'reboot'\n"
	       "\t'b': send test bad command\n"
	       "\t'h': print this help message\n");
}

#define BUFFER_SIZE (100)
static char context_inner_buffer[BUFFER_SIZE];
typedef struct _Context {
	struct membuff mb;
	struct cyclic_info cyclic;
	bool registered;
} Context;
static Context ctx;

static void poll_loop(struct cyclic_info *c)
{
	Context *my_ctx = container_of(c, Context, cyclic);
	struct membuff *mb = &my_ctx->mb;
	if (tstc()) {
		membuff_putbyte(mb, getchar());
	}
}

static efi_status_t EFIAPI
start(struct gbl_efi_fastboot_transport_protocol *this)
{
	EFI_ENTRY("%p", this);
	if (this != &gbl_efi_fastboot_transport_interactive_serial_proto) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	membuff_init(&ctx.mb, context_inner_buffer, BUFFER_SIZE);
	cyclic_register(&ctx.cyclic, poll_loop, 100 * 1000 /*100ms*/,
			DESCRIPTION);
	ctx.registered = true;
	print_help();

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI stop(struct gbl_efi_fastboot_transport_protocol *this)
{
	EFI_ENTRY("%p", this);
	if (this != &gbl_efi_fastboot_transport_interactive_serial_proto) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}
	if (!ctx.registered) {
		return EFI_EXIT(EFI_NOT_STARTED);
	}

	cyclic_unregister(&ctx.cyclic);
	ctx.registered = false;
	membuff_uninit(&ctx.mb);

	return EFI_EXIT(EFI_SUCCESS);
}

static void fill_reply(char *s, size_t *bufsize, void *buf)
{
	assert(s);
	*bufsize = min(strlen(s), *bufsize);
	strncpy((char *)buf, s, *bufsize);
}

static efi_status_t process_key(int key, size_t *bufsize, void *buf)
{
	efi_status_t res = EFI_SUCCESS;

	switch (key) {
	case 'a': {
		fill_reply("getvar:all", bufsize, buf);
		break;
	}
	case 'r': {
		fill_reply("reboot", bufsize, buf);
		break;
	}
	case 'b': {
		fill_reply("bad:command", bufsize, buf);
		break;
	}
	case 'h':
		print_help();
		// Fall through intentionally to return success
		// and no fastboot packets
	default:
		*bufsize = 0;
		res = EFI_SUCCESS;
		break;
	}

	return res;
}

static efi_status_t EFIAPI
receive(struct gbl_efi_fastboot_transport_protocol *this, size_t *bufsize,
	void *buf, gbl_efi_fastboot_rx_mode mode)
{
	struct membuff *mb = &ctx.mb;
	EFI_ENTRY_NO_LOG("%p, %p, %p, %u", this, bufsize, buf, mode);
	if (this != &gbl_efi_fastboot_transport_interactive_serial_proto ||
	    bufsize == NULL || (*bufsize > 0 && buf == NULL)) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (membuff_avail(mb)) {
		char a;
		do {
			a = membuff_getbyte(mb);
		} while (a == membuff_peekbyte(mb));

		efi_status_t res = process_key(a, bufsize, buf);
		return EFI_EXIT_NO_LOG(res);
	}

	*bufsize = 0;
	return EFI_EXIT_NO_LOG(EFI_SUCCESS);
}

static efi_status_t EFIAPI send(struct gbl_efi_fastboot_transport_protocol *this,
				size_t *bufsize, const void *buf)
{
	EFI_ENTRY_NO_LOG("%p, %p, %p", this, bufsize, buf);
	if (this != &gbl_efi_fastboot_transport_interactive_serial_proto ||
	    bufsize == NULL || (*bufsize > 0 && buf == NULL)) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (*bufsize < 4) {
		log_err("Bad fastboot command length: %zu\n", *bufsize);
		return EFI_EXIT_NO_LOG(EFI_SUCCESS);
	}

	if (0 == strncmp(buf, "INFO", 4)) {
		printf("%.*s\n", (int)*bufsize - 4, &((const char *)buf)[4]);
	} else if (0 == strncmp(buf, "FAIL", 4)) {
		printf("Fail: %.*s\n", (int)*bufsize - 4,
		       &((const char *)buf)[4]);
	} else if (0 == strncmp(buf, "OKAY", 4)) {
		// nop
	} else {
		log_err("Bad fastboot command: %.*s\n", 4, (const char *)buf);
		return EFI_EXIT_NO_LOG(EFI_SUCCESS);
	}

	return EFI_EXIT_NO_LOG(EFI_SUCCESS);
}

static efi_status_t EFIAPI
gbl_efi_flush(struct gbl_efi_fastboot_transport_protocol *this)
{
	EFI_ENTRY_NO_LOG("%p", this);
	if (this != &gbl_efi_fastboot_transport_interactive_serial_proto) {
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	return EFI_EXIT_NO_LOG(EFI_SUCCESS);
}

efi_status_t gbl_efi_fastboot_transport_interactive_serial_register(void)
{
	efi_handle_t handle = NULL;
	efi_status_t ret = efi_install_multiple_protocol_interfaces(
		&handle, &gbl_efi_fastboot_transport_guid,
		&gbl_efi_fastboot_transport_interactive_serial_proto, NULL);

	if (ret != EFI_SUCCESS) {
		log_err("Failed to install Interactive Serial GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}

static struct gbl_efi_fastboot_transport_protocol
	gbl_efi_fastboot_transport_interactive_serial_proto = {
		.revision = GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_REVISION,
		.description = DESCRIPTION,
		.start = start,
		.stop = stop,
		.receive = receive,
		.send = send,
		.flush = gbl_efi_flush,
	};
