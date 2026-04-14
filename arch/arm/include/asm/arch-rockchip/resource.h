/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __RESC_IMG_H_
#define __RESC_IMG_H_

#include <blk.h>
#include <linux/list.h>
#include <asm/global_data.h>

#define MAX_FILE_NAME_LEN		220
#define MAX_HASH_LEN			32
#define DTB_SUFFIX			".dtb"

struct resource_file {
	char		name[MAX_FILE_NAME_LEN];
	ulong		blk_start;
	ulong		blk_offset;
	char		hash[MAX_HASH_LEN];
	uint32_t	hash_size;
	uint32_t	size;		/* in byte */
	bool		in_ram;
	struct list_head link;
};

extern struct list_head entry_head;

/*
 * resource_destroy() - Destroy resource file list
 */
void resource_destroy(void);

/*
 * resource_setup_ram_list() - setup resource file list by given resource image.
 *
 * @dev_desc: boot device
 * @hdr: resource file hdr
 *
 * return 0 on success, otherwise fail.
 */
int resource_setup_ram_list(struct blk_desc *dev_desc, void *hdr);

/*
 * resource_read_file() - read file from resource.
 *
 * @buf: destination buf to store file data
 * @name: file name
 * @blk_offset: blocks offset in the file, 1 block = 512 bytes
 * @len: the size(by bytes) of file to read
 *
 * return the length of read data.
 */
int resource_read_file(void *buf, const char *name, int blk_offset, int len);

/*
 * resource_read_dtb() - read dtb file
 *
 * @fdt_addr: destination buf to store dtb file
 * @hash: hash value buffer
 * @hash_size: hash value length
 */
int resource_read_dtb(void *fdt_addr, char **hash, int *hash_size);

/*
 * resource_read_ram_dtb() - read dtb file from a resource image in memory
 *
 * @resc_addr: resource image base address
 * @fdt_addr: destination buf to store dtb file
 */
int resource_read_ram_dtb(void *resc_addr, void *fdt_addr);

#endif
