/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <android_bootloader.h>
#include <android_image.h>
#include <dm.h>
#include <fs.h>
#include <image.h>
#include <sysmem.h>
#include <asm/arch-rockchip/common.h>
#include <asm/arch-rockchip/fit.h>
#include <asm/arch-rockchip/resource.h>
#include <linux/kernel.h>
#include <u-boot/hash.h>

DECLARE_GLOBAL_DATA_PTR;

enum {
	DTB_IN_DISTRO,
	DTB_IN_RESOURCE,
	DTB_IN_FIT,
	DTB_IN_END,
};

__weak int rk_board_early_fdt_fixup(void *blob)
{
	return 0;
}

#ifdef CONFIG_ROCKCHIP_RESOURCE_IMAGE
static int fdt_check_hash(void *fdt_addr, u32 fdt_size,
			  char *hash_cmp, u32 hash_size)
{
	struct udevice *dev;
	enum HASH_ALGO algo;
	uchar hash[32];
	int ret = 0;

	ret = uclass_get_device(UCLASS_HASH, 0, &dev);
	if (ret) {
		printf("No Hash device, ret=%d\n", ret);
		return ret;
	}

	if (hash_size == 20)
		algo = HASH_ALGO_SHA1;
	else if (hash_size == 32)
		algo = HASH_ALGO_SHA256;
	else
		return -EINVAL;

	ret = hash_digest(dev, algo, fdt_addr, fdt_size, hash);
	if (ret) {
		printf("Hash-%d calc failed, ret=%d\n", algo, ret);
		return ret;
	}

	printf("HASH: ");
	if (memcmp(hash, hash_cmp, hash_size)) {
		printf("error\n");
		return -EBADF;
	}

	printf("OK\n");

	return 0;
}
#endif

#if defined(CONFIG_EARLY_DISTRO_DTB)
static int distro_dtb_get(void *fdt_addr)
{
	const char *cmd = "part list ${devtype} ${devnum} -bootable devplist";
	char *devnum, *devtype, *devplist;
	char devnum_part[12];
	char fdt_hex_str[19];
	char *fs_argv[5];

	if (!plat_bootdev() || !fdt_addr)
		return -ENODEV;

	if (run_command_list(cmd, -1, 0)) {
		printf("Failed to find -bootable\n");
		return -EINVAL;
	}

	devplist = env_get("devplist");
	if (!devplist)
		devplist = "1";

	devtype = env_get("devtype");
	devnum = env_get("devnum");
	sprintf(devnum_part, "%s:%s", devnum, devplist);
	sprintf(fdt_hex_str, "0x%lx", (ulong)fdt_addr);

	fs_argv[0] = "load";
	fs_argv[1] = devtype,
	fs_argv[2] = devnum_part;
	fs_argv[3] = fdt_hex_str;
	fs_argv[4] = CONFIG_EARLY_DISTRO_DTB_PATH;

	if (do_load(NULL, 0, 5, fs_argv, FS_TYPE_ANY))
		return -EIO;

	if (fdt_check_header(fdt_addr))
		return -EBADF;

	printf("DTB(Distro): %s\n", CONFIG_EARLY_DISTRO_DTB_PATH);

	return 0;
}
#endif

static int dtb_scan_get(void *fdt, int pos)
{
	if (pos == DTB_IN_DISTRO) {
#ifdef CONFIG_EARLY_DISTRO_DTB
		return distro_dtb_get(fdt);
#endif
	} else if (pos == DTB_IN_RESOURCE) {
#ifdef CONFIG_ROCKCHIP_RESOURCE_IMAGE
		int hash_size = 0;
		char *hash;
		u32 ret;

		ret = resource_read_dtb(fdt, &hash, &hash_size);
		if (ret) {
			printf("Failed to load DTB, ret=%d\n", ret);
			return ret;
		}

		if (fdt_check_header(fdt)) {
			printf("Invalid DTB magic !\n");
			return -EBADF;
		}

		if (hash_size && fdt_check_hash(fdt, fdt_totalsize(fdt), hash, hash_size)) {
			printf("Invalid DTB hash !\n");
			return -EBADF;
		}

		return 0;
#endif
	} else if (pos == DTB_IN_FIT) {
#if defined(CONFIG_ROCKCHIP_FIT_IMAGE) && !defined(CONFIG_ROCKCHIP_RESOURCE_IMAGE)
		return fit_image_read_dtb(fdt);
#endif
	}

	return -EINVAL;
}

