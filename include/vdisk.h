/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#ifndef __RVD_BLOCK_DEV__
#define __RVD_BLOCK_DEV__

struct rvd_block_dev {
	struct blk_desc *host_blk_dev;
	unsigned int start_sector;
	unsigned int num_sectors;
};

int rvd_init(void);

#endif
