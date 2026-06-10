/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2025 The Android Open Source Project
 */

#include <efi.h>
#include <efi_api.h>
#include <gbl_efi_fastboot_transport.h>
#include <gbl_efi_fastboot_transport_dummy.h>
#include <gbl_efi_fastboot_transport_interactive_serial.h>
#include <gbl_efi_fastboot_transport_usb.h>
#include <efi_loader.h>
#include <log.h>

const efi_guid_t gbl_efi_fastboot_transport_guid =
	GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_GUID;

efi_status_t gbl_efi_fastboot_transport_register(void)
{
	efi_status_t ret = EFI_SUCCESS;

	if (IS_ENABLED(CONFIG_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_DUMMY)) {
		ret = gbl_efi_fastboot_transport_dummy_register();
		if (ret != EFI_SUCCESS) {
			log_err("Failed to install GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL: 0x%lx\n",
				ret);
		}
	}

	if (IS_ENABLED(CONFIG_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_USB)) {
		ret = gbl_efi_fastboot_transport_usb_register();
		if (ret != EFI_SUCCESS) {
			log_err("Failed to install GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL: 0x%lx\n",
				ret);
		}
	}

	if (IS_ENABLED(CONFIG_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_SERIAL)) {
		ret = gbl_efi_fastboot_transport_interactive_serial_register();
		if (ret != EFI_SUCCESS) {
			log_err("Failed to install GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL: 0x%lx\n",
				ret);
		}
	}

	return ret;
}
