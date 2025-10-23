/*
 * Copyright 2025, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <command.h>
#include <part.h>
#include <stdlib.h>
#include "rkss.h"

/*
 *#define DEBUG_RKSS
 *#define DEBUG_CLEAN_RKSS
 */

/*
 *	RK Secure Storage Version 3
 *		For security = 1M, device is emmc or nand flash(block size <= 128 kb)
 *		Block 0 Size : 128 kb	<---->	Block 4 Size : 128 kb
 *		Block 1 Size : 128 kb	<---->	Block 5 Size : 128 kb
 *		Block 2 Size : 128 kb	<---->	Block 6 Size : 128 kb
 *		Block 3 Size : 128 kb	<---->	Block 7 Size : 128 kb
 *		For security = 1M, device is nand flash(block size == 256 kb)
 *		Block 0 Size : 256 kb	<---->	Block 2 Size : 256 kb
 *		Block 1 Size : 256 kb	<---->	Block 3 Size : 256 kb
 *		For security = 1M, device is nand flash(block size == 512 kb)
 *		Block 0 Size : 512 kb	<---->	Block 1 Size : 512 kb
 *
 *		For security = 2M or more reference appeal, maxup to 32M.
 *
 *	For security = 1M, device is emmc or nand flash(block size <= 128 kb):
 *	------ Block 0 structure --------
 *	2   pages for meta header		[0-1]
 *	32  pages for flags			[2-33]
 *		- 1 byte = 2 flag
 *	128 pages for meta table		[34-161]
 *		- size of table 128 bytes
 *	93  pages for data			[162-254]
 *	1   pages for meta footer		[255]
 *
 *	------ Block 1 structure --------
 *	256   pages for data			[256-511]
 *
 *	------ Block 2 structure --------
 *	256   pages for data			[512-767]
 *
 *	------ Block 3 structure --------
 *	256   pages for data			[768-1023]
 *
 *	------ Block 4 structure --------
 *	2   pages for meta header		[1024-1025]
 *	32  pages for flags			[1026-1057]
 *		- 1 byte = 2 flag
 *	128 pages for meta table		[1058-1185]
 *		- size of table 128 bytes
 *	93  pages for data			[1186-1278]
 *	1   pages for meta footer		[1279]
 *
 *	------ Block 5 structure --------
 *	256   pages for data			[1280-1535]
 *
 *	------ Block 6 structure --------
 *	256   pages for data			[1536-1791]
 *
 *	------ Block 7 structure --------
 *	256   pages for data			[1792-2047]
 */

#define RKSS_TAG			0x524B5353

#define RKSS_HEADER_INDEX		0
#define RKSS_HEADER_COUNT		2
#define RKSS_USEDFLAGS_INDEX		2
#define RKSS_USEDFLAGS_COUNT		32
#define RKSS_TABLE_INDEX		34
#define RKSS_TABLE_COUNT		128
#define RKSS_DATA_INDEX			162

#define TABLE_SIZE			128
#define NAME_MAX_LENGTH			112
#define RKSS_DATA_LEN			512
#define RKSS_TABLES_EACH_BLOCK		4

struct meta_header {
	uint32_t	tag;
	uint32_t	version;
	uint32_t	counter;
	uint32_t	security_size;
	uint32_t	block_phy_size;
	uint32_t	block_phy_num;
	uint32_t	block_logic_size;
	uint32_t	block_logic_num;
	uint32_t	page_size;
	uint32_t	page_num;
	uint32_t	page_area_num;
	uint16_t	power_recover_num;
	uint16_t	crc_recover_num;
	uint16_t	block_dirty;
	uint16_t	block_reserve;
	uint32_t	block_crc[128];
	uint8_t		block_ver[128];
	uint8_t		block_update[128];
	uint8_t		reserve[204];
};

struct meta_table {
	uint32_t	page_id;
	uint32_t	size;
	uint8_t		used;
	uint8_t		compression;
	char		name[NAME_MAX_LENGTH];
	uint8_t		reserve[6];
};

struct meta_footer {
	uint8_t		reserve[500];
	uint32_t	tag;
	uint32_t	crc;
	uint32_t	counter;
};

struct meta_block {
	struct meta_header	*header;
	uint8_t			*flags;
	struct meta_table	*table;
	uint8_t			*data;
	struct meta_footer	*footer;
};

struct cache_block {
	uint32_t	block_id;
	uint8_t		ver;
	uint8_t		used;
	uint8_t		*data;
};

struct file_cache {
	int		fd;
	uint32_t	page_id;
	uint32_t	data_len;
	uint8_t		used;
	uint8_t		*data;
};

static uint8_t *meta_buffer;
static struct meta_block meta_info;
static struct cache_block cache_info[2];
static struct file_cache read_cache;

static int rkss_step;
static struct blk_desc *dev_desc;
static struct disk_partition part_info;

#define MIN(a, b)	(((a) > (b)) ? (b) : (a))
#define CRC32_POLYNOMIAL 0xEDB88320
static uint32_t crc32_table[256];

static void crc32_init_table(void)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t c = i;
		for (int j = 0; j < 8; j++) {
			if (c & 1)
				c = CRC32_POLYNOMIAL ^ (c >> 1);
			else
				c >>= 1;
		}
		crc32_table[i] = c;
	}
}

static uint32_t rkss_crc32(const uint8_t *data, size_t length, uint32_t initial_crc)
{
	uint32_t crc = initial_crc ^ 0xFFFFFFFF;

	while (length >= 8) {
		crc ^= *(uint32_t*)data;
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		data += 4;

		crc ^= *(uint32_t*)data;
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		crc = crc32_table[(crc >> 24) & 0xFF] ^ (crc << 8);
		data += 4;

		length -= 8;
	}

	while (length--) {
		crc = (crc >> 8) ^ crc32_table[(crc ^ *data++) & 0xFF];
	}

	return crc ^ 0xFFFFFFFF;
}

static int check_security_exist(int print_flag)
{
	if (!dev_desc) {
		dev_desc = plat_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device\n", __func__);
			return -1;
		}

		if (part_get_info_by_name(dev_desc,
					  "security", &part_info) < 0) {
			dev_desc = NULL;
			if (print_flag != 0)
				printf("Could not find security partition\n");
			return -1;
		}
	}
	return 0;
}

#define MB (1 << 20)
static int check_security_size(void)
{
	uint32_t length = part_info.size * part_info.blksz;

	// Support 1M, 2M, 4M, 8M, 16M, 32M
	if (length >= MB && length <= (32*MB) && (length & (length - 1)) == 0)
		return 0;
	return -1;
}

static int check_page_size(void)
{
	if (part_info.blksz != RKSS_DATA_LEN) {
		printf("check security blksz fail\n");
		return -1;
	}
	return 0;
}

static bool check_is_mtd_device(void)
{
#ifdef CONFIG_MTD_BLK
	if (dev_desc && dev_desc->uclass_id == UCLASS_MTD) {
		return true;
	}
#endif
	return false;
}

static uint32_t get_security_size(void)
{
	return part_info.size * part_info.blksz;
}

static uint32_t get_page_size(void)
{
	return part_info.blksz;
}

static uint32_t get_page_num(void)
{
	return part_info.size;
}

