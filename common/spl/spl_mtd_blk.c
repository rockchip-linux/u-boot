// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <blk.h>
#include <dm.h>
#include <errno.h>
#include <image.h>
#include <malloc.h>
#include <mtd_blk.h>
#include <part.h>
#include <spl.h>
#include <spl_ab.h>
#include <spl_load.h>
#include <asm/u-boot.h>
#include <dm/device-internal.h>
#include <linux/compiler.h>
#include <linux/mtd/mtd.h>

static int spl_mtd_get_device_index(u32 boot_device)
{
	switch (boot_device) {
	case BOOT_DEVICE_MTD_BLK_NAND:
		return 0;
	case BOOT_DEVICE_MTD_BLK_SPI_NAND:
		return 1;
	case BOOT_DEVICE_MTD_BLK_SPI_NOR:
		return 2;
	}

#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	printf("spl: unsupported mtd boot device.\n");
#endif

	return -ENODEV;
}

struct blk_desc *find_mtd_device(int dev_num)
{
	struct udevice *dev;
	struct blk_desc *desc;
	int ret;

	ret = blk_find_device(UCLASS_MTD, dev_num, &dev);

	if (ret) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
		printf("MTD Device %d not found\n", dev_num);
#endif
		return NULL;
	}

	ret = device_probe(dev);
	if (ret) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
		printf("MTD Device %d not found\n", dev_num);
#endif
		return NULL;
	}

	desc = dev_get_uclass_plat(dev);
	if (!desc)
		return NULL;

	return desc;
}

static ulong h_spl_load_read(struct spl_load_info *load, ulong off,
			     ulong size, void *buf)
{
	struct blk_desc *bd = load->priv;
	lbaint_t sector = off >> bd->log2blksz;
	lbaint_t count = size >> bd->log2blksz;

	return blk_dread(bd, sector, count, buf) << bd->log2blksz;
}

int spl_mtd_load_image(struct spl_image_info *spl_image,
		       struct spl_boot_device *bootdev)
{
	lbaint_t image_sector = CONFIG_MTD_BLK_U_BOOT_OFFS;
	struct blk_desc *desc;
	int ret = -1;
	struct spl_load_info load;

	desc = find_mtd_device(spl_mtd_get_device_index(bootdev->boot_device));
	if (!desc)
		return -ENODEV;

	load.priv = desc;
	load.bl_len = desc->blksz;
	load.read = h_spl_load_read;

#ifdef CONFIG_SPL_LIBDISK_SUPPORT
	struct disk_partition info;

	mtd_blk_map_partitions(desc);
	ret = part_get_info_by_name(desc, PART_UBOOT, &info);
	if (ret > 0)
		image_sector = info.start;

#endif
	spl_load_init(&load, h_spl_load_read, desc, desc->blksz);
	return spl_load(spl_image, bootdev, &load, 0, image_sector << desc->log2blksz);
}

SPL_LOAD_IMAGE_METHOD("MTD0", 0, BOOT_DEVICE_MTD_BLK_NAND, spl_mtd_load_image);
SPL_LOAD_IMAGE_METHOD("MTD1", 0, BOOT_DEVICE_MTD_BLK_SPI_NAND, spl_mtd_load_image);
SPL_LOAD_IMAGE_METHOD("MTD2", 0, BOOT_DEVICE_MTD_BLK_SPI_NOR, spl_mtd_load_image);
