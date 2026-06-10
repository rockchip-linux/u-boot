// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026 Rockchip Electronics Co., Ltd.
 */

#include <config.h>
#include <dm.h>
#include <efi.h>
#include <efi_api.h>
#include <efi_loader.h>
#include <blk.h>
#include <part.h>
#include <malloc.h>
#include <android_avb/ab.h>
#include <android_image.h>
#include <env.h>
#include <gbl_efi_boot_control_protocol.h>
#include <gbl_efi_boot_memory_protocol.h>
#include <log.h>
#include <string.h>
#include <sysmem.h>
#include <vsprintf.h>

DECLARE_GLOBAL_DATA_PTR;

const efi_guid_t gbl_efi_boot_memory_guid =
	GBL_EFI_BOOT_MEMORY_PROTOCOL_GUID;

static struct gbl_efi_boot_memory_protocol gbl_efi_boot_memory_proto;

#define SZ_M(i)					(0x00100000UL * i)
#define GBL_BOOT_BUF_KERNEL_DEFAULT_SIZE	SZ_M(96)
#define GBL_BOOT_BUF_FDT_DEFAULT_SIZE		SZ_M(1) - SZ_4K
#define GBL_BOOT_BUF_RAMDISK_DEFAULT_SIZE	SZ_M(93)
#define GBL_BOOT_BUF_PVMFW_DEFAULT_SIZE		SZ_M(4)

/* Offset: 257M = 162M(ramdisk_addr_r) + 1M(gap) + 93M(GBL_BOOT_BUF_RAMDISK_DEFAULT_SIZE) */
#define GBL_BOOT_BUF_TYPE_GENERAL_ADDR		(CFG_SYS_SDRAM_BASE + SZ_M(256))
#define GBL_BOOT_BUF_TYPE_GENERAL_SIZE		SZ_M(256)
#define GBL_PARTITION_BUF_TABLE_ADDR		\
		(GBL_BOOT_BUF_TYPE_GENERAL_ADDR + GBL_BOOT_BUF_TYPE_GENERAL_SIZE)
#define GBL_PARTITION_BUF_TABLE_SIZE		SZ_M(256)

#define GBL_PARTITION_NAME_MAX_LEN		36
#define GBL_PARTITION_RECORDS_MAX		16
#define GBL_SLOT_SUFFIX_LEN			2

struct gbl_boot_buffer_env {
	const char *name;
	const char *addr_env;
	const char *size_env;
	ulong default_size;
	ulong alignment;
	int sysmem_id;
};

struct gbl_part_name {
	const char *base_name;
};

struct gbl_part_buf_record {
	char base_name[GBL_PARTITION_NAME_MAX_LEN + 1];
	void *addr;
	size_t size;
	bool preloaded;
};

static efi_status_t get_partition_size(const char *base_name, size_t *size);
static efi_status_t append_slot_to_partition_name(const char *base_name,
						  char *slotted_partition_name,
						  size_t slotted_partition_name_size);

static struct gbl_part_buf_record gbl_part_buf_records[GBL_PARTITION_RECORDS_MAX];
static size_t gbl_part_buf_record_cnt;
static bool gbl_slot_suffix_valid;
static bool gbl_general_buffer_reserved;
static char gbl_slot_suffix[GBL_SLOT_SUFFIX_LEN + 1];

static const struct gbl_part_name gbl_partition_list[] = {
	{ ANDROID_PARTITION_BOOT },
	{ ANDROID_PARTITION_VENDOR_BOOT },
	{ ANDROID_PARTITION_INIT_BOOT },
	{ ANDROID_PARTITION_RECOVERY },
	{ ANDROID_PARTITION_VBMETA },
	{ ANDROID_PARTITION_RESOURCE },
	{ PART_DTBO },
	{ "dtb" },
	{ "vbmeta_system" },
	{ "vbmeta_vendor" },
};