static uint32_t get_block_phy_size(void)
{
#ifdef CONFIG_MTD_BLK
	if (check_is_mtd_device()) {
		struct mtd_info *mtd = (struct mtd_info *)dev_desc->bdev->priv_;
		return mtd->erasesize;
	}
#endif
	return 0x20000;//128K
}

static uint32_t get_block_phy_num(void)
{
	return get_security_size() / get_block_phy_size();
}

static uint32_t get_block_logic_size(void)
{
	uint32_t security_size = get_security_size();
	uint32_t phy_size = get_block_phy_size();

	if ((security_size / phy_size) >= 2) {
		if (phy_size <= 0x20000)//128K
			return 0x20000;//128K
		else
			return phy_size;
	}

	printf("%s: security_size=0x%x, phy_size=0x%x\n",
		__func__, security_size, phy_size);
	return 0;
}

static uint32_t get_block_logic_num(void)
{
	return get_security_size() / get_block_logic_size();
}

static uint32_t get_area_page_offset(int index)
{
	uint32_t offset = part_info.size / 2;

	return part_info.start + index * offset;
}

static bool check_is_bad_area_block(uint8_t id)
{
#ifdef CONFIG_MTD_BLK
	if (check_is_mtd_device()) {
		struct mtd_info *mtd = (struct mtd_info *)dev_desc->bdev->priv_;
		uint32_t blksz = get_block_logic_size();
		uint32_t start, end, offset;
		start = get_area_page_offset(0) * get_page_size() + blksz * id;
		end = start + blksz;
		for (offset = start; offset < end; offset += get_block_phy_size()) {
			if (mtd_block_isbad(mtd, offset))
				return true;
		}
		start = get_area_page_offset(1) * get_page_size() + blksz * id;
		end = start + blksz;
		for (offset = start; offset < end; offset += get_block_phy_size()) {
			if (mtd_block_isbad(mtd, offset))
				return true;
		}
	}
#endif
	return false;
}

static uint32_t get_block_area_num(void)
{
	return get_block_logic_num() / 2;
}

static int get_meta_block_id(void)
{
	for (uint32_t i = 0; i < get_block_area_num(); i++) {
		if (!check_is_bad_area_block(i))
			return i;
	}
	return -1;
}

static int rkss_read_pages(uint8_t *data, uint32_t page_id, uint32_t page_num)
{
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint32_t begin_block = page_id / pages_per_block;
	uint32_t end_block = (page_id + page_num - 1) / pages_per_block;
	uint32_t block_offset = page_id % pages_per_block;
	uint32_t read_pages;
	uint32_t ret;
	uint8_t *ptr = data;

	if (meta_buffer == NULL) {
		printf("%s meta_buffer is null\n", __func__);
		return -1;
	}

	if (meta_info.header == NULL) {
		printf("%s meta_info.header is null\n", __func__);
		return -1;
	}

	if (data == NULL) {
		printf("%s data is null\n", __func__);
		return -1;
	}

	if (page_id >= meta_info.header->page_area_num) {
		printf("%s page_id invalid\n", __func__);
		return -1;
	}

	if (page_num == 0 || page_num > meta_info.header->page_area_num) {
		printf("%s page_num invalid\n", __func__);
		return -1;
	}

	if ((page_id + page_num) > meta_info.header->page_area_num) {
		printf("%s page_id or page_num invalid\n", __func__);
		return -1;
	}

	if (begin_block > end_block) {
		printf("%s block_id invalid\n", __func__);
		return -1;
	}

	if (begin_block >= get_block_area_num() || end_block >= get_block_area_num()) {
		printf("%s block_id invalid\n", __func__);
		return -1;
	}

	int meta_id = get_meta_block_id();
	if (meta_id < 0) {
		printf("%s: no valid meta block!\n", __func__);
		return -1;
	}

	if (begin_block == meta_id) {
		// read data from meta block cache data.
		if (end_block != meta_id) {
			printf("%s can not read meta block\n", __func__);
			return -1;
		}
		if ((page_id % pages_per_block) < RKSS_DATA_INDEX) {
			printf("%s can not read meta data\n", __func__);
			return -1;
		}
		uint32_t data_offset = ((page_id % pages_per_block) - RKSS_DATA_INDEX) * page_size;
		memcpy(data, meta_info.data + data_offset, page_num * page_size);
		return 0;
	}

	uint8_t *read_buffer = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, blksz);
	if (!read_buffer) {
		printf("%s: Malloc failed!\n", __func__);
		return -1;
	}

	for (uint32_t i = begin_block; i <= end_block; i++) {
		uint8_t ver = meta_info.header->block_ver[i];
		if (ver != 0 && ver != 1) {
			printf("block %d ver error\n", i);
			free(read_buffer);
			return -1;
		}

		// read data from cache buffer.
		uint32_t n;
		for (n = 0; n < ARRAY_SIZE(cache_info); n++) {
			if (!cache_info[n].data)
				continue;
			if (cache_info[n].used && cache_info[n].block_id == i) {
				memcpy(read_buffer, cache_info[n].data, blksz);
				break;
			}
		}

		// not in cache buffer, read from device.
		if (n == ARRAY_SIZE(cache_info)) {
			ret = blk_dread(dev_desc, get_area_page_offset(ver) + i * pages_per_block,
					pages_per_block, read_buffer);
			if (ret != pages_per_block) {
				printf("%s: blk_dread fail\n", __func__);
				free(read_buffer);
				return -1;
			}
		}

		if(rkss_crc32(read_buffer, blksz, 0) != meta_info.header->block_crc[i]) {
			printf("%s: block %d crc error\n", __func__, i);
			free(read_buffer);
			return -1;
		}

		if (i == begin_block) {
			read_pages = MIN(page_num, pages_per_block - block_offset);
			memcpy(ptr, read_buffer + block_offset * page_size, read_pages * page_size);
		} else if (i == end_block) {
			read_pages = (page_id + page_num) % pages_per_block;
			if (read_pages == 0)
				read_pages = pages_per_block;
			memcpy(ptr, read_buffer, read_pages * page_size);
		} else {
			read_pages = pages_per_block;
			memcpy(ptr, read_buffer, read_pages * page_size);
		}
		ptr += read_pages * page_size;
	}
	free(read_buffer);
	return 0;
}

