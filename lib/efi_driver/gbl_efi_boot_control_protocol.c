// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2024 The Android Open Source Project
 */

#include <android_avb/ab.h>
#include <android_avb/avb_ops_user.h>
#include <bootm.h>
#include <efi.h>
#include <efi_loader.h>
#include <efi_variable.h>
#include <gbl_efi_boot_control_protocol.h>
#include <gbl_efi_boot_memory_protocol.h>
#include <image.h>
#include <lmb.h>
#include <linux/string.h>
#include <mapmem.h>
#include <stdlib.h>
#include <efi_selftest.h>
#include <log.h>
#include <avb_verify.h>
#include <asm/arch-rockchip/boot_mode.h>

const efi_guid_t gbl_efi_boot_control_guid = GBL_EFI_BOOT_CONTROL_PROTOCOL_GUID;
static struct gbl_efi_boot_control_protocol gbl_efi_slot_proto;

static AvbABData g_ab_data;
static bool g_ab_data_valid;

static efi_status_t load_ab_metadata(AvbABData *ab_data);

int gbl_control_get_current_slot_idx(void)
{
	AvbABData ab_data;
	bool found = false;
	u8 max_idx = 0;
	efi_status_t ret;

	ret = load_ab_metadata(&ab_data);
	if (ret != EFI_SUCCESS)
		return -1;

	for (int i = 0; i < ARRAY_SIZE(ab_data.slots); i++) {
		AvbABSlotData *slot = &ab_data.slots[i];

		if (slot_is_bootable(slot)) {
			if (!found ||
			    slot->priority > ab_data.slots[max_idx].priority) {
				max_idx = i;
				found = true;
			}
		}
	}

	if (!found)
		return -1;

	return max_idx;
}

static int kernel_get_arch(efi_physical_addr_t kernel)
{
	/* See Documentation/arm64/booting.txt in the Linux kernel */
#define LINUX_ARM64_IMAGE_MAGIC		0x644d5241
	int magic_offset = 0x38;
	u32 *ih_magic;

	ih_magic = (u32 *)(kernel + magic_offset);

	return (*ih_magic == le32_to_cpu(LINUX_ARM64_IMAGE_MAGIC)) ?
						IH_ARCH_ARM64 : IH_ARCH_ARM;
}

static int gbl_prepare_loaded_os(const struct gbl_efi_loaded_os *os,
				 struct bootm_info *bmi,
				 boot_os_fn **boot_fn)
{
	ulong relocated_addr;
	ulong image_size;
	int states = BOOTM_STATE_MEASURE | BOOTM_STATE_OS_PREP |
		     BOOTM_STATE_FDT;
	int ret;

	if (!os->kernel || !os->kernel_size || !os->device_tree ||
	    !os->device_tree_size) {
		log_err("# GBL loaded OS is missing kernel or device tree\n");
		return -EINVAL;
	}

	bootm_init(bmi);
	memset(&images, 0, sizeof(images));
	bmi->images = &images;
	bmi->cmd_name = "gbl";
	images.verify = env_get_yesno("verify");
	images.os.os = IH_OS_LINUX;
	images.os.arch = kernel_get_arch(os->kernel);
	images.os.type = IH_TYPE_KERNEL;
	images.os.comp = IH_COMP_NONE;	/* decompressed by GBL by default */
	images.os.image_start = os->kernel;
	images.os.image_len = os->kernel_size;
	images.os.load = os->kernel;
	images.rd_start = os->ramdisk;
	images.rd_end = os->ramdisk + os->ramdisk_size;
	images.ft_addr = map_sysmem(os->device_tree, 0);
	images.ft_len = os->device_tree_size;
	relocated_addr = os->kernel;
	image_size = os->kernel_size;

	if (images.os.arch == IH_ARCH_ARM64) {
		if (booti_setup(os->kernel, &relocated_addr, &image_size, false)) {
			log_err("# GBL loaded kernel is not a valid ARM64 Image\n");
			return -ENOEXEC;
		}

		if (relocated_addr != os->kernel) {
			log_err("# GBL loaded kernel requires relocation (0x%lx -> 0x%lx), which is unsupported\n",
				(ulong)os->kernel, relocated_addr);
			return -ENOTSUPP;
		}
	}

	images.ep = relocated_addr;
	images.os.start = relocated_addr;
	images.os.end = relocated_addr + image_size;

	lmb_reserve(images.ep, image_size, LMB_NONE);
	lmb_reserve(os->device_tree, os->device_tree_size, LMB_NONE);
	if (os->ramdisk && os->ramdisk_size)
		lmb_reserve(os->ramdisk, os->ramdisk_size, LMB_NONE);

	if (IS_ENABLED(CONFIG_SYS_BOOT_RAMDISK_HIGH) &&
	    os->ramdisk && os->ramdisk_size)
		states |= BOOTM_STATE_RAMDISK;

	log_debug("# GBL handoff: preparing kernel=%pa fdt=%p ramdisk=%pa\n",
		  &images.ep, images.ft_addr, &images.rd_start);
	ret = bootm_run_states(bmi, states);
	if (ret)
		return ret;

	*boot_fn = bootm_os_get_boot_func(images.os.os);
	if (!*boot_fn) {
		log_err("No boot function for OS type %u\n", images.os.os);
		return -ENOEXEC;
	}

	log_info("# GBL kernel: 0x%08llx - 0x%08llx (%lu KiB)\n",
  		  os->kernel, os->kernel + os->kernel_size,
  		  DIV_ROUND_UP(os->kernel_size, 1024));
	log_info("# GBL fdt: 0x%08llx - 0x%08llx (%lu KiB)\n",
  		  os->device_tree, os->device_tree + os->device_tree_size,
  		  DIV_ROUND_UP(os->device_tree_size, 1024));
	if (os->ramdisk_size)
		log_info("# GBL ramdisk: 0x%08llx - 0x%08llx (%lu KiB)\n",
	  		  os->ramdisk, os->ramdisk + os->ramdisk_size,
	  		  DIV_ROUND_UP(os->ramdisk_size, 1024));

	if (images.ep != os->kernel)
		log_info("# GBL relocated kernel: 0x%08lx\n", (ulong)images.ep);

	return 0;
}