static const struct gbl_boot_buffer_env gbl_boot_buffer_envs[] = {
	[GBL_EFI_BOOT_BUFFER_TYPE_KERNEL] = {
		.name = "kernel",
		.addr_env = "kernel_addr_r",
		.size_env = "kernel_size_r",
		.default_size = GBL_BOOT_BUF_KERNEL_DEFAULT_SIZE,
		.alignment = SZ_2M,
		.sysmem_id = MEM_KERNEL,
	},
	[GBL_EFI_BOOT_BUFFER_TYPE_FDT] = {
		.name = "fdt",
		.addr_env = "fdt_addr_r",
		.size_env = "fdt_size_r",
		.default_size = GBL_BOOT_BUF_FDT_DEFAULT_SIZE,
		.alignment = 8,
		.sysmem_id = MEM_FDT,
	},
	[GBL_EFI_BOOT_BUFFER_TYPE_RAMDISK] = {
		.name = "ramdisk",
		.addr_env = "ramdisk_addr_r",
		.size_env = "ramdisk_size_r",
		.default_size = GBL_BOOT_BUF_RAMDISK_DEFAULT_SIZE,
		.alignment = 1,
		.sysmem_id = MEM_RAMDISK,
	},
	[GBL_EFI_BOOT_BUFFER_TYPE_PVMFW_DATA] = {
		.name = "pvmfw",
		.addr_env = "pvmfw_addr_r",
		.size_env = "pvmfw_size_r",
		.default_size = GBL_BOOT_BUF_PVMFW_DEFAULT_SIZE,
		.alignment = SZ_4K,
	},
};

static bool aligned_addr(ulong addr, ulong alignment)
{
	if (alignment <= 1)
		return true;

	return !(addr & (alignment - 1));
}

static const struct gbl_boot_buffer_env *
get_boot_buffer_env(enum gbl_efi_boot_buffer_type buf_type)
{
	if (buf_type >= ARRAY_SIZE(gbl_boot_buffer_envs))
		return NULL;

	return &gbl_boot_buffer_envs[buf_type];
}

static bool partition_requires_8_byte_alignment(const char *base_name)
{
	return !strcmp(base_name, "dtb") || !strcmp(base_name, PART_DTBO);
}

static ulong partition_required_alignment(const char *base_name)
{
	if (partition_requires_8_byte_alignment(base_name))
		return 8;

	return 1;
}

static efi_status_t get_cached_slot_suffix(void)
{
	int slot_idx;

	if (gbl_slot_suffix_valid)
		return EFI_SUCCESS;

	slot_idx = gbl_control_get_current_slot_idx();
	if (slot_idx < 0 || slot_idx >= SLOT_NUM) {
		log_err("Failed to get current slot index\n");
		return EFI_DEVICE_ERROR;
	}

	snprintf(gbl_slot_suffix, sizeof(gbl_slot_suffix), "_%c",
		 'a' + slot_idx);
	gbl_slot_suffix_valid = true;
	return EFI_SUCCESS;
}

static efi_status_t append_slot_to_partition_name(const char *base_name,
						  char *slotted_partition_name,
						  size_t slotted_partition_name_size)
{
	efi_status_t ret;

	if (!base_name || !slotted_partition_name ||
	    !slotted_partition_name_size)
		return EFI_INVALID_PARAMETER;

	ret = get_cached_slot_suffix();
	if (ret != EFI_SUCCESS)
		return ret;

	if (snprintf(slotted_partition_name, slotted_partition_name_size,
		     "%s%s", base_name, gbl_slot_suffix) >=
	    slotted_partition_name_size)
		return EFI_BUFFER_TOO_SMALL;

	return EFI_SUCCESS;
}

