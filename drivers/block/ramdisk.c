// SPDX-License-Identifier:     GPL-2.0+
/*
 * (C) Copyright 2018 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <ramdisk.h>
#include <dm/device-internal.h>
#include <asm/arch/rk_atags.h>

int dm_ramdisk_is_enabled(void)
{
	return 1;
}

static lbaint_t real_lba(struct udevice *dev, struct blk_desc *desc, lbaint_t lba)
{
	struct ramdisk_info *ri = dev_get_priv(dev);
	lbaint_t vbgpt_lba0 = desc->lba - 33;

	if (lba >= vbgpt_lba0)
		lba = ri->bgpt_lba + (lba - vbgpt_lba0);

	return lba;
}

static ulong ramdisk_bread(struct udevice *dev, lbaint_t start,
			   lbaint_t blkcnt, void *dst)
{
	struct blk_desc *desc = dev_get_uclass_platdata(dev);
	struct ramdisk_info *ri = dev_get_priv(dev);
	ulong byte_size, byte_start;

	byte_start = real_lba(dev, desc, start) * desc->blksz + ri->base;
	byte_size = blkcnt * desc->blksz;

	if ((ulong)dst != byte_start)
		memcpy((char *)dst, (char *)byte_start, byte_size);

	debug("%s: start: 0x%08lx -> 0x%08lx(0x%08lx), "
	      "blkcnt: 0x%08lx -> 0x%08lx, buffer: 0x%08lx\n",
	      __func__, (ulong)start, byte_start,
	      start * desc->blksz + ri->base,
	      (ulong)blkcnt, byte_size, (ulong)dst);

	return blkcnt;
}

static ulong ramdisk_bwrite(struct udevice *dev, lbaint_t start,
			    lbaint_t blkcnt, const void *src)
{
	struct blk_desc *desc = dev_get_uclass_platdata(dev);
	struct ramdisk_info *ri = dev_get_priv(dev);
	ulong byte_size, byte_start;

	byte_start = real_lba(dev, desc, start) * desc->blksz + ri->base;
	byte_size = blkcnt * desc->blksz;

	if ((ulong)src != byte_start)
		memcpy((char *)byte_start, (char *)src, byte_size);

	debug("%s: start: 0x%08lx -> 0x%08lx(0x%08lx), "
	      "blkcnt: 0x%08lx -> 0x%08lx, buffer: 0x%08lx\n",
	      __func__, (ulong)start, byte_start,
	      start * desc->blksz + ri->base,
	      (ulong)blkcnt, byte_size, (ulong)src);

	return blkcnt;
}

static ulong ramdisk_berase(struct udevice *dev,
			    lbaint_t start, lbaint_t blkcnt)
{
	struct blk_desc *desc = dev_get_uclass_platdata(dev);
	struct ramdisk_info *ri = dev_get_priv(dev);
	ulong byte_size, byte_start;

	byte_start = real_lba(dev, desc, start) * desc->blksz + ri->base;
	byte_size = blkcnt * desc->blksz;

	memset((char *)byte_start, 0, byte_size);

	debug("%s: start: 0x%08lx -> 0x%08lx(0x%08lx), "
	      "blkcnt: 0x%08lx -> 0x%08lx\n",
	      __func__, (ulong)start, byte_start,
	      start * desc->blksz + ri->base,
	      (ulong)blkcnt, byte_size);

	return blkcnt;
}

static int ramdisk_bind(struct udevice *dev)
{
	struct udevice *bdev;
	int ret;

	ret = blk_create_devicef(dev, "ramdisk_blk", "blk",
				 IF_TYPE_RAMDISK, 0, 512, 0, &bdev);
	if (ret) {
		printf("failed to create blk device, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int ramdisk_blk_probe(struct udevice *dev)
{
	struct blk_desc *bdesc = dev_get_uclass_platdata(dev);
	struct ramdisk_info *ri = dev_get_priv(dev);
	void *fw_addr = (void *)CONFIG_MOS_BOOTDEV_SHARED_ADDR +
				CONFIG_MOS_BOOTDEV_SHARED_ARGS_SIZE;

	memcpy(ri, fw_addr, sizeof(*ri));
	if (ri->magic != RAMDISK_INFO_MAGIC) {
		printf("No available ramdisk info!\n");
		return -EINVAL;
	}

	bdesc->lba = ri->lba;
	bdesc->rawlba = ri->lba;

	return 0;
}

static const struct blk_ops ramdisk_blk_ops = {
	.read	= ramdisk_bread,
	.write	= ramdisk_bwrite,
	.erase	= ramdisk_berase,
};

static const struct udevice_id ramdisk_ids[] = {
	{ .compatible = "ramdisk" },
	{ }
};

U_BOOT_DRIVER(ramdisk)	= {
	.name		= "ramdisk",
	.id		= UCLASS_RAMDISK,
	.bind		= ramdisk_bind,
	.of_match	= ramdisk_ids,
};

U_BOOT_DEVICE(ramdisk)	= {
	.name		= "ramdisk",
};

U_BOOT_DRIVER(ramdisk_blk) = {
	.name		= "ramdisk_blk",
	.id		= UCLASS_BLK,
	.probe		= ramdisk_blk_probe,
	.ops		= &ramdisk_blk_ops,
	.priv_auto_alloc_size = sizeof(struct ramdisk_info),
};

UCLASS_DRIVER(ramdisk)	= {
	.name		= "ramdisk",
	.id		= UCLASS_RAMDISK,
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
};