static int rkss_write_pages(uint8_t *data, uint32_t page_id, uint32_t page_num)
{
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint32_t begin_block = page_id / pages_per_block;
	uint32_t end_block = (page_id + page_num - 1) / pages_per_block;
	uint32_t block_offset = page_id % pages_per_block;
	uint32_t write_pages;
	uint32_t ret;
	uint8_t *ptr = data;

	debug("begin_block=%d, end_block=%d, block_offset=%d\n",
	       begin_block, end_block, block_offset);

	if (meta_buffer == NULL) {
		printf("%s meta_buffer is null\n", __func__);
		return -1;
	}

	if (meta_info.header == NULL) {
		printf("%s meta_info.header is null\n", __func__);
		return -1;
	}

	if (data == NULL) {
		printf("%s data is null\n", __func__);
		return -1;
	}

	if (page_id >= meta_info.header->page_area_num) {
		printf("%s page_id invalid\n", __func__);
		return -1;
	}

	if (page_num == 0 || page_num > meta_info.header->page_area_num) {
		printf("%s page_num invalid\n", __func__);
		return -1;
	}

	if ((page_id + page_num) > meta_info.header->page_area_num) {
		printf("%s page_id or page_num invalid\n", __func__);
		return -1;
	}

	if (begin_block > end_block) {
		printf("%s block_id invalid\n", __func__);
		return -1;
	}

	if (begin_block >= get_block_area_num() || end_block >= get_block_area_num()) {
		printf("%s block_id invalid\n", __func__);
		return -1;
	}

	int meta_id = get_meta_block_id();
	if (meta_id < 0) {
		printf("%s: no valid meta block!\n", __func__);
		return -1;
	}

	if (begin_block == meta_id) {
		// write data to meta block cache data, sync last step.
		if (end_block != meta_id) {
			printf("%s can not write meta block\n", __func__);
			return -1;
		}
		if ((page_id % pages_per_block) < RKSS_DATA_INDEX) {
			printf("%s can not write meta data\n", __func__);
			return -1;
		}
		uint32_t data_offset = ((page_id % pages_per_block) - RKSS_DATA_INDEX) * page_size;
		memcpy(meta_info.data + data_offset, data, page_num * page_size);
		meta_info.header->block_dirty = 1;
		return 0;
	}

	uint8_t *write_buffer = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, blksz);
	if (!write_buffer) {
		printf("Malloc failed!\n");
		return -1;
	}

	for (uint32_t i = begin_block; i <= end_block; i++) {
		uint8_t ver = meta_info.header->block_ver[i];
		if (ver != 0 && ver != 1) {
			printf("%s: block %d ver error\n", __func__, i);
			free(write_buffer);
			return -1;
		}
		// read data from cache buffer.
		uint32_t n;
		for (n = 0; n < ARRAY_SIZE(cache_info); n++) {
			if (!cache_info[n].data)
				continue;
			if (cache_info[n].used && cache_info[n].block_id == i) {
				memcpy(write_buffer, cache_info[n].data, blksz);
				break;
			}
		}
		// not in cache buffer, read from device.
		if (n == ARRAY_SIZE(cache_info)) {
			ret = blk_dread(dev_desc, get_area_page_offset(ver) + i * pages_per_block,
					pages_per_block, write_buffer);
			if (ret != pages_per_block) {
				printf("%s: blk_dread fail\n", __func__);
				free(write_buffer);
				return -1;
			}
		}

		if (i == begin_block) {
			write_pages = MIN(page_num, pages_per_block - block_offset);
			memcpy(write_buffer + block_offset * page_size, ptr, write_pages * page_size);
		} else if (i == end_block) {
			write_pages = (page_id + page_num) % pages_per_block;
			if (write_pages == 0)
				write_pages = pages_per_block;
			memcpy(write_buffer, ptr, write_pages * page_size);
		} else {
			write_pages = pages_per_block;
			memcpy(write_buffer, ptr, write_pages * page_size);
		}
		ptr += write_pages * page_size;

		if (meta_info.header->block_update[i] == 0) {
			ver = ver == 0 ? 1 : 0;
			meta_info.header->block_ver[i] = ver;
			meta_info.header->block_update[i] = 1;
		}

		meta_info.header->block_crc[i] = rkss_crc32(write_buffer, blksz, 0);
		meta_info.header->block_dirty = 1;

		// write data to cache buffer.
		for (n = 0; n < ARRAY_SIZE(cache_info); n++) {
			if (!cache_info[n].data)
				continue;
			if (!cache_info[n].used ||
			    (cache_info[n].used && cache_info[n].block_id == i)) {
				cache_info[n].used = 1;
				cache_info[n].block_id = i;
				cache_info[n].ver = ver;
				memcpy(cache_info[n].data, write_buffer, blksz);
				debug("write block %d to cache %d\n", i, n);
				break;
			}
		}
		// not write to cache buffer, write data to device.
		if (n == ARRAY_SIZE(cache_info)) {
			debug("write block %d to device\n", i);
			if (check_is_mtd_device()) {
				blk_derase(dev_desc, get_area_page_offset(ver) + i * pages_per_block,
					   pages_per_block);
			}

			ret = blk_dwrite(dev_desc, get_area_page_offset(ver) + i * pages_per_block,
					 pages_per_block, write_buffer);
			if (ret != pages_per_block) {
				printf("%s: blk_dwrite fail\n", __func__);
				free(write_buffer);
				return -1;
			}

			/* verify */
			memset(write_buffer, 0, blksz);
			ret = blk_dread(dev_desc, get_area_page_offset(ver) + i * pages_per_block,
					pages_per_block, write_buffer);
			if (ret != pages_per_block) {
				printf("%s: blk_read fail\n", __func__);
				free(write_buffer);
				return -1;
			}
			if (rkss_crc32(write_buffer, blksz, 0) != meta_info.header->block_crc[i]) {
				printf("%s: blk verify fail\n", __func__);
				free(write_buffer);
				return -1;
			}
		}
	}
	free(write_buffer);
	return 0;
}

static int rkss_write_empty_table(struct meta_table *ptable)
{
	int i, j;
	struct meta_table *p;
	int fd;

	if (meta_info.table == NULL) {
		printf("%s table is null\n", __func__);
		return -1;
	}
	for (i = 0; i < RKSS_TABLE_COUNT; i++) {
		for (j = 0; j < RKSS_TABLES_EACH_BLOCK; j++) {
			fd = i * RKSS_TABLES_EACH_BLOCK + j;
			p = meta_info.table + fd;
			if (p->used == 0) {
				memcpy(p, ptable, sizeof(struct meta_table));
				p->used = 1;
				meta_info.header->block_dirty = 1;
				return fd;
			}
		}
	}
	printf("No enough ptable space available in secure storage.\n");
	return -1;
}

static int rkss_write_back_table(int fd, struct meta_table *ptable)
{
	struct meta_table *p;

	if (fd < 0 || fd >= RKSS_TABLE_COUNT * RKSS_TABLES_EACH_BLOCK) {
		printf("%s fd invalid\n", __func__);
		return -1;
	}

	if (meta_info.table == NULL) {
		printf("%s table is null\n", __func__);
		return -1;
	}

	p = meta_info.table + fd;

	memcpy(p, ptable, sizeof(struct meta_table));
	meta_info.header->block_dirty = 1;
	return 0;
}

static int rkss_get_table_by_name(char *filename, struct meta_table *ptable)
{
	uint32_t i, j, len;
	struct meta_table *p;
	int fd;

	len = strlen(filename);
	if (len >= NAME_MAX_LENGTH) {
		printf("%s: filename is too long. length:%u\n", __func__, len);
		return -1;
	}

	if (meta_info.table == NULL) {
		printf("%s table is null\n", __func__);
		return -1;
	}

	for (i = 0; i < RKSS_TABLE_COUNT; i++) {
		for (j = 0; j < RKSS_TABLES_EACH_BLOCK; j++) {
			fd = i * RKSS_TABLES_EACH_BLOCK + j;
			p = meta_info.table + fd;

			if (p->used == 0)
				continue;

			if (!strcmp(p->name, filename)) {
				memcpy(ptable, p, sizeof(struct meta_table));
				return fd;
			}
		}
	}
	return -1;
}