static efi_status_t alloc_partition_table_buffer(const char *base_name,
						 ulong *addr, size_t *size,
						 bool *preloaded)
{
	ulong table_base = GBL_PARTITION_BUF_TABLE_ADDR;
	ulong running_offset = 0;
	efi_status_t ret;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(gbl_partition_list); i++) {
		const struct gbl_part_name *def =
			&gbl_partition_list[i];
		size_t part_size;

		running_offset = ALIGN(running_offset,
				       partition_required_alignment(def->base_name));
		ret = get_partition_size(def->base_name, &part_size);
		if (ret == EFI_NOT_FOUND) {
			if (!strcmp(base_name, def->base_name))
				return EFI_NOT_FOUND;
			continue;
		}
		if (ret != EFI_SUCCESS)
			return ret;

		if (!strcmp(base_name, def->base_name)) {
			*addr = table_base + running_offset;
			*size = part_size;
			*preloaded = 0;

			return EFI_SUCCESS;
		}
		running_offset += part_size;
	}

	return EFI_NOT_FOUND;
}

static struct gbl_part_buf_record *
find_partition_record(const char *base_name)
{
	size_t i;

	for (i = 0; i < gbl_part_buf_record_cnt; i++) {
		if (!strcmp(gbl_part_buf_records[i].base_name, base_name))
			return &gbl_part_buf_records[i];
	}

	return NULL;
}

efi_status_t gbl_efi_boot_memory_get_init_boot_version(
	struct gbl_android_boot_version *version)
{
	struct gbl_part_buf_record *record;
	const struct boot_img_hdr_v34 *hdr;

	if (!version)
		return EFI_INVALID_PARAMETER;

	record = find_partition_record(ANDROID_PARTITION_INIT_BOOT);
	if (!record)
		return EFI_NOT_FOUND;

	if (!record->addr ||
	    record->size < sizeof(struct boot_img_hdr_v34))
		return EFI_NOT_FOUND;

	hdr = record->addr;
	if (android_image_check_header((void *)hdr))
		return EFI_COMPROMISED_DATA;

	version->os_version = hdr->os_version;
	version->header_version = hdr->header_version;

	return EFI_SUCCESS;
}

static efi_status_t remember_partition_record(const char *base_name, void *addr,
					      size_t size, bool preloaded)
{
	struct gbl_part_buf_record *record;

	record = find_partition_record(base_name);
	if (!record) {
		if (gbl_part_buf_record_cnt >=
		    ARRAY_SIZE(gbl_part_buf_records))
			return EFI_OUT_OF_RESOURCES;

		record = &gbl_part_buf_records
			[gbl_part_buf_record_cnt++];
		memset(record, 0, sizeof(*record));
		strlcpy(record->base_name, base_name, sizeof(record->base_name));
	}

	record->addr = addr;
	record->size = size;
	record->preloaded = preloaded;

	return EFI_SUCCESS;
}

static efi_status_t get_partition_size(const char *base_name, size_t *size)
{
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
	char slotted_partition_name[GBL_PARTITION_NAME_MAX_LEN +
				    GBL_SLOT_SUFFIX_LEN + 1];
	ulong bytes;
	efi_status_t ret;

	ret = append_slot_to_partition_name(base_name, slotted_partition_name,
					    sizeof(slotted_partition_name));
	if (ret != EFI_SUCCESS)
		return ret;

	dev_desc = plat_bootdev();
	if (!dev_desc)
		return EFI_DEVICE_ERROR;

	if (part_get_info_by_name(dev_desc, slotted_partition_name,
				  &part_info) < 0)
		return EFI_NOT_FOUND;

	bytes = (ulong)part_info.size * part_info.blksz;
	*size = bytes;

	return EFI_SUCCESS;
}

