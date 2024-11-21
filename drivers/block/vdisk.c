/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <vdisk.h>

extern struct blk_desc *rockchip_get_bootdev(void);

static ulong rvd_bread(struct udevice *udev, lbaint_t start, lbaint_t blkcnt, void *dst)
{
	struct rvd_block_dev *rvd_dev = dev_get_priv(udev);
	struct blk_desc *blk_dev = rvd_dev->host_blk_dev;
	struct blk_desc *rvd_desc = dev_get_uclass_platdata(udev);
	unsigned char op_flag = blk_dev->op_flag;
	lbaint_t blks;

	blk_dev->op_flag = rvd_desc->op_flag;
	blks = blk_dread(blk_dev, start + rvd_dev->start_sector, blkcnt, dst);
	blk_dev->op_flag = op_flag;

	return blks;
}

static ulong rvd_bwrite(struct udevice *udev, lbaint_t start, lbaint_t blkcnt, const void *src)
{
	struct rvd_block_dev *rvd_dev = dev_get_priv(udev);
	struct blk_desc *blk_dev = rvd_dev->host_blk_dev;
	struct blk_desc *rvd_desc = dev_get_uclass_platdata(udev);
	unsigned char op_flag = blk_dev->op_flag;
	lbaint_t blks;

	blk_dev->op_flag = rvd_desc->op_flag;
	blks = blk_dwrite(blk_dev, start + rvd_dev->start_sector, blkcnt, src);
	blk_dev->op_flag = op_flag;

	return blks;
}

static ulong rvd_berase(struct udevice *udev, lbaint_t start, lbaint_t blkcnt)
{
	struct rvd_block_dev *rvd_dev = dev_get_priv(udev);
	struct blk_desc *blk_dev = rvd_dev->host_blk_dev;

	return blk_derase(blk_dev, start + rvd_dev->start_sector, blkcnt);
}

static int rvd_dev_bind(struct blk_desc *blk_dev, const char *part_name)
{
	struct rvd_block_dev *rvd_dev;
	struct udevice *parent;
	struct udevice *dev;
	disk_partition_t part_info;
	int ret;

	/* Remove and unbind the old device, if any */
	ret = blk_get_device(IF_TYPE_RVD, 0, &dev);
	if (ret == 0) {
		ret = device_remove(dev, DM_REMOVE_NORMAL);
		if (ret)
			return ret;
		ret = device_unbind(dev);
		if (ret)
			return ret;
	} else if (ret != -ENODEV) {
		return ret;
	}

	ret = uclass_get_device_by_name(UCLASS_RVD, "rvd", &parent);
	if (ret) {
		printf("No rvd device, ret=%d\n", ret);
		return ret;
	}

	ret = part_get_info_by_name(blk_dev, part_name, &part_info);
	if (ret < 0) {
		printf("No '%s' partition\n", part_name);
		return ret;
	}

	ret = blk_create_device(parent, "rvd_blk", "blk",
				IF_TYPE_RVD, 0, part_info.blksz,
				part_info.size * part_info.blksz, &dev);
	if (ret)
		return ret;

	ret = device_probe(dev);
	if (ret) {
		device_unbind(dev);
		return ret;
	}

	rvd_dev = dev_get_priv(dev);
	rvd_dev->host_blk_dev = blk_dev;
	rvd_dev->start_sector = part_info.start;
	rvd_dev->num_sectors = part_info.size;

	return blk_prepare_device(dev);
}

int rvd_init(void)
{
	static struct blk_desc *desc;
	int ret;

	desc = rockchip_get_bootdev();
	if (!desc)
		return -ENODEV;

	ret = rvd_dev_bind(desc, "secondary");
	if (!ret)
		printf("rvd init ok\n");
	else
		printf("rvd init failed, ret=%d\n", ret);

	return ret;
}

static const struct blk_ops rvd_blk_ops = {
	.read	= rvd_bread,
	.write	= rvd_bwrite,
	.erase	= rvd_berase,
};

U_BOOT_DRIVER(rvd) = {
	.id		= UCLASS_RVD,
	.name		= "rvd",
};

U_BOOT_DEVICE(rvd) = {
	.name		= "rvd",
};

U_BOOT_DRIVER(rvd_blk)  = {
	.name		= "rvd_blk",
	.id		= UCLASS_BLK,
	.ops		= &rvd_blk_ops,
	.priv_auto_alloc_size	= sizeof(struct rvd_block_dev),
};

UCLASS_DRIVER(vdisk) = {
	.id		= UCLASS_RVD,
	.name		= "vdisk",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
};