static int rkss_get_table_by_fd(int fd, struct meta_table *ptable)
{
	struct meta_table *p;

	if (fd < 0 || fd >= RKSS_TABLE_COUNT * RKSS_TABLES_EACH_BLOCK) {
		printf("%s fd invalid\n", __func__);
		return -1;
	}

	if (meta_info.table == NULL) {
		printf("%s table is null\n", __func__);
		return -1;
	}

	p = meta_info.table + fd;
	if (p->used != 1) {
		debug("%s unused table!\n", __func__);
		return -1;
	}
	memcpy(ptable, p, sizeof(struct meta_table));
	return 0;
}

static int rkss_get_pages_from_usedflags(int page_num)
{
	int i = 0;
	int count0 = 0;
	uint32_t page_size = get_page_size();

	if (meta_info.flags == NULL) {
		printf("%s flags is null\n", __func__);
		return -1;
	}
	for (i = 0; i < RKSS_USEDFLAGS_COUNT * page_size * 2; i++) {
		uint8_t *flag = meta_info.flags + (int)i / 2;
		uint8_t value = i & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;

		if (value == 0x0) {
			if (++count0 == page_num)
				return (i + 1 - page_num);
		} else {
			count0 = 0;
		}
	}

	printf("Not enough space available in secure storage !\n");
	return -1;
}