static efi_status_t preload_partition_buffer(const char *base_name, void *addr,
					     size_t size)
{
	char slotted_partition_name[GBL_PARTITION_NAME_MAX_LEN +
				    GBL_SLOT_SUFFIX_LEN + 1];
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
	ulong blkcnt;
	ulong bytes;
	efi_status_t ret;

	ret = append_slot_to_partition_name(base_name, slotted_partition_name,
					    sizeof(slotted_partition_name));
	if (ret != EFI_SUCCESS)
		return ret;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		log_err("Could not find boot device for partition %s\n", base_name);
		return EFI_DEVICE_ERROR;
	}

	if (part_get_info_by_name(dev_desc, slotted_partition_name,
				  &part_info) < 0) {
		log_err("Could not find partition %s\n",
			slotted_partition_name);
		return EFI_NOT_FOUND;
	}

	bytes = (ulong)part_info.size * part_info.blksz;
	if (bytes > size) {
		log_err("Partition buffer %s too small: need 0x%lx have 0x%zx\n",
			base_name, bytes, size);
		return EFI_DEVICE_ERROR;
	}

	blkcnt = part_info.size;
	if (blk_dread(dev_desc, part_info.start, blkcnt, addr) != blkcnt) {
		log_err("Failed to preload partition %s\n", base_name);
		return EFI_DEVICE_ERROR;
	}

	return EFI_SUCCESS;
}

static efi_status_t reserve_general_buffer(void)
{
	if (gbl_general_buffer_reserved)
		return EFI_SUCCESS;

	if (!sysmem_alloc_base(MEM_GBL,
			       GBL_BOOT_BUF_TYPE_GENERAL_ADDR,
			       GBL_BOOT_BUF_TYPE_GENERAL_SIZE +
			       GBL_PARTITION_BUF_TABLE_SIZE))
		return EFI_OUT_OF_RESOURCES;

	gbl_general_buffer_reserved = true;

	return EFI_SUCCESS;
}