static int fdt_remove_andr_bootargs(void *fdt)
{
	int chosen, len, ret = 0;
	size_t str_len;
	const char *bootargs;
	char *bootargs_tmp, *new_bootargs;
	char *item;
	bool changed = false;
	char *fwver;

	if (!fdt)
		return 0;

	chosen = fdt_path_offset(fdt, "/chosen");
	if (chosen < 0)
		return 0;

	bootargs = fdt_getprop(fdt, chosen, "bootargs", &len);
	if (!bootargs)
		return 0;

	str_len = strnlen(bootargs, len);
	if (str_len == len)
		return -EINVAL;

	bootargs_tmp = strdup(bootargs);
	new_bootargs = calloc(1, str_len + 1);
	if (!bootargs_tmp || !new_bootargs) {
		ret = -ENOMEM;
		goto out;
	}

	for (item = strtok(bootargs_tmp, " "); item; item = strtok(NULL, " ")) {
		if (!strncmp(item, "androidboot.", strlen("androidboot."))) {
			changed = true;
			continue;
		}

		if (*new_bootargs)
			strcat(new_bootargs, " ");
		strcat(new_bootargs, item);
	}

	if (changed)
		ret = fdt_setprop_string(fdt, chosen, "bootargs", new_bootargs);

	/* alias of "androidboot.fwver": for kernel cmdline can be read */
	fwver = env_get("fwver");
	if (fwver) {
		env_update("bootargs", fwver);
		env_set("fwver", NULL);
	}
out:
	free(bootargs_tmp);
	free(new_bootargs);

	return ret;
}

static efi_status_t ab_flow_to_efi_status(AvbABFlowResult result)
{
	switch (result) {
	case AVB_AB_FLOW_RESULT_OK:
		return EFI_SUCCESS;
	case AVB_AB_FLOW_RESULT_ERROR_OOM:
		return EFI_OUT_OF_RESOURCES;
	default:
		return EFI_DEVICE_ERROR;
	}
}

static efi_status_t load_ab_metadata(AvbABData *ab_data)
{
	if (!ab_data)
		return EFI_INVALID_PARAMETER;

	if (!g_ab_data_valid) {
		if (ab_get_slot_data(&g_ab_data) != 0)
			return EFI_DEVICE_ERROR;

		g_ab_data_valid = true;
	}

	*ab_data = g_ab_data;

	return EFI_SUCCESS;
}

static void fill_slot_info(const AvbABData *ab_data, u8 idx,
			   struct gbl_efi_slot_info *info)
{
	const AvbABSlotData *slot = &ab_data->slots[idx];

	memset(info, 0, sizeof(*info));
	info->suffix = 'a' + idx;
	info->priority = slot->priority;
	info->remaining_tries = slot->tries_remaining;
	info->successful = slot->successful_boot;
	info->unbootable_reason = slot_is_bootable((AvbABSlotData *)slot) ?
					  GBL_EFI_UNBOOTABLE_REASON_UNKNOWN_REASON :
					  GBL_EFI_UNBOOTABLE_REASON_NO_MORE_TRIES;
}