static int rkss_incref_usedflags(uint32_t page_id, uint32_t page_num)
{
	int value, i;
	uint8_t *flag;

	if (page_num == 0)
		return 0;

	if (page_id >= meta_info.header->page_area_num) {
		printf("%s page_id invalid\n", __func__);
		return -1;
	}

	if (page_num > meta_info.header->page_area_num) {
		printf("%s page_num invalid\n", __func__);
		return -1;
	}

	if ((page_id + page_num) > meta_info.header->page_area_num) {
		printf("%s page_id or page_num invalid\n", __func__);
		return -1;
	}

	if (meta_info.flags == NULL) {
		printf("%s flags is null\n", __func__);
		return -1;
	}

	for (i = 0; i < page_num; i++, page_id++) {
		flag = meta_info.flags + (int)page_id / 2;
		value = page_id & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (++value > 0xF) {
			printf("%s: reference out of data: %d\n", __func__, value);
			value = 0xF;
		}
		*flag = page_id & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	meta_info.header->block_dirty = 1;
	return 0;
}

static int rkss_decref_usedflags(uint32_t page_id, uint32_t page_num)
{
	int value, i;
	uint8_t *flag;

	if (page_num == 0)
		return 0;

	if (page_id >= meta_info.header->page_area_num) {
		printf("%s page_id invalid\n", __func__);
		return -1;
	}

	if (page_num > meta_info.header->page_area_num) {
		printf("%s page_num invalid\n", __func__);
		return -1;
	}

	if ((page_id + page_num) > meta_info.header->page_area_num) {
		printf("%s page_id or page_num invalid\n", __func__);
		return -1;
	}

	if (meta_info.flags == NULL) {
		printf("%s flags is null\n", __func__);
		return -1;
	}

	for (i = 0; i < page_num; i++, page_id++) {
		flag = meta_info.flags + (int)page_id / 2;
		value = page_id & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (--value < 0) {
			printf("%s: reference out of data: %d\n", __func__, value);
			value = 0x0;
		}
		*flag = page_id & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	meta_info.header->block_dirty = 1;
	return 0;
}

static int rkss_init_usedflags(void)
{
	uint8_t *flags;
	uint8_t *flagw;
	int n, value, start, end;
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;

	int meta_id = get_meta_block_id();
	if (meta_id < 0) {
		printf("%s: no valid meta block!\n", __func__);
		return -1;
	}

	debug("init usedflags section ...\n");

	flags = meta_info.flags;
	if (flags == NULL) {
		printf("%s flags is null\n", __func__);
		return -1;
	}

	memset(flags, 0, RKSS_USEDFLAGS_COUNT * page_size);

	//bad block
	for (uint32_t i = 0; i < get_block_area_num(); i++) {
		if (check_is_bad_area_block(i)) {
			start = i * pages_per_block;
			end = start + pages_per_block;
			for (n = start; n < end; n++) {
				flagw = flags + (int)n/2;
				value = 0x1;
				*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
				(*flagw & 0x0F) | (value << 4);
			}
		}
	}

	//meta block
	start = meta_id * pages_per_block;
	end = start + RKSS_HEADER_COUNT + RKSS_USEDFLAGS_COUNT + RKSS_TABLE_COUNT;
	for (n = start; n < end; n++) {
		flagw = flags + (int)n/2;
		value = 0x1;
		*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
				(*flagw & 0x0F) | (value << 4);
	}

	n = start + pages_per_block - 1;
	flagw = flags + (int)n/2;
	value = 0x1;
	*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
			(*flagw & 0x0F) | (value << 4);

	//unvalid block
	for (n = meta_info.header->page_area_num; n < RKSS_USEDFLAGS_COUNT * page_size * 2; n++) {
		flagw = flags + (int)n/2;
		value = 0x1;
		*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
				(*flagw & 0x0F) | (value << 4);
	}
	return 0;
}

static int find_valid_meta_block(void)
{
	int ret, i;
	uint32_t max_ver = 0;
	uint32_t max_index = 0;
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint32_t crc_size = blksz - sizeof(struct meta_footer);

	if (blksz == 0)
		return -1;

	int meta_id = get_meta_block_id();
	if (meta_id < 0) {
		printf("%s: no valid meta block!\n", __func__);
		return -1;
	}

	/* Find valid from (backup0 - backup1) */
	for (i = 0; i < 2; i++) {
		ret = blk_dread(dev_desc, get_area_page_offset(i) + meta_id * pages_per_block,
				pages_per_block, meta_buffer);
		if (ret != pages_per_block) {
			if (check_is_mtd_device()) {
				printf("MTD device, ignore blk_dread fail\n");
				memset(meta_buffer, 0, blksz);
			} else {
				printf("%s: blk_dread fail\n", __func__);
				return -1;
			}
		}

		if (meta_info.header->tag == RKSS_TAG) {
			if (meta_info.header->version == RKSS_VERSION_V3) {
				debug("data version equal to image version, do nothing!\n");
			} else if (meta_info.header->version < RKSS_VERSION_V3) {
				printf("data version lower than image version!\n");
				return -1;
			} else {
				printf("data version higher than image version!\n");
				printf("please update image!\n");
				return -1;
			}
		}

		if ((meta_info.header->tag == RKSS_TAG) &&
		    (meta_info.footer->counter == meta_info.header->counter) &&
		    rkss_crc32(meta_buffer, crc_size, 0) == meta_info.footer->crc) {
			if (max_ver < meta_info.header->counter) {
				max_index = i;
				max_ver = meta_info.header->counter;
			}
		}
	}

	if (max_ver == 0) {
		printf("No valid meta block found!\n");
		printf("Reset meta info...\n");
		memset(meta_buffer, 0, blksz);
		meta_info.header->tag = RKSS_TAG;
		meta_info.header->version = RKSS_VERSION_V3;
		meta_info.header->counter = 1;
		meta_info.header->security_size = get_security_size();
		meta_info.header->block_phy_size = get_block_phy_size();
		meta_info.header->block_phy_num = get_block_phy_num();
		meta_info.header->block_logic_size = get_block_logic_size();
		meta_info.header->block_logic_num = get_block_logic_num();
		meta_info.header->page_size = get_page_size();
		meta_info.header->page_num = get_page_num();
		meta_info.header->page_area_num = get_page_num() / 2;
		meta_info.header->block_dirty = 1;

		/* Init Usedflags */
		if (rkss_init_usedflags() < 0) {
			printf("%s rkss_init_usedflags fail !\n", __func__);
			return -1;
		}

		meta_info.footer->tag = RKSS_TAG;
		meta_info.footer->crc = rkss_crc32(meta_buffer, crc_size, 0);
		meta_info.footer->counter = 1;

		return 0;
	}

	debug("max_ver=%d, max_index=%d.\n", max_ver, max_index);

	ret = blk_dread(dev_desc, get_area_page_offset(max_index) + meta_id * pages_per_block,
			pages_per_block, meta_buffer);
	if (ret != pages_per_block) {
		printf("%s: blk_dread fail\n", __func__);
		return -1;
	}

	if (meta_info.header->security_size != get_security_size() ||
	    meta_info.header->block_phy_size != get_block_phy_size() ||
	    meta_info.header->block_logic_size != get_block_logic_size() ||
	    meta_info.header->page_size != get_page_size()) {
		printf("security partition size or block size changed!\n");
		return -1;
	}

	return 0;
}

static int rkss_storage_init(void)
{
	uint32_t flags_offset, table_offset, data_offset, footer_offset;
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();

	if (blksz == 0)
		return -1;

	flags_offset = RKSS_USEDFLAGS_INDEX * page_size;
	table_offset = RKSS_TABLE_INDEX * page_size;
	data_offset = RKSS_DATA_INDEX * page_size;
	footer_offset = blksz - page_size;

	if (meta_buffer == NULL) {
		/* Always use, no need to release */
		meta_buffer = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, blksz);
		if (!meta_buffer) {
			printf("%s: Malloc meta buffer failed!\n", __func__);
			return -1;
		}

		/* Pointer initialization */
		meta_info.header = (struct meta_header *)(meta_buffer);
		meta_info.flags = (uint8_t *)(meta_buffer + flags_offset);
		meta_info.table = (struct meta_table *)(meta_buffer + table_offset);
		meta_info.data = (uint8_t *)(meta_buffer + data_offset);
		meta_info.footer = (struct meta_footer *)(meta_buffer + footer_offset);

		if (find_valid_meta_block() < 0) {
			free(meta_buffer);
			meta_buffer = NULL;
			return -1;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(cache_info); i++) {
		cache_info[i].used = 0;
		cache_info[i].block_id = 0;
		cache_info[i].ver = 0;
		cache_info[i].data = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, blksz);
		if (!cache_info[i].data) {
			printf("%s: Malloc cache buffer failed!\n", __func__);
		}
	}
	return 0;
}

static int rkss_sync_cache_block(void)
{
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint32_t ret, ver;
	int block_id;

	// write cache buffer to device.
	for (uint32_t n = 0; n < ARRAY_SIZE(cache_info); n++) {
		if (!cache_info[n].data || !cache_info[n].used)
			continue;

		debug("sync cache %d block %d to device\n", n, cache_info[n].block_id);
		ver = cache_info[n].ver;
		block_id = cache_info[n].block_id;

		if (check_is_mtd_device()) {
			blk_derase(dev_desc, get_area_page_offset(ver) + block_id * pages_per_block,
				   pages_per_block);
		}

		ret = blk_dwrite(dev_desc, get_area_page_offset(ver) + block_id * pages_per_block,
				 pages_per_block, cache_info[n].data);
		if (ret != pages_per_block) {
			printf("%s: blk_dwrite fail\n", __func__);
			return -1;
		}

		/* verify */
		memset(cache_info[n].data, 0, blksz);
		ret = blk_dread(dev_desc, get_area_page_offset(ver) + block_id * pages_per_block,
				pages_per_block, cache_info[n].data);
		if (ret != pages_per_block) {
			printf("%s: blk_read fail\n", __func__);
			return -1;
		}
		if (rkss_crc32(cache_info[n].data, blksz, 0) != meta_info.header->block_crc[block_id]) {
			printf("%s: blk verify fail\n", __func__);
			return -1;
		}

		cache_info[n].used = 0;
		cache_info[n].block_id = 0;
		cache_info[n].ver = 0;
	}
	return 0;
}

static int rkss_sync_meta_block(void)
{
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint32_t crc_size = blksz - sizeof(struct meta_footer);
	uint32_t ret, ver;

	int meta_id = get_meta_block_id();
	if (meta_id < 0) {
		printf("%s: no valid meta block!\n", __func__);
		return -1;
	}

	if (meta_buffer == NULL) {
		printf("%s meta_buffer is null\n", __func__);
		return -1;
	}

	if (meta_info.header == NULL) {
		printf("%s meta_info.header is null\n", __func__);
		return -1;
	}

	if (meta_info.header->block_dirty == 0) {
		debug("No need to sync meta block!\n");
		return 0;
	}

	meta_info.header->counter++;
	meta_info.header->block_dirty = 0;
	memset(meta_info.header->block_update, 0, 128);
	meta_info.footer->tag = RKSS_TAG;
	meta_info.footer->counter = meta_info.header->counter;
	meta_info.footer->crc = rkss_crc32(meta_buffer, crc_size, 0);

	ver = meta_info.header->counter % 2;
	if (check_is_mtd_device()) {
		blk_derase(dev_desc, get_area_page_offset(ver) + meta_id * pages_per_block, pages_per_block);
	}

	ret = blk_dwrite(dev_desc, get_area_page_offset(ver) + meta_id * pages_per_block,
			 pages_per_block, meta_buffer);
	if (ret != pages_per_block) {
		printf("%s: blk_dwrite fail\n", __func__);
		return -1;
	}

	/* verify */
	memset(meta_buffer, 0, blksz);
	ret = blk_dread(dev_desc, get_area_page_offset(ver) + meta_id * pages_per_block,
			pages_per_block, meta_buffer);
	if (ret != pages_per_block) {
		printf("%s: blk_read fail\n", __func__);
		return -1;
	}

	if (meta_info.header->tag != RKSS_TAG ||
	    meta_info.footer->counter != meta_info.header->counter ||
	    rkss_crc32(meta_buffer, crc_size, 0) != meta_info.footer->crc) {
		printf("%s: blk verify fail\n", __func__);
		return -1;
	}

	return 0;
}

static int rkss_sync_block(void)
{
	if (rkss_sync_cache_block() < 0) {
		printf("rkss_sync_cache_block fail !\n");
		return -1;
	}

	if (rkss_sync_meta_block() < 0) {
		printf("rkss_sync_meta_block fail !\n");
		return -1;
	}
	return 0;
}

#ifdef DEBUG_CLEAN_RKSS
static int rkss_storage_reset(void)
{
	int ret, i;
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint8_t *delete_buff;

	if (blksz == 0)
		return -1;

	int meta_id = get_meta_block_id();
	if (meta_id < 0) {
		printf("%s: no valid meta block!\n", __func__);
		return -1;
	}

	delete_buff = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, blksz);
	if (!delete_buff) {
		printf("%s: Malloc failed!\n", __func__);
		return -1;
	}
	memset(delete_buff, 0, blksz);

	/* Find valid from (backup0 - backup1) */
	for (i = 0; i < 2; i++) {
		if (check_is_mtd_device()) {
			blk_derase(dev_desc, get_area_page_offset(i) + meta_id * pages_per_block, pages_per_block);
		}

		ret = blk_dwrite(dev_desc, get_area_page_offset(i) + meta_id * pages_per_block,
				 pages_per_block, delete_buff);
		if (ret != pages_per_block) {
			free(delete_buff);
			printf("%s: blk_dwrite fail\n", __func__);
			return -1;
		}
	}
	free(delete_buff);
	printf("reset meta block success!\n");
	return 0;
}
#endif

