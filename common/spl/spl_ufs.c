// SPDX-License-Identifier: GPL-2.0+
/*
 * Rockchip UFS Host Controller driver
 *
 * Copyright (C) 2024 Rockchip Electronics Co.Ltd.
 */

#include <common.h>
#include <asm/u-boot.h>
#include <blk.h>
#include <errno.h>
#include <image.h>
#include <part.h>
#include <scsi.h>
#include <ufs.h>
#include <spl.h>
#include <spl_load.h>

DECLARE_GLOBAL_DATA_PTR;

static ulong h_spl_load_read(struct spl_load_info *load, ulong off,
			     ulong size, void *buf)
{
	struct blk_desc *bd = load->priv;
	lbaint_t sector = off >> bd->log2blksz;
	lbaint_t count = size >> bd->log2blksz;

	return blk_dread(bd, sector, count, buf) << bd->log2blksz;
}

static int spl_ufs_load_image(struct spl_image_info *spl_image,
			       struct spl_boot_device *bootdev)
{
	lbaint_t image_sector = CONFIG_SYS_UFS_RAW_MODE_U_BOOT_SECTOR;
	int ret;
	struct blk_desc *desc;
	struct spl_load_info load;

	/* try to recognize storage devices immediately */
	ufs_probe();
	scsi_scan(true);

	desc = blk_get_devnum_by_uclass_id(UCLASS_SCSI, 0);
	if (!desc)
		return -ENODEV;

	load.priv = desc;
	load.bl_len = desc->blksz;
	load.read = h_spl_load_read;

#ifdef CONFIG_SPL_LIBDISK_SUPPORT
	struct disk_partition info;

	ret = part_get_info_by_name(desc, PART_UBOOT, &info);
	if (ret > 0)
		image_sector = info.start;
#endif

	spl_load_init(&load, h_spl_load_read, desc, desc->blksz);
	return spl_load(spl_image, bootdev, &load, 0, image_sector << desc->log2blksz);
}

SPL_LOAD_IMAGE_METHOD("UFS", 0, BOOT_DEVICE_UFS, spl_ufs_load_image);