static efi_status_t EFIAPI
get_slot_count(struct gbl_efi_boot_control_protocol *self, u8 *slot_count)
{
	AvbABData ab_data;

	EFI_ENTRY("%p, %p", self, slot_count);
	if (self != &gbl_efi_slot_proto || !slot_count)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	efi_status_t res = load_ab_metadata(&ab_data);
	if (res != EFI_SUCCESS)
		return EFI_EXIT(res);

	*slot_count = ARRAY_SIZE(ab_data.slots);

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
get_slot_info(struct gbl_efi_boot_control_protocol *self, u8 idx,
	      struct gbl_efi_slot_info *info)
{
	AvbABData ab_data;

	EFI_ENTRY("%p, %uc, %p", self, idx, info);
	if (self != &gbl_efi_slot_proto || !info)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	efi_status_t res = load_ab_metadata(&ab_data);
	if (res != EFI_SUCCESS)
		return EFI_EXIT(res);

	if (idx >= ARRAY_SIZE(ab_data.slots))
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	fill_slot_info(&ab_data, idx, info);

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t
get_current_slot_idx(struct gbl_efi_boot_control_protocol *self,
		     AvbABData *ab_data, u8 *idx)
{
	u8 max_idx = 0;
	bool found = false;

	if (self != &gbl_efi_slot_proto || !ab_data || !idx)
		return EFI_INVALID_PARAMETER;

	for (int i = 0; i < ARRAY_SIZE(ab_data->slots); i++) {
		AvbABSlotData *slot = &ab_data->slots[i];

		if (slot_is_bootable(slot)) {
			if (!found ||
			    slot->priority > ab_data->slots[max_idx].priority) {
				max_idx = i;
				found = true;
			}
		}
	}

	if (!found)
		return EFI_NOT_FOUND;

	*idx = max_idx;
	return EFI_SUCCESS;
}

static efi_status_t EFIAPI
get_current_slot(struct gbl_efi_boot_control_protocol *self,
		 struct gbl_efi_slot_info *info)
{
	AvbABData ab_data;
	efi_status_t res;
	u8 idx;

	EFI_ENTRY("%p, %p", self, info);
	if (self != &gbl_efi_slot_proto || !info)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	res = load_ab_metadata(&ab_data);
	if (res != EFI_SUCCESS)
		return EFI_EXIT(res);

	res = get_current_slot_idx(self, &ab_data, &idx);
	if (res != EFI_SUCCESS)
		return EFI_EXIT(res);

	fill_slot_info(&ab_data, idx, info);

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
set_active_slot(struct gbl_efi_boot_control_protocol *self, u8 idx)
{
	AvbABData ab_data;
	unsigned int slot = idx;
	AvbABFlowResult ret;
	efi_status_t res;

	EFI_ENTRY("%p, %uc", self, idx);
	if (self != &gbl_efi_slot_proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	res = load_ab_metadata(&ab_data);
	if (res != EFI_SUCCESS)
		return EFI_EXIT(res);

	if (idx >= ARRAY_SIZE(ab_data.slots))
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	ret = ab_set_slot_active(&slot);
	if (ret == 0) {
		efi_status_t res;

		g_ab_data_valid = false;
		res = load_ab_metadata(&ab_data);
		if (res != EFI_SUCCESS)
			return EFI_EXIT(res);
	}

	return EFI_EXIT(ab_flow_to_efi_status(ret));
}

static efi_status_t EFIAPI
get_one_shot_boot_mode(struct gbl_efi_boot_control_protocol *self,
		       enum gbl_efi_one_shot_boot_mode *mode)
{
	EFI_ENTRY("%p, %p", self, mode);
	int boot_mode = plat_boot_mode();

	if (self != &gbl_efi_slot_proto || !mode)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	*mode = GBL_EFI_ONE_SHOT_BOOT_MODE_NONE;

	if (boot_mode == BOOT_MODE_RECOVERY)
		*mode = GBL_EFI_ONE_SHOT_BOOT_MODE_RECOVERY;
	else if (boot_mode == BOOT_MODE_BOOTLOADER)
		*mode = GBL_EFI_ONE_SHOT_BOOT_MODE_BOOTLOADER;
	/*
	 * TODO: GBL_EFI_BOOT_CONTROL_PROTOCOL.GetOneShotBootMode()
	 * must only be used for one-shot, non-persistent boot modes triggered
	 * by the user (e.g., the user holding the volume-down button during
	 * boot). We should not rely on persistent storage to determine a
	 * one-shot boot mode. Refer to the GBL documentation for this method.
	 *
	 * A serial console input-based implementation of this method (similar
	 * to the serial-based fastboot transport) could serve as a better
	 * reference implementation.
	 */
	return EFI_EXIT(EFI_SUCCESS);
}

#ifdef CONFIG_GBL_EFI_FW_API_LEVEL
static efi_status_t efi_init_gbl_fw_api_level(int andr_version)
{
	char *api_level = NULL;
	int ret;

	if (andr_version == 16)
		api_level = "202604";

	if (api_level) {
		log_info("# GBL Android %d api_level: %s\n", andr_version, api_level);
	} else {
		log_info("# GBL Android %d no api_level to set\n", andr_version);
		return -EINVAL;
	}

	ret = efi_set_variable_int(u"gbl_fw_api_level", &gbl_efi_vendor_guid,
				   EFI_VARIABLE_BOOTSERVICE_ACCESS |
				   EFI_VARIABLE_RUNTIME_ACCESS |
				   EFI_VARIABLE_READ_ONLY,
				   strlen(api_level), api_level, false);
	if (ret != EFI_SUCCESS) {
		log_info("# GBL Failed to set api_level, ret=0x%x\n", ret);
		ret = -EIO;
	}

	return ret;
}
#endif

static efi_status_t EFIAPI
handle_loaded_os(struct gbl_efi_boot_control_protocol *self,
		 const struct gbl_efi_loaded_os *os)
{
	struct gbl_android_boot_version version;
	struct bootm_info bmi;
	boot_os_fn *boot_fn;
	efi_status_t ret;
	u32 andr_version;

	EFI_ENTRY("%p, %p", self, os);
	if (self != &gbl_efi_slot_proto || !os)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	ret = gbl_efi_boot_memory_get_init_boot_version(&version);
	if (ret == EFI_SUCCESS) {
		if (version.os_version) {
			andr_version = (version.os_version >> 25) & 0x7f;
			log_info("# GBL init_boot: Android %u.%u, Build %u.%u, v%d\n",
				 andr_version,
				 (version.os_version >> 18) & 0x7F,
				 ((version.os_version >> 4) & 0x7f) + 2000,
				 version.os_version & 0x0F,
				 version.header_version);
#ifdef CONFIG_GBL_EFI_FW_API_LEVEL
			if (efi_init_gbl_fw_api_level(andr_version))
				return EFI_EXIT(EFI_INVALID_PARAMETER);
#endif
		}
	} else {
		log_info("# GBL: No 'init_boot' record !\n");
	}

	log_debug("# GBL: %s\n", env_get("andr_bootargs"));

	log_info("# GBL handoff os: kernel=0x%08llx, fdt=0x%08llx, ramdisk=0x%08llx\n",
		 os->kernel, os->device_tree, os->ramdisk);

	if (gbl_prepare_loaded_os(os, &bmi, &boot_fn))
		return EFI_EXIT(EFI_DEVICE_ERROR);

	if (fdt_remove_andr_bootargs((void *)os->device_tree))
		return EFI_EXIT(EFI_DEVICE_ERROR);

	log_info("# GBL handoff: exit boot-services\n");
	if (efi_exit_boot_services_current_image() != EFI_SUCCESS)
		return EFI_EXIT(EFI_DEVICE_ERROR);

	log_info("# GBL handoff: jumping to kernel entry 0x%08lx\n", images.ep);
	if (boot_selected_os(BOOTM_STATE_OS_GO, &bmi, boot_fn))
		return EFI_EXIT(EFI_DEVICE_ERROR);

	return EFI_EXIT(EFI_SUCCESS);
}

static struct gbl_efi_boot_control_protocol gbl_efi_slot_proto = {
	.revision = GBL_EFI_BOOT_CONTROL_REVISION,
	.get_slot_count = get_slot_count,
	.get_slot_info = get_slot_info,
	.get_current_slot = get_current_slot,
	.set_active_slot = set_active_slot,
	.get_one_shot_boot_mode = get_one_shot_boot_mode,
	.handle_loaded_os = handle_loaded_os,
};

efi_status_t gbl_efi_boot_control_register(void)
{
	efi_status_t ret = efi_add_protocol(
		efi_root, &gbl_efi_boot_control_guid, &gbl_efi_slot_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install GBL_EFI_BOOT_CONTROL_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}