#ifdef DEBUG_RKSS
static void rkss_dump(char *name, void *data, uint32_t len)
{
	char *p = (char *)data;
	uint32_t i = 0;

	printf("------------- DUMP %s, len %d ------\n", name, len);
	for (i = 0; i < len; i++) {
		if ((i != 0) && (i % 32 == 0))
			printf("\n");
		printf("%02x ", *(p + i));
	}
	printf("\n");
	printf("------------- DUMP END -------------\n");
}

static void rkss_dump_ptable(void)
{
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint32_t begin_block, end_block, page_num;

	uint32_t table_num = RKSS_TABLE_COUNT * RKSS_TABLES_EACH_BLOCK;
	struct meta_table *ptable;

	ptable = meta_info.table;
	if (ptable == NULL)
		return;

	printf("-------------- DUMP ptable --------------\n");
	for (uint32_t i = 0; i < table_num; i++) {
		if (ptable->used == 0) {
			ptable++;
			continue;
		}
		page_num = ptable->size / page_size;
		if (ptable->size % page_size)
			page_num++;
		if (page_num == 0) {
			printf("[%02d] %s , page_id:%d, size:%d\n",
				i, ptable->name, ptable->page_id, ptable->size);
			ptable++;
			continue;
		}
		begin_block = ptable->page_id / pages_per_block;
		end_block = (ptable->page_id + page_num - 1) / pages_per_block;
		printf("[%02d] %s , page_id:%d, size:%d, begin_block=%d, end_block=%d\n",
			i, ptable->name, ptable->page_id, ptable->size, begin_block, end_block);

		ptable++;
	}
	printf("-------------- DUMP END --------------\n");
}

static void rkss_dump_block_crc(void)
{
	uint32_t blksz = get_block_logic_size();
	uint32_t page_size = get_page_size();
	uint32_t pages_per_block = blksz / page_size;
	uint32_t block_num = get_block_area_num();
	uint32_t ret, ver;
	uint8_t *read_buffer;
	uint32_t ver0_crc, ver1_crc, block_crc;

	if (meta_info.header == NULL)
		return;

	printf("-------------- DUMP block crc --------------\n");

	int meta_id = get_meta_block_id();
	if (meta_id < 0) {
		printf("%s: no valid meta block!\n", __func__);
		return;
	}

	read_buffer = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, blksz);
	if (!read_buffer) {
		printf("%s: Malloc failed!\n", __func__);
		return;
	}
	for (uint32_t i = 0; i < block_num; i++) {
		if (check_is_bad_area_block(i)) {
			printf("block %3d, is bad block.\n", i);
			continue;
		}
		if (i == meta_id) {
			printf("block %3d, is meta block.\n", i);
			printf("  tag=0x%08x, version=%d, counter=%d, security_size=%d\n",
			       meta_info.header->tag, meta_info.header->version,
			       meta_info.header->counter, meta_info.header->security_size);
			printf("  block_phy_size=%d, block_phy_num=%d, block_logic_size=%d, block_logic_num=%d\n",
			       meta_info.header->block_phy_size, meta_info.header->block_phy_num,
			       meta_info.header->block_logic_size, meta_info.header->block_logic_num);
			printf("  page_size=%d, page_num=%d, page_area_num=%d\n",
			       meta_info.header->page_size, meta_info.header->page_num,
			       meta_info.header->page_area_num);
			continue;
		}
		ver = meta_info.header->block_ver[i];
		if (ver != 0 && ver != 1) {
			printf("%s: block %d ver error.\n", __func__, i);
			free(read_buffer);
			return;
		}
		ret = blk_dread(dev_desc, get_area_page_offset(0) + i * pages_per_block,
				pages_per_block, read_buffer);
		if (ret != pages_per_block) {
			printf("%s: blk_dread fail\n", __func__);
			free(read_buffer);
			return;
		}
		ver0_crc = rkss_crc32(read_buffer, blksz, 0);

		ret = blk_dread(dev_desc, get_area_page_offset(1) + i * pages_per_block,
				pages_per_block, read_buffer);
		if (ret != pages_per_block) {
			printf("%s: blk_dread fail\n", __func__);
			free(read_buffer);
			return;
		}
		ver1_crc = rkss_crc32(read_buffer, blksz, 0);

		block_crc = meta_info.header->block_crc[i];
		printf("block %3d, ver=%d, ver0_crc=0x%08x, ver1_crc=0x%08x, block_crc=0x%08x\n",
		       i, ver, ver0_crc, ver1_crc, block_crc);
	}
	printf("-------------- DUMP END --------------\n");
	free(read_buffer);
	return;
}

static void rkss_dump_usedflags(void)
{
	uint32_t flags_num = RKSS_USEDFLAGS_COUNT * RKSS_DATA_LEN;

	if (meta_info.flags == NULL)
		return;
	rkss_dump("flags", meta_info.flags, flags_num);
}
#endif

int tee_supp_rk_fs_init_v3(void)
{
	assert(sizeof(struct meta_table) == TABLE_SIZE);

	crc32_init_table();

	if (check_security_exist(0) < 0)
		return 0;

	if (check_security_size() < 0)
		return -1;

	if (check_page_size() < 0)
		return -1;

	/* clean secure storage */
#ifdef DEBUG_CLEAN_RKSS
	if (rkss_storage_reset() < 0)
		return -1;
#endif

	if (rkss_storage_init() < 0)
		return -1;

#ifdef DEBUG_RKSS
	rkss_dump_ptable();
	rkss_dump_block_crc();
	rkss_dump_usedflags();
#endif

	return 0;
}

static uint32_t ree_fs_new_open(size_t num_params,
				struct optee_msg_param *params)
{
	char *filename;
	int fd;
	struct meta_table p = {0};

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(filename) >= NAME_MAX_LENGTH) {
		printf("%s: file name too long. %s\n", __func__, filename);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s: open file: %s, len: %zu\n",
		__func__, filename, strlen(filename));
	fd = rkss_get_table_by_name(filename, &p);
	if (fd < 0) {
		printf("%s: no such file. %s\n", __func__, filename);
		return TEE_ERROR_ITEM_NOT_FOUND;
	}

	params[2].u.value.a = fd;
	return TEE_SUCCESS;
}

