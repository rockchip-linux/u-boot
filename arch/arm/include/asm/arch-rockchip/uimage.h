/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __ROCKCHIP_UIMAGE_H_
#define __ROCKCHIP_UIMAGE_H_

#include <image.h>

#define UIMG_I(fmt, args...)	printf("uImage: "fmt, ##args)

void *uimage_load_bootables(void);
int uimage_sysmem_free_each(struct legacy_img_hdr *img, u32 ramdisk_sz);
int uimage_sysmem_reserve_each(struct legacy_img_hdr *hdr, u32 *ramdisk_sz);
int uimage_init_resource(struct blk_desc *dev_desc);
#endif