int rockchip_read_dtb_file(void *fdt)
{
	int locate, ret;
	int size;

	for (locate = 0; locate < DTB_IN_END; locate++) {
		ret = dtb_scan_get(fdt, locate);
		if (!ret)
			break;
	}
	if (ret) {
		printf("No valid DTB, ret=%d\n", ret);
		return ret;
	}

	/* reserved memory */
	size = fdt_totalsize(fdt);
	if (!sysmem_alloc_base(MEM_FDT, (phys_addr_t)fdt,
		ALIGN(size, RK_BLK_SIZE) + CONFIG_SYS_FDT_PAD))
		return -ENOMEM;

	/* early fixup */
	rk_board_early_fdt_fixup(fdt);

	/* dtbo overlay */
#ifdef CONFIG_ANDROID_DTBO_SUPPORT
	android_fdt_overlay_apply((void *)fdt);
#endif

	return 0;
}

int rockchip_ram_read_dtb_file(void *img, void *fdt)
{
	int format;

	format = (genimg_get_format(img));
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	if (format == IMAGE_FORMAT_ANDROID) {
		struct andr_img_hdr *hdr = img;
		struct blk_desc *dev_desc;
		ulong offset;

		dev_desc = plat_bootdev();
		if (!dev_desc)
			return -ENODEV;

		offset = hdr->page_size + ALIGN(hdr->kernel_size, hdr->page_size) +
			ALIGN(hdr->ramdisk_size, hdr->page_size);
#ifdef CONFIG_ROCKCHIP_RESOURCE_IMAGE
		int ret;

		ret = resource_setup_ram_list(dev_desc, (void *)hdr + offset);
		if (ret)
			return ret;

		return rockchip_read_dtb_file((void *)fdt);
#else
		if (fdt_check_header((void *)offset))
			return -EINVAL;

		memcpy(fdt, (char *)offset, fdt_totalsize(offset));
		if (!sysmem_alloc_base(MEM_FDT, (phys_addr_t)fdt,
			ALIGN(fdt_totalsize(fdt), RK_BLK_SIZE) + CONFIG_SYS_FDT_PAD))
			return -ENOMEM;

		return 0;
#endif
	}
#endif
#ifdef CONFIG_FIT
	if (format == IMAGE_FORMAT_FIT) {
		const void *data;
		size_t size;
		int noffset, ret;
#ifdef CONFIG_ROCKCHIP_RESOURCE_IMAGE
		const char *path = "/images/resource";
#else
		const char *path = "/images/fdt";
#endif

		noffset = fdt_path_offset(img, path);
		if (noffset < 0)
			return noffset;

#ifdef CONFIG_ROCKCHIP_RESOURCE_IMAGE
		struct blk_desc *dev_desc;
		ret = fit_image_get_data(img, noffset, &data, &size);
		if (ret < 0)
			return ret;

	printf("####### %s, %d, resc addr: %p\n", __func__, __LINE__, data);
		dev_desc = plat_bootdev();
		if (!dev_desc)
			return -ENODEV;

		ret = resource_setup_ram_list(dev_desc, (void *)data);
		if (ret) {
			printf("resource_setup_ram_list fail, ret=%d\n", ret);
			return ret;
		}

		return rockchip_read_dtb_file((void *)fdt);
#else

		ret = fit_image_get_data(img, noffset, &data, &size);
		if (ret)
			return ret;

		if (fdt_check_header(data))
			return -EINVAL;

		memcpy(fdt, data, size);
		if (!sysmem_alloc_base(MEM_FDT, (phys_addr_t)fdt,
			ALIGN(fdt_totalsize(fdt), RK_BLK_SIZE) + CONFIG_SYS_FDT_PAD))
			return -ENOMEM;

		printf("Load DTB from 'images/fdt'\n");

		return 0;
#endif
	}
#endif

	return -EINVAL;
}