static u32 ree_fs_new_create(size_t num_params,
				     struct optee_msg_param *params)
{
	char *filename;
	int fd;
	int ret, num;
	struct meta_table p = {0};
	uint32_t page_size = get_page_size();

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(filename) >= NAME_MAX_LENGTH) {
		printf("%s: file name too long. %s\n", __func__, filename);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s create file: %s, len: %zu\n",
		__func__, filename, strlen(filename));

	fd = rkss_get_table_by_name(filename, &p);
	if (fd >= 0) {
		printf("%s : file exist, clear it. %s\n", __func__, filename);
		/* decrease ref from usedflags */
		num = p.size / page_size;
		if (p.size % page_size)
			num++;
		ret = rkss_decref_usedflags(p.page_id, num);
		if (ret < 0) {
			printf("%s: rkss_decref_usedflags error !\n", __func__);
			return TEE_ERROR_GENERIC;
		}

		/* rm from ptable */
		memset(&p, 0, sizeof(struct meta_table));
		ret = rkss_write_back_table(fd, &p);
		if (ret < 0) {
			printf("%s : write back error %d\n", __func__, ret);
			return TEE_ERROR_GENERIC;
		}
	}

	strcpy(p.name, filename);
	p.page_id = 0;
	p.size = 0;
	p.used = 1;
	fd = rkss_write_empty_table(&p);
	if (fd < 0) {
		printf("%s : write empty ptable error. %s\n", __func__, filename);
		return TEE_ERROR_GENERIC;
	}

	params[2].u.value.a = fd;

	return TEE_SUCCESS;
}

static u32 ree_fs_new_close(size_t num_params,
				    struct optee_msg_param *params)
{
	debug("%s !\n", __func__);
	UNREFERENCED_PARAMETER(params);
	UNREFERENCED_PARAMETER(num_params);
	return TEE_SUCCESS;
}

static void clean_read_cache(void)
{
	read_cache.fd = 0;
	read_cache.page_id = 0;
	read_cache.data_len = 0;
	read_cache.used = 0;
	if (read_cache.data)
		free(read_cache.data);
	read_cache.data = NULL;
	return;
}

static u32 ree_fs_new_read(size_t num_params,
				   struct optee_msg_param *params)
{
	int fd;
	off_t offs;
	uint8_t *data;
	size_t len;
	int ret;
	struct meta_table p = {0};
	int di, page_num;
	uint8_t *file_data;
	uint32_t page_size = get_page_size();

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;

	data = tee_supp_param_to_va(params + 1);
	if (!data)
		return TEE_ERROR_BAD_PARAMETERS;
	len = params[1].u.rmem.size;

	debug("%s: fd:%d, len:%zu, offs:%ld\n",
		__func__, fd, len, offs);

	ret = rkss_get_table_by_fd(fd, &p);
	if (ret < 0) {
		printf("%s: unavailable fd: %d!\n", __func__, fd);
		return TEE_ERROR_GENERIC;
	}

	if (p.size == 0) {
		params[1].u.rmem.size = 0;
		return TEE_SUCCESS;
	}

	if (offs >= p.size)
		return TEE_ERROR_BAD_PARAMETERS;

	di = (offs + len) > p.size ? (p.size - offs) : len;

	/* read from read_cache */
	if (read_cache.data) {
		if (read_cache.used && read_cache.fd == fd &&
		    read_cache.page_id == p.page_id) {
			memcpy(data, read_cache.data + offs, di);
			params[1].u.rmem.size = di;
			return TEE_SUCCESS;
		}
	}

	page_num = p.size / page_size;
	if (p.size % page_size)
		page_num++;

	file_data = malloc(page_num * page_size);
	if (!file_data)
		return TEE_ERROR_OUT_OF_MEMORY;

	ret = rkss_read_pages(file_data, p.page_id, page_num);
	if (ret < 0) {
		printf("%s: unavailable file index!\n", __func__);
		free(file_data);
		return TEE_ERROR_GENERIC;
	}

	/* update read_cache */
	if (read_cache.data) {
		if (read_cache.data_len != page_num * page_size)
			clean_read_cache();
	}
	if (!read_cache.data) {
		read_cache.data = malloc(page_num * page_size);
		if (!read_cache.data) {
			printf("malloc read_cache.data fail!\n");
			return TEE_ERROR_OUT_OF_MEMORY;
		}
		read_cache.data_len = page_num * page_size;
	}
	if (read_cache.data) {
		read_cache.fd = fd;
		read_cache.page_id = p.page_id;
		read_cache.used = 1;
		memcpy(read_cache.data, file_data, page_num * page_size);
	}

	memcpy(data, file_data + offs, di);
	free(file_data);
	params[1].u.rmem.size = di;
	return TEE_SUCCESS;
}

static u32 ree_fs_new_write(size_t num_params,
				    struct optee_msg_param *params)
{
	int fd;
	off_t offs;
	uint8_t *data;
	size_t len;
	struct meta_table p = {0};
	int ret, new_size, new_page_num, old_page_num;
	uint8_t *file_data = 0;
	uint8_t *old_file_data = 0;
	uint32_t page_size = get_page_size();

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;

	data = tee_supp_param_to_va(params + 1);
	if (!data)
		return TEE_ERROR_BAD_PARAMETERS;
	len = params[1].u.rmem.size;

	debug("%s: fd:%d, len:%zu, offs:%ld\n",
		__func__, fd, len, offs);

