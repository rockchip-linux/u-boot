/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <blk.h>
#include <dm/uclass.h>
#include <efi_loader.h>
#include <image.h>
#include <hang.h>
#include <malloc.h>
#include <mmc.h>
#include <part.h>
#include <sysmem.h>
#include <u-boot/uuid.h>
#include <asm/cache.h>

__weak void rockchip_capsule_update_board_setup(void) {}

#if IS_ENABLED(CONFIG_EFI_HAVE_CAPSULE_SUPPORT) && IS_ENABLED(CONFIG_EFI_PARTITION)

#define DFU_ALT_BUF_LEN			SZ_1K

static struct efi_fw_image rockchip_fw_images[] = {
	{
		.fw_name = u"ROCKCHIP-FIT",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = NULL,
	.num_images = ARRAY_SIZE(rockchip_fw_images),
	.images = rockchip_fw_images,
};

static struct efi_fw_image *fw_images;

static bool updatable_image(struct disk_partition *info)
{
	int i;
	bool ret = false;
	efi_guid_t image_type_guid;

	uuid_str_to_bin(info->type_guid, image_type_guid.b,
			UUID_STR_FORMAT_GUID);

	for (i = 0; i < update_info.num_images; i++) {
		if (!guidcmp(&fw_images[i].image_type_id, &image_type_guid)) {
			ret = true;
			break;
		}
	}

	return ret;
}

static void set_image_index(struct disk_partition *info, int index)
{
	int i;
	efi_guid_t image_type_guid;

	uuid_str_to_bin(info->type_guid, image_type_guid.b,
			UUID_STR_FORMAT_GUID);

	for (i = 0; i < update_info.num_images; i++) {
		if (!guidcmp(&fw_images[i].image_type_id, &image_type_guid)) {
			fw_images[i].image_index = index;
			break;
		}
	}
}

static int get_mmc_desc(struct blk_desc **desc)
{
	int ret;
	struct mmc *mmc;
	struct udevice *dev;

	/*
	 * For now the firmware images are assumed to
	 * be on the SD card
	 */
	ret = uclass_get_device(UCLASS_MMC, 1, &dev);
	if (ret)
		return -1;

	mmc = mmc_get_mmc_dev(dev);
	if (!mmc)
		return -ENODEV;

	if ((ret = mmc_init(mmc)))
		return ret;

	*desc = mmc_get_blk_desc(mmc);
	if (!*desc)
		return -1;

	return 0;
}

void gpt_capsule_update_setup(void)
{
	int p, i, ret;
	struct disk_partition info;
	struct blk_desc *desc = NULL;

	fw_images = update_info.images;
	rockchip_capsule_update_board_setup();

	ret = get_mmc_desc(&desc);
	if (ret) {
		log_err("Unable to get mmc desc\n");
		return;
	}

	for (p = 1, i = 1; p <= MAX_SEARCH_PARTITIONS; p++) {
		if (part_get_info(desc, p, &info))
			continue;

		/*
		 * Since we have a GPT partitioned device, the updatable
		 * images could be stored in any order. Populate the
		 * image_index at runtime.
		 */
		if (updatable_image(&info)) {
			set_image_index(&info, i);
			i++;
		}
	}
}
#endif /* CONFIG_EFI_HAVE_CAPSULE_SUPPORT && CONFIG_EFI_PARTITION */
