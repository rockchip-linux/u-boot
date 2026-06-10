// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef __GBL_EFI_BOOT_MEMORY_PROTOCOL_H__
#define __GBL_EFI_BOOT_MEMORY_PROTOCOL_H__

#include <efi.h>
#include <efi_api.h>

#define GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION 0x00010000

#define GBL_EFI_BOOT_MEMORY_PROTOCOL_GUID \
	EFI_GUID(0x309f2874, 0xad59, 0x4fd2, 0xaf, 0x5e, 0xce, 0x0f, 0x4a, \
		 0xb4, 0x01, 0xa6)

enum gbl_efi_boot_buffer_type {
	GBL_EFI_BOOT_BUFFER_TYPE_GENERAL_LOAD = 0,
	GBL_EFI_BOOT_BUFFER_TYPE_KERNEL,
	GBL_EFI_BOOT_BUFFER_TYPE_RAMDISK,
	GBL_EFI_BOOT_BUFFER_TYPE_FDT,
	GBL_EFI_BOOT_BUFFER_TYPE_PVMFW_DATA,
	GBL_EFI_BOOT_BUFFER_TYPE_FASTBOOT_DOWNLOAD,
};

typedef u32 gbl_efi_partition_buffer_flag;
#define GBL_EFI_PARTITION_BUFFER_FLAG_PRELOADED (1U << 0)

struct gbl_android_boot_version {
	u32 os_version;
	u32 header_version;
};

extern const efi_guid_t gbl_efi_boot_memory_guid;

struct gbl_efi_boot_memory_protocol {
	u64 revision;

	efi_status_t(EFIAPI *get_partition_buffer)(
		struct gbl_efi_boot_memory_protocol *self,
		/* in */ const char *base_name,
		/* out */ size_t *size,
		/* out */ void **addr,
		/* out */ gbl_efi_partition_buffer_flag *flag);

	efi_status_t(EFIAPI *sync_partition_buffer)(
		struct gbl_efi_boot_memory_protocol *self,
		/* in */ bool sync_preloaded);

	efi_status_t(EFIAPI *get_boot_buffer)(
		struct gbl_efi_boot_memory_protocol *self,
		/* in */ enum gbl_efi_boot_buffer_type buf_type,
		/* out */ size_t *size,
		/* out */ void **addr);
};

efi_status_t gbl_efi_boot_memory_register(void);
efi_status_t gbl_efi_boot_memory_get_init_boot_version(
	struct gbl_android_boot_version *version);

#endif /* __GBL_EFI_BOOT_MEMORY_PROTOCOL_H__ */