static efi_status_t EFIAPI
get_partition_buffer(struct gbl_efi_boot_memory_protocol *self,
		     const char *base_name, size_t *size, void **addr,
		     gbl_efi_partition_buffer_flag *flag)
{
	efi_status_t ret;
	ulong addr_val;
	bool preloaded;

	EFI_ENTRY("%p, %p, %p, %p, %p", self, base_name, size, addr, flag);

	if (self != &gbl_efi_boot_memory_proto || !base_name ||
	    !size || !addr || !flag)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	ret = reserve_general_buffer();
	if (ret != EFI_SUCCESS)
		return EFI_EXIT(ret);

	ret = alloc_partition_table_buffer(base_name, &addr_val, size, &preloaded);
	if (ret != EFI_SUCCESS)
		return EFI_EXIT(ret);

	*addr = (void *)addr_val;
	*flag = preloaded ? GBL_EFI_PARTITION_BUFFER_FLAG_PRELOADED : 0;

	ret = remember_partition_record(base_name, *addr, *size, preloaded);
	if (ret != EFI_SUCCESS)
		return EFI_EXIT(ret);

	if (preloaded) {
		ret = preload_partition_buffer(base_name, *addr, *size);
		if (ret != EFI_SUCCESS)
			return EFI_EXIT(ret);
	}

	log_info("# GBL buffer '%s': addr=0x%08lx size=0x%zx preloaded=%u\n",
		  base_name, (ulong)*addr, *size, preloaded);

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
sync_partition_buffer(struct gbl_efi_boot_memory_protocol *self,
		      bool sync_preloaded)
{
	struct gbl_part_buf_record *record;
	size_t i;

	EFI_ENTRY("%p, %u", self, sync_preloaded);

	if (self != &gbl_efi_boot_memory_proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	if (!sync_preloaded)
		return EFI_EXIT(EFI_SUCCESS);

	for (i = 0; i < gbl_part_buf_record_cnt; i++) {
		record = &gbl_part_buf_records[i];
		if (!record->preloaded)
			continue;

		if (preload_partition_buffer(record->base_name, record->addr,
					     record->size) != EFI_SUCCESS)
			return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI
get_boot_buffer(struct gbl_efi_boot_memory_protocol *self,
		enum gbl_efi_boot_buffer_type buf_type,
		size_t *size, void **addr)
{
	const struct gbl_boot_buffer_env *cfg;
	const char *addr_str;
	ulong addr_val;
	ulong size_val;
	ulong fdt_addr_r;

	EFI_ENTRY("%p, %u, %p, %p", self, buf_type, size, addr);
	if (self != &gbl_efi_boot_memory_proto || !size || !addr)
		return EFI_EXIT(EFI_INVALID_PARAMETER);


	/* TYPE: general/fastboot */
	if (buf_type == GBL_EFI_BOOT_BUFFER_TYPE_GENERAL_LOAD) {
		efi_status_t ret = reserve_general_buffer();

		if (ret != EFI_SUCCESS)
			return EFI_EXIT(ret);

		*addr = (void *)GBL_BOOT_BUF_TYPE_GENERAL_ADDR;
		*size = GBL_BOOT_BUF_TYPE_GENERAL_SIZE;

		return EFI_EXIT(EFI_SUCCESS);
	} else if (buf_type == GBL_EFI_BOOT_BUFFER_TYPE_FASTBOOT_DOWNLOAD) {
		if (!sysmem_alloc_base(MEM_FASTBOOT,
				       CONFIG_FASTBOOT_BUF_ADDR,
				       CONFIG_FASTBOOT_BUF_SIZE))
			return EFI_EXIT(EFI_OUT_OF_RESOURCES);

		sysmem_free(CONFIG_FASTBOOT_BUF_ADDR);

		*addr = (void *)CONFIG_FASTBOOT_BUF_ADDR;
		*size = CONFIG_FASTBOOT_BUF_SIZE;

		return EFI_EXIT(EFI_SUCCESS);
	}

	/* TYPE: kernel/fdt/ramdisk/pvmfw */
	cfg = get_boot_buffer_env(buf_type);
	if (!cfg)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	addr_str = env_get(cfg->addr_env);
	size_val = env_get_ulong(cfg->size_env, 16, cfg->default_size);

	if (!addr_str || !*addr_str) {
		*addr = NULL;
		*size = size_val;
		log_debug("# GBL boot buffer %s not fixed, recommend size 0x%lx\n",
			  cfg->name, size_val);
		return EFI_EXIT(EFI_SUCCESS);
	}

	addr_val = hextoul(addr_str, NULL);
	if (!addr_val) {
		log_err("Invalid %s address from env %s: %s\n", cfg->name,
			cfg->addr_env, addr_str);
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (!aligned_addr(addr_val, cfg->alignment)) {
		log_err("# GBL boot buffer %s addr 0x%lx is not %lu-byte aligned\n",
			cfg->name, addr_val, cfg->alignment);
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (!size_val) {
		log_err("# GBL boot buffer %s has zero size in env %s\n",
			cfg->name, cfg->size_env);
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	}

	if (cfg->sysmem_id > 0) {
		fdt_addr_r = env_get_ulong("fdt_addr_r", 16, 0);

		if ((cfg->sysmem_id == MEM_FDT) &&
		    (gd->flags & GD_FLG_KDTB_READY) &&
		     gd->fdt_blob == (void *)fdt_addr_r)
			sysmem_free((phys_addr_t)gd->fdt_blob);

		if (!sysmem_alloc_base(cfg->sysmem_id, addr_val, size_val))
			return EFI_EXIT(EFI_OUT_OF_RESOURCES);
	}

	*addr = (void *)addr_val;
	*size = size_val;
	log_debug("# GBL boot buffer %s: addr=0x%lx size=0x%lx\n", cfg->name,
		  addr_val, size_val);

	return EFI_EXIT(EFI_SUCCESS);
}

static struct gbl_efi_boot_memory_protocol gbl_efi_boot_memory_proto = {
	.revision = GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION,
	.get_partition_buffer = get_partition_buffer,
	.sync_partition_buffer = sync_partition_buffer,
	.get_boot_buffer = get_boot_buffer,
};

efi_status_t gbl_efi_boot_memory_register(void)
{
	efi_status_t ret = efi_add_protocol(efi_root, &gbl_efi_boot_memory_guid,
					    &gbl_efi_boot_memory_proto);
	if (ret != EFI_SUCCESS) {
		log_err("Failed to install GBL_EFI_BOOT_MEMORY_PROTOCOL: 0x%lx\n",
			ret);
	}

	return ret;
}