	ret = rkss_get_table_by_fd(fd, &p);
	if (ret < 0) {
		printf("%s: fd:%d unvailable!\n", __func__, fd);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	new_size = offs + len > p.size ? offs + len : p.size;
	if (new_size == 0) {
		printf("%s: write zero size!\n", __func__);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	new_page_num = new_size / page_size;
	if (new_size % page_size)
		new_page_num++;

	file_data = malloc(new_page_num * page_size);
	if (!file_data)
		return TEE_ERROR_OUT_OF_MEMORY;

	memset(file_data, 0, new_page_num * page_size);

	if (p.size != 0) {
		/* Read old file data out */
		old_page_num = p.size / page_size;
		if (p.size % page_size)
			old_page_num++;
		old_file_data = malloc(old_page_num * page_size);
		if (!old_file_data) {
			ret = TEE_ERROR_OUT_OF_MEMORY;
			goto out;
		}
		ret = rkss_read_pages(old_file_data, p.page_id, old_page_num);
		if (ret < 0) {
			printf("%s: unavailable file index %d page_num %d\n",
				__func__, p.page_id, old_page_num);
			ret = TEE_ERROR_GENERIC;
			goto out;
		}
		memcpy(file_data, old_file_data, p.size);
		ret = rkss_decref_usedflags(p.page_id, old_page_num);
		if (ret < 0) {
			printf("%s: rkss_decref_usedflags error !\n", __func__);
			ret = TEE_ERROR_GENERIC;
			goto out;
		}
	}

	/* update new file info */
	memcpy(file_data + offs, data, len);
	p.used = 1;
	p.size = new_size;
	int get_page_id = rkss_get_pages_from_usedflags(new_page_num);
	if (get_page_id < 0) {
		printf("%s: rkss_get_pages_from_usedflags error !\n", __func__);
		ret = TEE_ERROR_GENERIC;
		goto out;
	}
	p.page_id = get_page_id;

	ret = rkss_incref_usedflags(p.page_id, new_page_num);
	if (ret < 0) {
		printf("%s: rkss_incref_usedflags error !\n", __func__);
		ret = TEE_ERROR_GENERIC;
		goto out;
	}

	ret = rkss_write_back_table(fd, &p);
	if (ret < 0) {
		printf("%s: write ptable error!\n", __func__);
		ret = TEE_ERROR_GENERIC;
		goto out;
	}

	/* write new file data */
	ret = rkss_write_pages(file_data, p.page_id, new_page_num);
	if (ret < 0) {
		printf("%s: write file data error!\n", __func__);
		ret = TEE_ERROR_GENERIC;
		goto out;
	}
	ret = TEE_SUCCESS;

out:
	if (file_data)
		free(file_data);
	if (old_file_data)
		free(old_file_data);

	return ret;
}

static u32 ree_fs_new_truncate(size_t num_params,
				       struct optee_msg_param *params)
{
	size_t len;
	int fd, ret;
	struct meta_table p = {0};
	uint32_t old_page_num, new_page_num;
	uint32_t page_size = get_page_size();

	fd = params[0].u.value.b;
	len = params[0].u.value.c;

	debug("%s: fd:%d, lenth:%zu\n", __func__, fd, len);

	ret = rkss_get_table_by_fd(fd, &p);
	if (ret < 0) {
		printf("%s: fd:%d unvailable!\n", __func__, fd);
		return TEE_ERROR_GENERIC;
	}
	if (len > p.size) {
		printf("%s: truncate error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	old_page_num = p.size / page_size;
	if (p.size % page_size)
		old_page_num++;

	new_page_num = len / page_size;
	if (len % page_size)
		new_page_num++;

	ret = rkss_decref_usedflags(p.page_id + new_page_num, old_page_num - new_page_num);
	if (ret < 0) {
		printf("%s: rkss_decref_usedflags error !\n", __func__);
		ret = TEE_ERROR_GENERIC;
	}

	p.size = len;
	ret = rkss_write_back_table(fd, &p);
	if (ret < 0) {
		printf("%s: write ptable error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

static u32 ree_fs_new_remove(size_t num_params,
				     struct optee_msg_param *params)
{
	char *filename;
	struct meta_table p = {0};
	int ret, fd, num;
	uint32_t page_size = get_page_size();

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	fd = rkss_get_table_by_name(filename, &p);
	if (fd < 0) {
		printf("%s: no such file. %s\n", __func__, filename);
		return 0;
	}

	debug("%s fd:%d page_id:%d size:%d\n", filename, fd, p.page_id, p.size);

	/* decrease ref from usedflags */
	num = p.size / page_size;
	if (p.size % page_size)
		num++;

	ret = rkss_decref_usedflags(p.page_id, num);
	if (ret < 0) {
		printf("%s: rkss_decref_usedflags error !\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	/* rm from ptable */
	memset(&p, 0, sizeof(struct meta_table));
	ret = rkss_write_back_table(fd, &p);
	if (ret < 0) {
		printf("%s: write back error %d\n", __func__, ret);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

static u32 ree_fs_new_rename(size_t num_params,
				     struct optee_msg_param *params)
{
	char *old_fname;
	char *new_fname;
	struct meta_table p = {0};
	int ret;

	old_fname = tee_supp_param_to_va(params + 1);
	if (!old_fname)
		return TEE_ERROR_BAD_PARAMETERS;

	new_fname = tee_supp_param_to_va(params + 2);
	if (!new_fname)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(new_fname) >= NAME_MAX_LENGTH) {
		printf("%s: new file name too long. %s\n", __func__, new_fname);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s: rename: %s -> %s\n", __func__, old_fname, new_fname);

	ret = rkss_get_table_by_name(old_fname, &p);
	if (ret < 0) {
		printf("%s: filename: %s no found.\n", __func__, old_fname);
		return TEE_ERROR_ITEM_NOT_FOUND;
	}

	strcpy(p.name, new_fname);

	ret = rkss_write_back_table(ret, &p);
	if (ret < 0) {
		printf("%s: write ptable error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

static u32 ree_fs_new_opendir(size_t num_params,
				      struct optee_msg_param *params)
{
	UNREFERENCED_PARAMETER(params);
	UNREFERENCED_PARAMETER(num_params);
	return TEE_ERROR_NOT_SUPPORTED;
}

static u32 ree_fs_new_closedir(size_t num_params,
				       struct optee_msg_param *params)
{
	UNREFERENCED_PARAMETER(params);
	UNREFERENCED_PARAMETER(num_params);
	return TEE_SUCCESS;
}

static u32 ree_fs_new_readdir(size_t num_params,
				      struct optee_msg_param *params)
{
	UNREFERENCED_PARAMETER(params);
	UNREFERENCED_PARAMETER(num_params);
	return TEE_ERROR_NOT_SUPPORTED;
}

int tee_supp_rk_fs_process_v3(size_t num_params,
			      struct optee_msg_param *params)
{
	uint32_t ret;

	if (!num_params || !tee_supp_param_is_value(params))
		return TEE_ERROR_BAD_PARAMETERS;

	switch (params->u.value.a) {
	case OPTEE_MRF_OPEN:
		debug(">>>>>>> [%d] OPTEE_MRF_OPEN!\n", rkss_step++);
		ret = ree_fs_new_open(num_params, params);
		break;
	case OPTEE_MRF_CREATE:
		debug(">>>>>>> [%d] OPTEE_MRF_CREATE!\n", rkss_step++);
		ret = ree_fs_new_create(num_params, params);
		break;
	case OPTEE_MRF_CLOSE:
		debug(">>>>>>> [%d] OPTEE_MRF_CLOSE!\n", rkss_step++);
		ret = ree_fs_new_close(num_params, params);
		rkss_sync_block();
		break;
	case OPTEE_MRF_READ:
		debug(">>>>>>> [%d] OPTEE_MRF_READ!\n", rkss_step++);
		ret = ree_fs_new_read(num_params, params);
		break;
	case OPTEE_MRF_WRITE:
		debug(">>>>>>> [%d] OPTEE_MRF_WRITE!\n", rkss_step++);
		ret = ree_fs_new_write(num_params, params);
		break;
	case OPTEE_MRF_TRUNCATE:
		debug(">>>>>>> [%d] OPTEE_MRF_TRUNCATE!\n", rkss_step++);
		ret = ree_fs_new_truncate(num_params, params);
		break;
	case OPTEE_MRF_REMOVE:
		debug(">>>>>>> [%d] OPTEE_MRF_REMOVE!\n", rkss_step++);
		ret = ree_fs_new_remove(num_params, params);
		rkss_sync_block();
		break;
	case OPTEE_MRF_RENAME:
		debug(">>>>>>> [%d] OPTEE_MRF_RENAME!\n", rkss_step++);
		ret = ree_fs_new_rename(num_params, params);
		rkss_sync_block();
		break;
	case OPTEE_MRF_OPENDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_OPENDIR!\n", rkss_step++);
		ret = ree_fs_new_opendir(num_params, params);
		break;
	case OPTEE_MRF_CLOSEDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_CLOSEDIR!\n", rkss_step++);
		ret = ree_fs_new_closedir(num_params, params);
		break;
	case OPTEE_MRF_READDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_READDIR!\n", rkss_step++);
		ret = ree_fs_new_readdir(num_params, params);
		break;
	default:
		ret = TEE_ERROR_BAD_PARAMETERS;
		break;
	}
	/* clean read cache except read operation */
	if (params->u.value.a != OPTEE_MRF_READ)
		clean_read_cache();

	return ret;
}
