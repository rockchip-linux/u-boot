/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <part.h>
#include <spl.h>

static ulong h_spl_load_read(struct spl_load_info *load, ulong sector,
			     ulong count, void *buf)
{
	return blk_dread(load->dev, sector, count, buf);
}

int spl_ramdisk_load_image(struct spl_image_info *spl_image,
			   struct spl_boot_device *bootdev)
{
	struct image_header *header;
	struct spl_load_info load;
	struct blk_desc *desc;
	const char *partition = "uboot";
	disk_partition_t info;
	ulong count;
	int ret = 0;

	desc = blk_get_devnum_by_type(IF_TYPE_RAMDISK, 0);
	if (!desc) {
		printf("No ramdisk 0 device\n");
		return -ENODEV;
	}

	ret = part_get_info_by_name(desc, partition, &info);
	if (ret < 0) {
		printf("No '%s' paritition\n", partition);
		return -EINVAL;
	}

	header = (void *)(CONFIG_SYS_TEXT_BASE - sizeof(struct image_header));
	count = blk_dread(desc, info.start, 1, header);
	if (count != 1)
		return -EIO;

	load.dev = desc;
	load.priv = NULL;
	load.filename = NULL;
	load.bl_len = desc->blksz;
	load.read = h_spl_load_read;

	return spl_load_simple_fit(spl_image, &load, info.start, header);
}

SPL_LOAD_IMAGE_METHOD("RAMDISK", 0, BOOT_DEVICE_RAM, spl_ramdisk_load_image);

