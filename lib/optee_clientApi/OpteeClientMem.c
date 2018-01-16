/*
 * Copyright 2017, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <stdlib.h>
#include <optee_include/OpteeClientMem.h>

void *my_mem_start;
uint32_t my_count;
uint8_t *my_flag;
typedef struct {
	void *addrBlock;
	uint32_t sizeBlock;
	uint8_t used;
} ALLOC_FLAG;
ALLOC_FLAG alloc_flags[50];

void my_malloc_init(void *start, uint32_t size)
{
	memset(start, 0, size);
	my_mem_start = start;
	my_count = size/4096;
	my_flag = malloc(size/4096);
	memset(my_flag, 0, size/4096);
	memset(alloc_flags, 0, 50 * sizeof(ALLOC_FLAG));
}

void write_usedblock(void *addr, uint32_t size)
{
	uint8_t k;
	for (k = 0; k < 50; k++) {
		if (alloc_flags[k].used == 0) {
			alloc_flags[k].used = 1;
			alloc_flags[k].addrBlock = addr;
			alloc_flags[k].sizeBlock = size;
			break;
		}
	}
}

uint32_t find_sizeblock(void *addr)
{
	uint8_t k;
	for (k = 0; k < 50; k++)
		if (alloc_flags[k].used == 1 &&
				alloc_flags[k].addrBlock == addr)
			return alloc_flags[k].sizeBlock;

	return 0;
}

void free_usedblock(void *addr)
{
	uint8_t k;
	for (k = 0; k < 50; k++) {
		if (alloc_flags[k].used == 1 &&
				alloc_flags[k].addrBlock == addr) {
			alloc_flags[k].used = 0;
			alloc_flags[k].addrBlock = 0;
			alloc_flags[k].sizeBlock = 0;
			break;
		}
	}
}

void *my_malloc(uint32_t size)
{
	uint32_t i, j, k, num;

	num = (size - 1) / 4096 + 1;

	for (i = 0; i < my_count - num; i++) {
		if (*(my_flag + i) == 0) {
			for (j = 0; j < num; j++) {
				if (*(my_flag + i + j) != 0)
					break;
			}
			if (j == num) {
				for (k = 0; k < num; k++) {
					*(my_flag + i + k) = 1;
					memset(my_mem_start +
						(i + k) * 4096, 0, 4096);
				}
				debug(" malloc is: 0x%X  0x%X\n",
					(int)i, (int)num);
				write_usedblock((my_mem_start + i * 4096),
					num * 4096);

				return my_mem_start + (i * 4096);
			}
		}
	}

	return 0;
}

void my_free(void *ptr)
{
	uint32_t i, j, num, size;

	if (ptr < my_mem_start)
		return;

	i = (ptr - my_mem_start) / 4096;
	size = find_sizeblock(ptr);
	free_usedblock(ptr);
	if (size == 0)
		return;

	num = (size-1)/4096+1;
	debug(" free is: 0x%X  0x%X\n", i, num);

	for (j = 0; j < num; j++) {
		*(my_flag + i + j) = 0;
		memset(my_mem_start + (i + j) * 4096, 0, 4096);
	}
}

/*
 * Initlialize the memory component, for example providing the
 * containing drivers handle.
 */
void OpteeClientMemInit(void)
{
#if defined CONFIG_ROCKCHIP_ARCH32
	debug(" OpteeClientMemInit 32\n");
#if defined CONFIG_RKCHIP_RK3288
	my_malloc_init((void *)0x0910a000, 0x000e0000);
#else
	my_malloc_init((void *)0x6910a000, 0x000e0000);
#endif
#endif

#if defined CONFIG_ROCKCHIP_ARCH64
	debug(" OpteeClientMemInit 64\n");
	my_malloc_init((void *)0x09e10000, 0x003e0000);
#endif
}

/*
 * Allocate a page aligned block of memory from the TrustZone
 * shared memory block.
 */
void *OpteeClientMemAlloc(uint32_t length)
{
	return my_malloc(length);
}

/*
 * Free a block of memory previously allocated using the
 * OpteeClientMemAlloc function.
 */
void OpteeClientMemFree(void *mem)
{
	my_free(mem);
}
