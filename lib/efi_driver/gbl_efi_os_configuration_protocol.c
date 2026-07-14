/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (C) 2024 The Android Open Source Project
 */

#include <efi.h>
#include <efi_api.h>
#include <efi_loader.h>
#include <env.h>
#include <gbl_efi_os_configuration.h>
#include <log.h>
#include <string.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

const efi_guid_t gbl_efi_os_config_guid =
	GBL_EFI_OS_CONFIGURATION_PROTOCOL_GUID;

static const char *gbl_efi_dt_source_name(u32 source)
{
	switch (source) {
	case GBL_EFI_DEVICE_TREE_SOURCE_BOOT:
		return "boot";
	case GBL_EFI_DEVICE_TREE_SOURCE_VENDOR_BOOT:
		return "vendor_boot";
	case GBL_EFI_DEVICE_TREE_SOURCE_DTBO:
		return "dtbo";
	case GBL_EFI_DEVICE_TREE_SOURCE_DTB:
		return "dtb";
	default:
		return "unknown";
	}
}

static const char *gbl_efi_dt_type_name(u32 type)
{
	switch (type) {
	case GBL_EFI_DEVICE_TREE_TYPE_DEVICE_TREE:
		return "device_tree";
	case GBL_EFI_DEVICE_TREE_TYPE_OVERLAY:
		return "overlay";
	case GBL_EFI_DEVICE_TREE_TYPE_PVM_DA_OVERLAY:
		return "pvm_da_overlay";
	default:
		return "unknown";
	}
}

static efi_status_t EFIAPI fixup_bootconfig(
	struct gbl_efi_os_configuration_protocol *self, size_t bootconfig_size,
	const char *bootconfig, size_t *fixup_buffer_size, char *fixup)
{
	char *andr_bootargs;
	size_t required_size;

	EFI_ENTRY("%p, %zu, %p, %p, %p", self, bootconfig_size, bootconfig,
		  fixup_buffer_size, fixup);

	if (!self || !bootconfig || !fixup_buffer_size || !fixup)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	debug("GBL OS config: input bootconfig size=%zu\n", bootconfig_size);
	debug("GBL OS config: input bootconfig begin\n");
	debug("%.*s\n", (int)bootconfig_size, bootconfig);
	debug("GBL OS config: input bootconfig end\n");

	/* extract */
	if (env_update_extract_subset("bootargs", "andr_bootargs", "androidboot.")) {
		printf("extract androidboot.xxx error\n");
		return EFI_EXIT(EFI_LOAD_ERROR);
	}

	andr_bootargs = env_get("andr_bootargs");
	if (!andr_bootargs) {
		*fixup_buffer_size = 0;
		return EFI_EXIT(EFI_SUCCESS);
	}
	debug("u-boot androidboot:\n    %s\n", andr_bootargs);

	required_size = strlen(andr_bootargs);
	if (*fixup_buffer_size < required_size) {
		*fixup_buffer_size = required_size;
		return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
	}

	/* return */
	memcpy(fixup, andr_bootargs, required_size);
	*fixup_buffer_size = required_size;

	debug("GBL: andr: %s\n", andr_bootargs);
	debug("GBL: size: %ld\n", (ulong)(*fixup_buffer_size));

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI select_device_trees(
	struct gbl_efi_os_configuration_protocol *self, size_t num_device_trees,
	struct gbl_efi_verified_device_tree *device_trees)
{
	EFI_ENTRY("%p, %zu, %p", self, num_device_trees, device_trees);
	bool found_base_dt = false;

	if (!self || !num_device_trees || !device_trees)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	printf("# GBL OS DT num=%zu\n", num_device_trees);
	for (size_t i = 0; i < num_device_trees; i++) {
		struct gbl_efi_verified_device_tree *dt = &device_trees[i];

		printf("# GBL OS DT: dt[%zu] source=%s(%u) type=%s(%u) id=0x%x rev=0x%x selected=%u dt=%p\n",
		       i,
		       gbl_efi_dt_source_name(dt->metadata.source),
		       dt->metadata.source,
		       gbl_efi_dt_type_name(dt->metadata.type),
		       dt->metadata.type,
		       dt->metadata.id,
		       dt->metadata.rev,
		       dt->selected,
		       dt->device_tree);
	}

	for (size_t i = 0; i < num_device_trees; i++) {
		if (device_trees[i].metadata.type ==
		    GBL_EFI_DEVICE_TREE_TYPE_DEVICE_TREE) {
			device_trees[i].selected = true;
			printf("# GBL OS config: select dt[%zu] as base device tree\n", i);
			found_base_dt = true;
		} else if (device_trees[i].metadata.type ==
		    GBL_EFI_DEVICE_TREE_TYPE_OVERLAY) {
			device_trees[i].selected = true;
			printf("# GBL OS config: select dt[%zu] as overlay\n", i);
		}
	}

	if (found_base_dt)
		return EFI_EXIT(EFI_SUCCESS);

	log_err("No base device tree provided, nothing to select.\n");
	return EFI_EXIT(EFI_INVALID_PARAMETER);
}

static efi_status_t EFIAPI select_fit_configuration(
	struct gbl_efi_os_configuration_protocol *self, size_t fit_size,
	const u8 *fit, size_t metadata_size, const u8 *metadata,
	size_t *selected_configuration_offset)
{
	EFI_ENTRY("%p, %zu, %p, %zu, %p, %p", self, fit_size, fit,
		  metadata_size, metadata, selected_configuration_offset);

	if (!self || !fit || !selected_configuration_offset ||
	    (metadata_size > 0 && !metadata))
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	return EFI_EXIT(EFI_UNSUPPORTED);
}

static struct gbl_efi_os_configuration_protocol gbl_efi_os_config_proto = {
	.revision = GBL_EFI_OS_CONFIGURATION_PROTOCOL_REVISION,
	.fixup_bootconfig = fixup_bootconfig,
	.select_device_trees = select_device_trees,
	.select_fit_configuration = select_fit_configuration,
};

efi_status_t gbl_efi_os_config_register(void)
{
	efi_status_t ret = efi_add_protocol(efi_root, &gbl_efi_os_config_guid,
					    &gbl_efi_os_config_proto);
	if (ret != EFI_SUCCESS)
		log_err("Failed to install GBL_EFI_OS_CONFIGURATION_PROTOCOL: 0x%lx\n",
			ret);

	return ret;
}
