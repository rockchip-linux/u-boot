/*
 * Copyright 2012 Google Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __DRIVERS_STORAGE_BLOCKDEV_H__
#define __DRIVERS_STORAGE_BLOCKDEV_H__

#include <asm/io.h>

//#include "base/list.h"
//#include "drivers/storage/stream.h"

typedef uint64_t lba_t;

/*typedef struct BlockDevOps {
	lba_t (*read)(struct BlockDevOps *me, lba_t start, lba_t count,
		      void *buffer);
	lba_t (*write)(struct BlockDevOps *me, lba_t start, lba_t count,
		       const void *buffer);
	lba_t (*fill_write)(struct BlockDevOps *me, lba_t start, lba_t count,
			    uint32_t fill_pattern);
	lba_t (*erase)(struct BlockDevOps *me, lba_t start, lba_t count);
	StreamOps *(*new_stream)(struct BlockDevOps *me, lba_t start,
				 lba_t count);
} BlockDevOps;*/

typedef struct BlockDevOps {
    int card_id;
} BlockDevOps;

typedef struct BlockDev {
	BlockDevOps ops;

	const char *name;
	int removable;
	int external_gpt;
	unsigned int block_size;
	/* If external_gpt = 0, then stream_block_count may be 0, indicating
	 * that the block_count value applies for both read/write and streams */
	lba_t block_count;		/* size addressable by read/write */
	lba_t stream_block_count;	/* size addressible by new_stream */

	//ListNode list_node;
} BlockDev;

//extern ListNode fixed_block_devices;
//extern ListNode removable_block_devices;

typedef struct BlockDevCtrlrOps {
	int (*update)(struct BlockDevCtrlrOps *me);
	/*
	 * Check if a block device is owned by the ctrlr. 1 = success, 0 =
	 * failure
	 */
	int (*is_bdev_owned)(struct BlockDevCtrlrOps *me, BlockDev *bdev);
} BlockDevCtrlrOps;

typedef struct BlockDevCtrlr {
	BlockDevCtrlrOps ops;

	int need_update;
	//ListNode list_node;
} BlockDevCtrlr;

//extern ListNode fixed_block_dev_controllers;
//extern ListNode removable_block_dev_controllers;

//StreamOps *new_simple_stream(BlockDevOps *me, lba_t start, lba_t count);

typedef enum {
	BLOCKDEV_FIXED,
	BLOCKDEV_REMOVABLE,
} blockdev_type_t;

//int get_all_bdevs(blockdev_type_t type, ListNode **bdevs);

#endif /* __DRIVERS_STORAGE_BLOCKDEV_H__ */
