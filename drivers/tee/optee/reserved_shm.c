/*
 * Copyright 2023, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <command.h>
#include <linux/io.h>
#include <stdlib.h>
#include "optee_smc.h"

#define	SHM_PAGE_SIZE	4096
#define	SHM_ALLOC_COUNT	64

void *shm_start;
u32 shm_count;
u8 *shm_use_flag = NULL;

struct reserved_shm_alloc_flag {
	void *addr_block;
	u32 size_block;
	u8 used;
};
struct reserved_shm_alloc_flag shm_alloc_flag[SHM_ALLOC_COUNT];

static void write_used_block(void *addr, u32 size)
{
	u8 k;
	for (k = 0; k < SHM_ALLOC_COUNT; k++) {
		if (shm_alloc_flag[k].used == 0) {
			shm_alloc_flag[k].used = 1;
			shm_alloc_flag[k].addr_block = addr;
			shm_alloc_flag[k].size_block = size;
			break;
		}
	}
}

static u32 find_size_block(void *addr)
{
	u8 k;
	for (k = 0; k < SHM_ALLOC_COUNT; k++)
		if (shm_alloc_flag[k].used == 1 &&
				shm_alloc_flag[k].addr_block == addr)
			return shm_alloc_flag[k].size_block;

	return 0;
}

static void free_used_block(void *addr)
{
	u8 k;
	for (k = 0; k < SHM_ALLOC_COUNT; k++) {
		if (shm_alloc_flag[k].used == 1 &&
				shm_alloc_flag[k].addr_block == addr) {
			shm_alloc_flag[k].used = 0;
			shm_alloc_flag[k].addr_block = 0;
			shm_alloc_flag[k].size_block = 0;
			break;
		}
	}
}

void *reserved_shm_malloc(u32 size)
{
	u32 i, j, k, num;

	if (size == 0)
		size = 1;

	num = (size - 1) / SHM_PAGE_SIZE + 1;
	if (shm_count < num)
		return NULL;

	for (i = 0; i < shm_count - num; i++) {
		if (*(shm_use_flag + i) == 0) {
			for (j = 0; j < num; j++) {
				if (*(shm_use_flag + i + j) != 0)
					break;
			}
			if (j == num) {
				for (k = 0; k < num; k++) {
					*(shm_use_flag + i + k) = 1;
					memset(shm_start + (i + k) * SHM_PAGE_SIZE,
					       0, SHM_PAGE_SIZE);
				}
				write_used_block((shm_start + i * SHM_PAGE_SIZE),
						 num * SHM_PAGE_SIZE);

				return shm_start + (i * SHM_PAGE_SIZE);
			}
		}
	}

	return 0;
}

void reserved_shm_free(void *ptr)
{
	u32 i, j, num, size;

	if (ptr < shm_start)
		return;

	size = find_size_block(ptr);
	if (size == 0)
		return;

	i = (ptr - shm_start) / SHM_PAGE_SIZE;
	num = (size - 1) / SHM_PAGE_SIZE + 1;

	for (j = 0; j < num; j++) {
		*(shm_use_flag + i + j) = 0;
		memset(shm_start + (i + j) * SHM_PAGE_SIZE, 0, SHM_PAGE_SIZE);
	}
	free_used_block(ptr);
}

int reserved_shm_init(struct optee_smc_get_shm_config_result config)
{
	void *start = phys_to_virt(config.start);
	u32 size = config.size;

	debug("%s: start=%p size=0x%x\n", __func__, start, size);

	if (start == NULL || size == 0) {
		printf("%s: invalid parameter\n", __func__);
		return -1;
	}

	memset(start, 0, size);

	shm_start = start;
	shm_count = size / SHM_PAGE_SIZE;
	if (shm_use_flag == NULL) {
		shm_use_flag = malloc(shm_count);
		if (shm_use_flag == NULL) {
			printf("%s: malloc failed!\n", __func__);
			return -1;
		}
	}
	memset(shm_use_flag, 0, shm_count);
	memset(shm_alloc_flag, 0, sizeof(shm_alloc_flag));
	return 0;
}
