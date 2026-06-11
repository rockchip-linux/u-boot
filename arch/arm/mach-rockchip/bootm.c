// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd.
 */
#include <common.h>
#include <android_bootloader.h>
#include <blk.h>
#include <env.h>
#include <fdt_support.h>
#include <hotkey.h>
#include <image.h>
#include <malloc.h>
#include <part.h>
#include <sysmem.h>
#include <timestamp.h>
#include <version.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/atags.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/common.h>

DECLARE_GLOBAL_DATA_PTR;

extern struct bootm_headers images;

static int bootm_image_populate_dtb(void *img)
{
	ulong fdt_addr_r = env_get_ulong("fdt_addr_r", 16, 0);

	if ((gd->flags & GD_FLG_KDTB_READY) && gd->fdt_blob == (void *)fdt_addr_r)
		sysmem_free((phys_addr_t)gd->fdt_blob);
	else
		gd->fdt_blob = (void *)fdt_addr_r;

	return rockchip_ram_read_dtb_file(img, (void *)gd->fdt_blob);
}

/*
 * Implement it to support CLI command:
 *   - Android: bootm [aosp addr]
 *   - FIT:     bootm [fit addr]
 *   - uImage:  bootm [uimage addr]
 *
 * Purpose:
 *   - The original bootm command args require fdt addr on AOSP,
 *     which is not flexible on rockchip boot/recovery.img.
 *   - Take Android/FIT/uImage image into sysmem management to avoid image
 *     memory overlap.
 */
#if defined(CONFIG_ANDROID_BOOT_IMAGE) ||	\
	defined(CONFIG_ROCKCHIP_FIT_IMAGE) ||	\
	defined(CONFIG_ROCKCHIP_UIMAGE)
int board_do_bootm(int argc, char * const argv[])
{
	int format;
	void *img;

	/* only 'bootm' full image goes further */
	if (argc != 2)
		return 0;

	img = (void *)simple_strtoul(argv[1], NULL, 16);
	format = (genimg_get_format(img));

	/* Android */
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	if (format == IMAGE_FORMAT_ANDROID) {
		struct andr_img_hdr *hdr;
		ulong load_addr;
		ulong size;
		int ret;

		hdr = (struct andr_img_hdr *)img;
		printf("BOOTM: transferring to board Android\n");

		load_addr = env_get_ulong("kernel_addr_r", 16, 0);
		load_addr -= hdr->page_size;
		size = android_image_get_end(hdr) - (ulong)hdr;

		if (!sysmem_alloc_base(MEM_ANDROID, (ulong)hdr, size))
			return -ENOMEM;

		ret = bootm_image_populate_dtb(img);
		if (ret) {
			printf("bootm can't read dtb, ret=%d\n", ret);
			return ret;
		}

		ret = android_image_memcpy_separate(hdr, &load_addr);
		if (ret) {
			printf("board do bootm failed, ret=%d\n", ret);
			return ret;
		}

		return android_bootloader_boot_kernel(load_addr);
	}
#endif

	/* FIT */
#ifdef CONFIG_FIT
	if (format == IMAGE_FORMAT_FIT) {
		char boot_cmd[64];
		int ret;

		printf("BOOTM: transferring to board FIT\n");

		ret = bootm_image_populate_dtb(img);
		if (ret) {
			printf("bootm can't read dtb, ret=%d\n", ret);
			return ret;
		}
		snprintf(boot_cmd, sizeof(boot_cmd), "boot_fit %s", argv[1]);
		return run_command(boot_cmd, 0);
	}
#endif

	/* uImage */
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
	if (format == IMAGE_FORMAT_LEGACY &&
	    image_get_type(img) == IH_TYPE_MULTI) {
		char boot_cmd[64];

		printf("BOOTM: transferring to board uImage\n");
		snprintf(boot_cmd, sizeof(boot_cmd), "boot_uimage %s", argv[1]);
		return run_command(boot_cmd, 0);
	}
#endif
	return 0;
}
#endif

int bootm_board_start(void)
{
	/* sysmem */
	hotkey_run(HK_SYSMEM);
	sysmem_overflow_check();

	return 0;
}
