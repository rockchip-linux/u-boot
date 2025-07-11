/*
 * Copyright 2023, Rockchip Electronics Co., Ltd
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
 *	RK Secure Storage Version 2
 *		Area0 Backup 0 Size : 256 kb	<---->	Area0 Backup 1 Size : 256 kb
 *		Area1 Backup 0 Size : 256 kb	<---->	Area1 Backup 1 Size : 256 kb
 *
 *	------ 1 section is 512 bytes -----
 *	------ Area0 Backup0 section from 0 to 511 --------
 *	1 section for file header		[0]
 *	1 section for used flags		[1]
 *		- 1 byte = 2 flag
 *	62 section for file tables		[2-63]
 *		- size of table 128 bytes
 *	447 section for data			[64-510]
 *	1 section for file footer		[511]
 *
 * 	------ Area0 Backup1 section from 512 to 1023 --------
 *	1 section for file header		[512]
 *	1 section for used flags		[513]
 *		- 1 byte = 2 flag
 *	62 section for file tables		[514-575]
 *		- size of table 128 bytes
 *	447 section for data			[576-1022]
 *	1 section for file footer		[1023]
 *
 * 	------ Area1 Backup0 section from 1024 to 1535 --------
 *	1 section for file header		[1024]
 *	1 section for used flags		[1025]
 *		- 1 byte = 2 flag
 *	62 section for file tables		[1026-1087]
 *		- size of table 128 bytes
 *	447 section for data			[1088-1534]
 *	1 section for file footer		[1535]
 *
 * 	------ Area1 Backup1 section from 1536 to 2047 --------
 *	1 section for file header		[1536]
 *	1 section for used flags		[1537]
 *		- 1 byte = 2 flag
 *	62 section for file tables		[1538-1599]
 *		- size of table 128 bytes
 *	447 section for data			[1600-2046]
 *	1 section for file footer		[2047]
 */

/* define for backup */
#define RKSS_HEADER_INDEX		0
#define RKSS_HEADER_COUNT		1
#define RKSS_USEDFLAGS_INDEX		1
#define RKSS_USEDFLAGS_COUNT		1
#define RKSS_TABLE_INDEX		2
#define RKSS_TABLE_COUNT		62
#define RKSS_DATA_INDEX			64
#define RKSS_DATA_COUNT			447
#define RKSS_FOOTER_INDEX		511
#define RKSS_FOOTER_COUNT		1
#define RKSS_SECTION_COUNT		512

#define RKSS_MAX_AREA_NUM		8
#define RKSS_ACTIVE_AREA_NUM		2
#define RKSS_DATA_LEN			512
#define RKSS_EACH_FILEFOLDER_COUNT	4
#define RKSS_TABLE_SIZE			128
#define RKSS_NAME_MAX_LENGTH		112
#define RKSS_BACKUP_NUM			2
#define RKSS_TAG			0x524B5353

#define SYNC_NONE			0
#define SYNC_DOING			1
#define SYNC_DONE			2

struct rkss_file_header {
	uint32_t	tag;
	uint32_t	version;
	uint32_t	backup_count;
	uint16_t	backup_index;
	uint16_t	backup_dirty;
	uint16_t	sync_flag;
	uint8_t		reserve[494];
};
struct rkss_file_table {
	uint32_t	size;
	uint16_t	index;
	uint8_t		flags;
	uint8_t		used;
	char		name[RKSS_NAME_MAX_LENGTH];
	uint8_t		reserve[8];
};
struct rkss_file_footer {
	uint8_t		reserve[508];
	uint32_t	backup_count;
};
struct rkss_file {
	struct rkss_file_header *header;
	uint8_t	*flags;
	struct rkss_file_table *table;
	uint8_t	*data;
	struct rkss_file_footer *footer;
};

/* RK Secure Storage Calls */
static char dir_cache[RKSS_NAME_MAX_LENGTH][12];
static int dir_num;
static int dir_seek;
static uint8_t *rkss_buffer[RKSS_MAX_AREA_NUM];
static struct rkss_file rkss_info[RKSS_MAX_AREA_NUM];

static struct blk_desc *dev_desc;
static struct disk_partition part_info;

static int check_security_exist(int print_flag)
{
	if (!dev_desc) {
		dev_desc = plat_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device.\n", __func__);
			return -1;
		}

		if (part_get_info_by_name(dev_desc,
					  "security", &part_info) < 0) {
			dev_desc = NULL;
			if (print_flag != 0)
				printf("%s: Could not find security partition.\n", __func__);
			return -1;
		}
	}
	return 0;
}

static int rkss_verify_usedflags(unsigned int area_index)
{
	uint8_t *flags;
	int i, duel, flag, n, value;
	uint8_t *flagw;
	int used_count;

	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: Not support area_index 0x%x.\n", __func__, area_index);
		return -1;
	}

	flags = rkss_info[area_index].flags;
	if (flags == NULL) {
		printf("%s: flags is null.\n", __func__);
		return -1;
	}

	used_count = RKSS_HEADER_COUNT +
		RKSS_USEDFLAGS_COUNT +
		RKSS_TABLE_COUNT;

	for (i = 0; i < used_count; i++) {
		duel = *(flags + (int)i/2);
		flag = i & 0x1 ? duel & 0x0F : (duel & 0xF0) >> 4;
		if (flag != 0x1)
			goto init;
	}

	for (i = RKSS_FOOTER_INDEX; i < RKSS_USEDFLAGS_COUNT * RKSS_DATA_LEN * 2; i++) {
		duel = *(flags + (int)i/2);
		flag = i & 0x1 ? duel & 0x0F : (duel & 0xF0) >> 4;
		if (flag != 0x1)
			goto init;
	}

	debug("%s: success.\n", __func__);
	return 0;

init:
	debug("%s: init usedflags section ...\n", __func__);
	memset(flags, 0, RKSS_USEDFLAGS_COUNT * RKSS_DATA_LEN);
	for (n = 0; n < used_count; n++) {
		flagw = flags + (int)n/2;
		value = 0x1;
		*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
				(*flagw & 0x0F) | (value << 4);
	}

	for (n = RKSS_FOOTER_INDEX; n < RKSS_USEDFLAGS_COUNT * RKSS_DATA_LEN * 2; n++) {
		flagw = flags + (int)n/2;
		value = 0x1;
		*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
				(*flagw & 0x0F) | (value << 4);
	}
	return 0;
}

#ifdef DEBUG_CLEAN_RKSS
static int rkss_storage_delete(uint32_t area_index)
{
	int ret;
	uint32_t size;
	uint8_t *delete_buff;

	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: Not support area_index 0x%x\n", __func__, area_index);
		return -1;
	}

	printf("%s: delete area index 0x%x!\n", __func__, area_index);
	size = RKSS_SECTION_COUNT * RKSS_BACKUP_NUM * RKSS_DATA_LEN;
	delete_buff = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, size);
	if (!delete_buff) {
		printf("%s: Malloc failed!\n", __func__);
		return -1;
	}
	memset(delete_buff, 0, size);
	ret = blk_dwrite(dev_desc,
			 part_info.start + area_index *
			 RKSS_SECTION_COUNT * RKSS_BACKUP_NUM,
			 RKSS_SECTION_COUNT * RKSS_BACKUP_NUM,
			 delete_buff);
	if (ret != RKSS_SECTION_COUNT * RKSS_BACKUP_NUM) {
		free(delete_buff);
		printf("%s: blk_dwrite failed!\n", __func__);
		return -1;
	}

	if (delete_buff)
		free(delete_buff);
	printf("%s: delete area success!\n", __func__);
	return 0;
}

static int rkss_storage_reset(void)
{
	if (rkss_storage_delete(0) < 0)
		return -1;
	if (rkss_storage_delete(1) < 0)
		return -1;
	return 0;
}
#endif

#ifdef DEBUG_RKSS
static void rkss_dump(void *data, unsigned int len)
{
	char *p = (char *)data;
	unsigned int i = 0;

	printf("-------------- DUMP %d --------------\n", len);
	for (i = 0; i < len; i++) {
		if (i % 32 == 0)
			printf("\n");
		printf("%02x ", *(p + i));
	}
	printf("\n");
	printf("------------- DUMP END -------------\n");
}

static void rkss_dump_ptable(void)
{
	int i, j, n;
	struct rkss_file_table *ptable;

	printf("-------------- DUMP ptable --------------\n");

	for (i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		ptable = rkss_info[i].table;
		if (rkss_info[i].table == NULL)
			continue;
		printf("--------------- area[%d] tables ------------\n", i);
		for (j = 0; j < RKSS_TABLE_COUNT; j++) {
			for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
				printf("[%02d][%c] %s , inx:%d, size:%d\n",
						j * RKSS_EACH_FILEFOLDER_COUNT + n,
						ptable->used == 0 ? 'F':'T', ptable->name,
						ptable->index, ptable->size);

				ptable++;
			}
		}
	}
	printf("-------------- DUMP END --------------\n");
}

static void rkss_dump_usedflags(void)
{
	int i;

	for (i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		if (rkss_info[i].flags == NULL)
			continue;
		printf("--------------- area[%d] flags ------------\n", i);
		rkss_dump(rkss_info[i].flags, RKSS_USEDFLAGS_COUNT * RKSS_DATA_LEN);
	}
}
#endif

static int rkss_read_multi_sections(unsigned int area_index,
				    unsigned char *data,
				    unsigned long index,
				    unsigned int num)
{
	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: area_index invalid\n", __func__);
		return -1;
	}
	if (index >= RKSS_SECTION_COUNT || num > RKSS_SECTION_COUNT ||
		(index + num) > RKSS_SECTION_COUNT) {
		printf("%s: index num invalid\n", __func__);
		return -1;
	}
	if (rkss_buffer[area_index] == NULL) {
		printf("%s: rkss_buffer is null\n", __func__);
		return -1;
	}
	memcpy(data, rkss_buffer[area_index] + index * RKSS_DATA_LEN, num * RKSS_DATA_LEN);
	return 0;
}

static int rkss_write_multi_sections(unsigned int area_index,
				     unsigned char *data,
				     unsigned long index,
				     unsigned int num)
{
	if (num == 0)
		return 0;

	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: area_index invalid\n", __func__);
		return -1;
	}

	if (index >= RKSS_SECTION_COUNT || num > RKSS_SECTION_COUNT ||
		(index + num) > RKSS_SECTION_COUNT) {
		printf("%s: index num invalid\n", __func__);
		return -1;
	}

	if (rkss_buffer[area_index] == NULL) {
		printf("%s: rkss_buffer is null\n", __func__);
		return -1;
	}

	memcpy(rkss_buffer[area_index] + index * RKSS_DATA_LEN, data, num * RKSS_DATA_LEN);
	rkss_info[area_index].header->backup_dirty = 1;
	return 0;
}

static int rkss_get_fileinfo_by_index(int fd,
				      struct rkss_file_table *ptable,
				      unsigned int *out_area_index)
{
	struct rkss_file_table *p;
	unsigned int area_index;

	area_index = fd / (RKSS_TABLE_COUNT * RKSS_EACH_FILEFOLDER_COUNT);
	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: area_index invalid\n", __func__);
		return -1;
	}

	p = rkss_info[area_index].table;
	if (p == NULL) {
		printf("%s: table is null\n", __func__);
		return -1;
	}

	p += fd % (RKSS_TABLE_COUNT * RKSS_EACH_FILEFOLDER_COUNT);
	if (p->used != 1) {
		debug("%s: unused table!\n", __func__);
		return -1;
	}
	debug("%s: p->used=%d p->name=%s p->index=%d p->size=%d\n",
		__func__, p->used, p->name, p->index, p->size);
	memcpy(ptable, p, sizeof(struct rkss_file_table));
	*out_area_index = area_index;
	return 0;
}

static int rkss_get_fileinfo_by_name(char *filename,
				     struct rkss_file_table *ptable,
				     unsigned int *out_area_index)
{
	int ret;
	unsigned int i, j, n, len;
	struct rkss_file_table *p;

	len = strlen(filename);
	if (len > RKSS_NAME_MAX_LENGTH - 1) {
		printf("%s: filename is too long. length:%u\n", __func__, len);
		return -1;
	}

	for (i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		if (rkss_info[i].table == NULL)
			continue;
		for (j = 0; j < RKSS_TABLE_COUNT; j++) {
			for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
				p = rkss_info[i].table + j * RKSS_EACH_FILEFOLDER_COUNT + n;

				if (p->used == 0)
					continue;

				if (!strcmp(p->name, filename)) {
					debug("%s: area%d hit table[%d/%d], index[%d/%d]\n",
						__func__, i, j, RKSS_TABLE_COUNT, n, RKSS_EACH_FILEFOLDER_COUNT);
					memcpy(ptable, p, sizeof(struct rkss_file_table));
					*out_area_index = i;
					ret = i * RKSS_TABLE_COUNT * RKSS_EACH_FILEFOLDER_COUNT +
						j * RKSS_EACH_FILEFOLDER_COUNT + n;
					return ret;
				}

				// Folder Matching
				const char *split = "/";
				char *last_inpos = filename;
				char *last_svpos = p->name;
				char *cur_inpos = NULL;
				char *cur_svpos = NULL;

				do {
					cur_inpos = strstr(last_inpos, split);
					cur_svpos = strstr(last_svpos, split);
					int size_in = cur_inpos == NULL ?
							(int)strlen(last_inpos) : cur_inpos - last_inpos;
					int size_sv = cur_svpos == NULL ?
							(int)strlen(last_svpos) : cur_svpos - last_svpos;

					ret = memcmp(last_inpos, last_svpos, size_in);

					last_inpos = cur_inpos + 1;
					last_svpos = cur_svpos + 1;

					if (size_in != size_sv || ret)
						goto UNMATCHFOLDER;

				} while (cur_inpos && cur_svpos);

				debug("%s: Matched folder: %s\n", __func__, p->name);
				return -100;
UNMATCHFOLDER:
				debug("%s: Unmatched ...\n", __func__);
			}
		}
	}
	debug("%s: file or dir no found!\n", __func__);
	return -1;
}

static int rkss_get_dirs_by_name(char *filename)
{
	int ret;
	unsigned int i, j, n, len;
	struct rkss_file_table *p;

	len = strlen(filename);
	if (len > RKSS_NAME_MAX_LENGTH - 1) {
		printf("%s: filename is too long. length:%u\n", __func__, len);
		return -1;
	}

	dir_num = 0;
	for (i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		if (rkss_info[i].table == NULL)
			continue;
		for (j = 0; j < RKSS_TABLE_COUNT; j++) {
			for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
				p = rkss_info[i].table + j * RKSS_EACH_FILEFOLDER_COUNT + n;

				if (p->used == 0)
					continue;

				// Full Matching
				ret = memcmp(p->name, filename, strlen(filename));
				debug("%s: comparing [fd:%d] : %s ?= %s, ret: %d\n", __func__,
					(i * RKSS_TABLE_COUNT + j) * RKSS_EACH_FILEFOLDER_COUNT + n,
					p->name, filename, ret);
				if (!ret && strlen(p->name) > strlen(filename)) {
					char *chk = p->name + strlen(filename);
					if (*chk == '/') {
						char *file = p->name + strlen(filename) + 1;
						char *subdir = strtok(file, "/");
						debug("%s: found: %s\n", __func__, subdir);
						strcpy(dir_cache[dir_num], subdir);
						++dir_num;
					}
				}
			}
		}
	}
	return dir_num;
}

static int rkss_get_empty_section_from_usedflags(unsigned int area_index,
						 int section_size)
{
	int i = 0;
	int count0 = 0;

	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: area_index invalid\n", __func__);
		return -1;
	}
	if (rkss_info[area_index].flags == NULL) {
		printf("%s: flags is null\n", __func__);
		return -1;
	}
	for (i = 0; i < RKSS_SECTION_COUNT; i++) {
		uint8_t *flag = rkss_info[area_index].flags + (int)i/2;
		uint8_t value = i & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;

		if (value == 0x0) {
			if (++count0 == section_size)
				return (i + 1 - section_size);
		} else {
			count0 = 0;
		}
	}

	printf("%s: Not enough space available in secure storage!\n", __func__);
	return -10;
}

static int rkss_incref_multi_usedflags_sections(unsigned int area_index,
						unsigned int index,
						unsigned int num)
{
	int value, i;
	uint8_t *flag;

	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: area_index invalid\n", __func__);
		return -1;
	}

	if (index >= RKSS_SECTION_COUNT || num > RKSS_SECTION_COUNT ||
		(index + num) > RKSS_SECTION_COUNT) {
		printf("%s: index[%d] out of range.\n", __func__, index);
		return -1;
	}
	if (rkss_info[area_index].flags == NULL) {
		printf("%s: flags is null\n", __func__);
		return -1;
	}

	for (i = 0; i < num; i++, index++) {
		flag = rkss_info[area_index].flags + (int)index / 2;
		value = index & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (++value > 0xF) {
			printf("%s: reference out of data: %d\n", __func__, value);
			value = 0xF;
		}
		*flag = index & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	rkss_info[area_index].header->backup_dirty = 1;
	return 0;
}

static int rkss_decref_multi_usedflags_sections(unsigned int area_index,
						unsigned int index,
						unsigned int num)
{
	int value, i;
	uint8_t *flag;

	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: area_index invalid\n", __func__);
		return -1;
	}

	if (index >= RKSS_SECTION_COUNT || num > RKSS_SECTION_COUNT ||
		(index + num) > RKSS_SECTION_COUNT) {
		printf("%s: index[%d] out of range.\n", __func__, index);
		return -1;
	}
	if (rkss_info[area_index].flags == NULL) {
		printf("%s: flags is null\n", __func__);
		return -1;
	}

	for (i = 0; i < num; i++, index++) {
		flag = rkss_info[area_index].flags + (int)index / 2;
		value = index & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (--value < 0) {
			printf("%s: reference out of data: %d\n", __func__, value);
			value = 0x0;
		}
		*flag = index & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	rkss_info[area_index].header->backup_dirty = 1;
	return 0;
}

static int rkss_get_remain_tables(struct rkss_file_table *p)
{
	unsigned int i, n;
	int count = 0;

	if (p == NULL)
		return -1;

	for (i = 0; i < RKSS_TABLE_COUNT; i++) {
		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
			if (p->used == 0)
				count++;
			p++;
		}
	}
	return count;
}

static int rkss_get_remain_flags(uint8_t *flags)
{
	unsigned int i, value;
	uint8_t *flag;
	int count = 0;

	if (flags == NULL)
		return -1;

	for (i = 0; i < RKSS_SECTION_COUNT; i++) {
		flag = flags + (int)i / 2;
		value = i & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (value == 0)
			count++;
	}
	return count;
}

static int rkss_get_larger_area(void)
{
	int i, tables, flags, max_flags = 0;
	int area_index = -1;

	for (i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		if (rkss_info[i].table == NULL ||
		rkss_info[i].flags == NULL)
			continue;
		tables = rkss_get_remain_tables(rkss_info[i].table);
		flags = rkss_get_remain_flags(rkss_info[i].flags);
		if (tables > 0 && flags > 0 && flags > max_flags) {
			max_flags = flags;
			area_index = i;
		}
	}
	return area_index;
}

static int rkss_write_area_empty_ptable(unsigned int area_index,
					struct rkss_file_table *pfile_table)
{
	int i, n, ret;
	struct rkss_file_table *p;

	if (rkss_info[area_index].table == NULL) {
		printf("%s: table is null\n", __func__);
		return -1;
	}
	for (i = 0; i < RKSS_TABLE_COUNT; i++) {
		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
			p = rkss_info[area_index].table + i * RKSS_EACH_FILEFOLDER_COUNT + n;
			if (p->used == 0) {
				memcpy(p, pfile_table, sizeof(struct rkss_file_table));
				p->used = 1;
				debug("%s: write emt ptable : [%d,%d] name:%s, index:%d, size:%d, used:%d\n",
					__func__, i, n, p->name, p->index, p->size, p->used);
				rkss_info[area_index].header->backup_dirty = 1;
				ret = area_index * RKSS_TABLE_COUNT * RKSS_EACH_FILEFOLDER_COUNT +
					i * RKSS_EACH_FILEFOLDER_COUNT + n;
				return ret;
			}
		}
	}
	printf("%s: No enough ptable space available in secure storage.\n", __func__);
	return -1;
}

static int rkss_write_empty_ptable(struct rkss_file_table *pfile_table)
{
	int area_index;

	area_index = rkss_get_larger_area();
	if (area_index < 0) {
		printf("%s: get area index failed!\n", __func__);
		return -1;
	}

	return rkss_write_area_empty_ptable(area_index, pfile_table);
}

static int rkss_write_back_ptable(int fd, struct rkss_file_table *pfile_table)
{
	struct rkss_file_table *p;
	unsigned int area_index;

	area_index = fd / (RKSS_TABLE_COUNT * RKSS_EACH_FILEFOLDER_COUNT);
	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: area_index invalid\n", __func__);
		return -1;
	}

	p = rkss_info[area_index].table;
	if (p == NULL) {
		printf("%s: table is null\n", __func__);
		return -1;
	}

	p += fd % (RKSS_TABLE_COUNT * RKSS_EACH_FILEFOLDER_COUNT);

	memcpy(p, pfile_table, sizeof(struct rkss_file_table));
	debug("%s: write ptable : name:%s, index:%d, size:%d, used:%d\n",
		__func__, p->name, p->index, p->size, p->used);

	rkss_info[area_index].header->backup_dirty = 1;
	return 0;
}

static int rkss_storage_write(void)
{
	int ret, i;
	int dirty_count = 0;
	int dirty_num = 0;

	for (i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		if (rkss_info[i].header != NULL && rkss_info[i].header->backup_dirty == 1)
			dirty_count++;
	}

	for (i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		if (rkss_info[i].header != NULL && rkss_info[i].header->backup_dirty == 1) {
			rkss_info[i].header->backup_count++;
			rkss_info[i].footer->backup_count = rkss_info[i].header->backup_count;
			rkss_info[i].header->backup_index++;
			if (rkss_info[i].header->backup_index >= RKSS_BACKUP_NUM)
				rkss_info[i].header->backup_index = 0;
			rkss_info[i].header->backup_dirty = 0;
			dirty_num++;
			rkss_info[i].header->sync_flag = SYNC_NONE;
			if (dirty_count > 1) {
				if (dirty_num == dirty_count)
					rkss_info[i].header->sync_flag = SYNC_DONE;
				else
					rkss_info[i].header->sync_flag = SYNC_DOING;
			}

			if (rkss_info[i].header->backup_count == 0xffffffff) {
				rkss_info[i].header->backup_count = 1;
				rkss_info[i].footer->backup_count = 1;
				ret = blk_dwrite(dev_desc,
					part_info.start + i * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
					rkss_info[i].header->backup_index * RKSS_SECTION_COUNT,
					RKSS_SECTION_COUNT, rkss_buffer[i]);
				if (ret != RKSS_SECTION_COUNT) {
					printf("%s: blk_dwrite failed!\n", __func__);
					return -1;
				}

				rkss_info[i].header->backup_count = 2;
				rkss_info[i].footer->backup_count = 2;
				rkss_info[i].header->backup_index++;
				if (rkss_info[i].header->backup_index >= RKSS_BACKUP_NUM)
					rkss_info[i].header->backup_index = 0;
				ret = blk_dwrite(dev_desc,
					part_info.start + i * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
					rkss_info[i].header->backup_index * RKSS_SECTION_COUNT,
					RKSS_SECTION_COUNT, rkss_buffer[i]);
				if (ret != RKSS_SECTION_COUNT) {
					printf("%s: blk_dwrite failed!\n", __func__);
					return -1;
				}
			} else {
				ret = blk_dwrite(dev_desc,
					part_info.start + i * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
					rkss_info[i].header->backup_index * RKSS_SECTION_COUNT,
					RKSS_SECTION_COUNT, rkss_buffer[i]);
				if (ret != RKSS_SECTION_COUNT) {
					printf("%s: blk_dwrite failed!\n", __func__);
					return -1;
				}
			}
		}
	}
	return 0;
}

static int rkss_storage_clean_sync(void)
{
	for (int i = 0; i < RKSS_MAX_AREA_NUM; i++) {
		if (rkss_info[i].header != NULL && rkss_info[i].header->sync_flag != SYNC_NONE) {
			rkss_info[i].header->backup_count++;
			rkss_info[i].footer->backup_count = rkss_info[i].header->backup_count;
			rkss_info[i].header->backup_index++;
			if (rkss_info[i].header->backup_index >= RKSS_BACKUP_NUM)
				rkss_info[i].header->backup_index = 0;
			rkss_info[i].header->backup_dirty = 0;
			rkss_info[i].header->sync_flag = SYNC_NONE;

			int ret = blk_dwrite(dev_desc,
					     part_info.start + i * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
					     rkss_info[i].header->backup_index * RKSS_SECTION_COUNT,
					     RKSS_SECTION_COUNT, rkss_buffer[i]);
			if (ret != RKSS_SECTION_COUNT) {
				printf("blk_dwrite fail \n");
				return -1;
			}
		}
	}
	return 0;
}

static int rkss_storage_init(uint32_t area_index)
{
	unsigned long ret = 0;
	uint32_t size, i;
	uint32_t max_ver = 0;
	uint32_t max_index = 0;
	uint32_t flags_offset, table_offset, data_offset, footer_offset;

	if (area_index >= RKSS_MAX_AREA_NUM) {
		printf("%s: Not support index=0x%x\n", __func__, area_index);
		return -1;
	}

	size = RKSS_SECTION_COUNT * RKSS_DATA_LEN;
	flags_offset = RKSS_USEDFLAGS_INDEX * RKSS_DATA_LEN;
	table_offset = RKSS_TABLE_INDEX * RKSS_DATA_LEN;
	data_offset = RKSS_DATA_INDEX * RKSS_DATA_LEN;
	footer_offset = RKSS_FOOTER_INDEX * RKSS_DATA_LEN;

	if (rkss_buffer[area_index] == NULL) {
		/* Always use, no need to release */
		rkss_buffer[area_index] = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, size);
		if (!(rkss_buffer[area_index])) {
			printf("%s: Malloc failed!\n", __func__);
			return -1;
		}

		/* Pointer initialization */
		rkss_info[area_index].header = (struct rkss_file_header *)(rkss_buffer[area_index]);
		rkss_info[area_index].flags = (uint8_t *)(rkss_buffer[area_index] + flags_offset);
		rkss_info[area_index].table = (struct rkss_file_table *)(rkss_buffer[area_index] + table_offset);
		rkss_info[area_index].data = (uint8_t *)(rkss_buffer[area_index] + data_offset);
		rkss_info[area_index].footer = (struct rkss_file_footer *)(rkss_buffer[area_index] + footer_offset);

		/* Find valid from (backup0 - backup1) */
		for (i = 0; i < RKSS_BACKUP_NUM; i++) {
			ret = blk_dread(dev_desc,
				part_info.start +
				area_index * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
				i * RKSS_SECTION_COUNT,
				RKSS_SECTION_COUNT, rkss_buffer[area_index]);
			if (ret != RKSS_SECTION_COUNT) {
				printf("%s: blk_dread failed!\n", __func__);
				return -1;
			}

			if ((rkss_info[area_index].header->tag == RKSS_TAG) &&
			    (rkss_info[area_index].footer->backup_count == rkss_info[area_index].header->backup_count)) {
				if (max_ver < rkss_info[area_index].header->backup_count) {
					max_index = i;
					max_ver = rkss_info[area_index].header->backup_count;
				}
			}
		}

		if (max_ver) {
			debug("%s: max_ver=%d, max_index=%d.\n",
				__func__, max_ver, max_index);

			if (max_index != (RKSS_BACKUP_NUM - 1)) {
				ret = blk_dread(dev_desc,
					part_info.start +
					area_index * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
					max_index * RKSS_SECTION_COUNT,
					RKSS_SECTION_COUNT, rkss_buffer[area_index]);
				if (ret != RKSS_SECTION_COUNT) {
					printf("%s: blk_dread failed!\n", __func__);
					return -1;
				}
			}

			if (rkss_info[area_index].header->version == RKSS_VERSION_V2) {
				debug("%s: data version equal to image version, do nothing!\n", __func__);
			} else if (rkss_info[area_index].header->version < RKSS_VERSION_V2) {
				printf("%s: data version lower than image version!\n", __func__);
				/* convert rkss version 2 to higher rkss version */
				free(rkss_buffer[area_index]);
				rkss_buffer[area_index] = NULL;
				return -1;
			} else {
				printf("%s: data version higher than image version!\n", __func__);
				printf("%s: please update image!\n", __func__);
				free(rkss_buffer[area_index]);
				rkss_buffer[area_index] = NULL;
				return -1;
			}
		} else {
			printf("%s: Reset area[%d] info...\n", __func__, area_index);
			memset(rkss_buffer[area_index], 0, size);
			rkss_info[area_index].header->tag = RKSS_TAG;
			rkss_info[area_index].header->version = RKSS_VERSION_V2;
			rkss_info[area_index].header->backup_count = 1;
			rkss_info[area_index].footer->backup_count = 1;
			/* Verify Usedflags Section */
			if (rkss_verify_usedflags(area_index) < 0) {
				printf("%s: rkss_verify_usedflags failed!\n", __func__);
				return -1;
			}
		}
	}
	return 0;
}

static int rkss_check_sync_done(void)
{
	int ret;

	if (rkss_info[0].header->sync_flag == SYNC_DOING &&
	    rkss_info[1].header->sync_flag != SYNC_DONE) {
		//rkss_info[0] need rolled back
		rkss_info[0].header->backup_index++;
		if (rkss_info[0].header->backup_index >= RKSS_BACKUP_NUM)
			rkss_info[0].header->backup_index = 0;
		ret = blk_dread(dev_desc,
				part_info.start + 0 * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
				rkss_info[0].header->backup_index * RKSS_SECTION_COUNT,
				RKSS_SECTION_COUNT, rkss_buffer[0]);
		if (ret != RKSS_SECTION_COUNT) {
			printf("blk_dread fail! \n");
			return -1;
		}
		if ((rkss_info[0].header->tag != RKSS_TAG) ||
		    (rkss_info[0].footer->backup_count != rkss_info[0].header->backup_count)) {
			printf("check header fail! \n");
			return -1;
		}

		rkss_info[0].header->backup_index++;
		if (rkss_info[0].header->backup_index >= RKSS_BACKUP_NUM)
			rkss_info[0].header->backup_index = 0;
		ret = blk_dwrite(dev_desc,
				 part_info.start + 0 * RKSS_SECTION_COUNT * RKSS_BACKUP_NUM +
				 rkss_info[0].header->backup_index * RKSS_SECTION_COUNT,
				 RKSS_SECTION_COUNT, rkss_buffer[0]);
		if (ret != RKSS_SECTION_COUNT) {
			printf("blk_dwrite fail! \n");
			return -1;
		}
	}
	ret = rkss_storage_clean_sync();
	if (ret) {
		printf("clean sync flag fail! \n");
		return -1;
	}
	return 0;
}

static uint32_t ree_fs_new_open(size_t num_params,
				struct optee_msg_param *params)
{
	char *filename;
	int fd;
	struct rkss_file_table p = {0};
	unsigned int area_index;

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(filename) > RKSS_NAME_MAX_LENGTH) {
		printf("%s: file name too long. %s\n", __func__, filename);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s: open file: %s, len: %zu\n",
		__func__, filename, strlen(filename));
	fd = rkss_get_fileinfo_by_name(filename, &p, &area_index);
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
	struct rkss_file_table p = {0};
	unsigned int area_index;

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(filename) > RKSS_NAME_MAX_LENGTH) {
		printf("%s: file name too long. %s\n", __func__, filename);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s create file: %s, len: %zu\n",
		__func__, filename, strlen(filename));
	fd = rkss_get_fileinfo_by_name(filename, &p, &area_index);
	if (fd >= 0) {
		printf("%s : file exist, clear it. %s\n", __func__, filename);
		/* decrease ref from usedflags */
		num = p.size / RKSS_DATA_LEN + 1;
		ret = rkss_decref_multi_usedflags_sections(area_index, p.index, num);
		if (ret < 0) {
			printf("%s: rkss_decref_multi_usedflags_sections error!\n", __func__);
			return TEE_ERROR_GENERIC;
		}

		/* rm from ptable */
		memset(&p, 0, sizeof(struct rkss_file_table));
		ret = rkss_write_back_ptable(fd, &p);
		if (ret < 0) {
			printf("%s : write back error %d\n", __func__, ret);
			return TEE_ERROR_GENERIC;
		}
	}

	strcpy(p.name, filename);
	p.index = 0;
	p.size = 0;
	p.used = 1;
	p.flags = RK_FS_R | RK_FS_W;
	fd = rkss_write_empty_ptable(&p);
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

static u32 ree_fs_new_read(size_t num_params,
			   struct optee_msg_param *params)
{
	uint8_t *data;
	size_t len;
	off_t offs;
	int fd;
	int ret;
	struct rkss_file_table p = {0};
	int di, section_num;
	uint8_t *temp_file_data;
	unsigned int area_index;

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;

	data = tee_supp_param_to_va(params + 1);
	if (!data)
		return TEE_ERROR_BAD_PARAMETERS;
	len = params[1].u.rmem.size;

	debug("%s: fd:%d, len:%zu, offs:%ld\n",
		__func__, fd, len, offs);

	ret = rkss_get_fileinfo_by_index(fd, &p, &area_index);
	if (ret < 0) {
		printf("%s: unavailable fd: %d!\n", __func__, fd);
		return TEE_ERROR_GENERIC;
	}

	if (offs >= p.size)
		return TEE_ERROR_BAD_PARAMETERS;

	section_num = p.size / RKSS_DATA_LEN + 1;
	temp_file_data = malloc(section_num * RKSS_DATA_LEN);
	if (!temp_file_data) {
		printf("%s: Malloc failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	ret = rkss_read_multi_sections(area_index, temp_file_data, p.index, section_num);
	if (ret < 0) {
		printf("%s: unavailable file index!\n", __func__);
		free(temp_file_data);
		return TEE_ERROR_GENERIC;
	}

	di = (offs + len) > p.size ? (p.size - offs) : len;
	memcpy(data, temp_file_data + offs, di);
	free(temp_file_data);
	temp_file_data = 0;
	params[1].u.rmem.size = di;

	return TEE_SUCCESS;
}

static u32 ree_fs_new_write(size_t num_params,
			    struct optee_msg_param *params)
{
	uint8_t *data;
	size_t len;
	off_t offs;
	struct rkss_file_table p = {0};
	int ret, fd, new_size;
	int section_num;
	uint8_t *file_data = 0, *temp_file_data = 0;
	unsigned int area_index;

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;

	data = tee_supp_param_to_va(params + 1);
	if (!data)
		return TEE_ERROR_BAD_PARAMETERS;
	len = params[1].u.rmem.size;

	debug("%s: fd:%d, len:%zu, offs:%ld\n",
		__func__, fd, len, offs);

	ret = rkss_get_fileinfo_by_index(fd, &p, &area_index);
	if (ret < 0) {
		printf("%s: fd:%d unvailable!\n", __func__, fd);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	new_size = offs + len > p.size ? offs + len : p.size;
	file_data = malloc(new_size);
	if (!file_data)
		return TEE_ERROR_OUT_OF_MEMORY;

	if (p.size != 0) {
		/* Read old file data out */
		section_num = p.size / RKSS_DATA_LEN + 1;
		temp_file_data = malloc(section_num * RKSS_DATA_LEN);
		if (!temp_file_data)
			return TEE_ERROR_OUT_OF_MEMORY;

		ret = rkss_read_multi_sections(area_index, temp_file_data, p.index, section_num);
		if (ret < 0) {
			printf("%s: unavailable file index %d section_num %d\n",
					__func__, p.index, section_num);
			ret = TEE_ERROR_GENERIC;
			goto out;
		}
		memcpy(file_data, temp_file_data, p.size);
		free(temp_file_data);
		temp_file_data = 0;
		ret = rkss_decref_multi_usedflags_sections(area_index, p.index, section_num);
		if (ret < 0) {
			printf("%s: rkss_decref_multi_usedflags_sections error!\n", __func__);
			ret = TEE_ERROR_GENERIC;
			goto out;
		}
	}

	/* update new file info */
	memcpy(file_data + offs, data, len);
	p.size = new_size;
	section_num = new_size / RKSS_DATA_LEN + 1;
	p.index = rkss_get_empty_section_from_usedflags(area_index, section_num);
	debug("%s: Get Empty section in %d\n", __func__, p.index);
	p.used = 1;
	ret = rkss_incref_multi_usedflags_sections(area_index, p.index, section_num);
	if (ret < 0) {
		printf("%s: rkss_incref_multi_usedflags_sections error!\n", __func__);
		ret = TEE_ERROR_GENERIC;
		goto out;
	}

	ret = rkss_write_back_ptable(fd, &p);
	if (ret < 0) {
		printf("%s: write ptable error!\n", __func__);
		ret = TEE_ERROR_GENERIC;
		goto out;
	}

	/* write new file data */
	temp_file_data = malloc(section_num * RKSS_DATA_LEN);
	if (!temp_file_data)
		return TEE_ERROR_OUT_OF_MEMORY;

	memset(temp_file_data, 0, section_num * RKSS_DATA_LEN);
	memcpy(temp_file_data, file_data, p.size);
	rkss_write_multi_sections(area_index, temp_file_data, p.index, section_num);
	free(temp_file_data);
	temp_file_data = 0;

out:
	if (file_data) {
		free(file_data);
	}
	if (temp_file_data) {
		free(temp_file_data);
		temp_file_data = 0;
	}

	return TEE_SUCCESS;
}

/* TODO: update file data space */
static u32 ree_fs_new_truncate(size_t num_params,
			       struct optee_msg_param *params)
{
	size_t len;
	int fd, ret;
	struct rkss_file_table p = {0};
	unsigned int section_num_old, section_num_new;
	unsigned int area_index;

	fd = params[0].u.value.b;
	len = params[0].u.value.c;

	debug("%s: fd:%d, lenth:%zu\n", __func__, fd, len);

	ret = rkss_get_fileinfo_by_index(fd, &p, &area_index);
	if (ret < 0) {
		printf("%s: fd:%d unvailable!\n", __func__, fd);
		return TEE_ERROR_GENERIC;
	}
	if (len > p.size) {
		printf("%s: truncate error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	section_num_old = p.size / RKSS_DATA_LEN + 1;
	section_num_new = len / RKSS_DATA_LEN + 1;
	ret = rkss_decref_multi_usedflags_sections(area_index, p.index + section_num_new, section_num_old - section_num_new);
	if (ret < 0) {
		printf("%s: rkss_decref_multi_usedflags_sections error!\n", __func__);
		ret = TEE_ERROR_GENERIC;
	}
	p.size = len;
	ret = rkss_write_back_ptable(fd, &p);
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
	struct rkss_file_table p = {0};
	int ret, fd, num;
	unsigned int area_index;

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	ret = rkss_get_fileinfo_by_name(filename, &p, &area_index);
	if (ret < 0) {
		printf("%s: no such file. %s\n", __func__, filename);
		return 0;
	}
	fd = ret;

	debug("%s: %s fd:%d index:%d size:%d\n",
		__func__, filename, fd, p.index, p.size);

	/* decrease ref from usedflags */
	num = p.size / RKSS_DATA_LEN + 1;
	ret = rkss_decref_multi_usedflags_sections(area_index, p.index, num);
	if (ret < 0) {
		printf("%s: rkss_decref_multi_usedflags_sections error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	/* rm from ptable */
	memset(&p, 0, sizeof(struct rkss_file_table));
	ret = rkss_write_back_ptable(fd, &p);
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
	struct rkss_file_table p = {0};
	int ret;
	unsigned int area_index;

	old_fname = tee_supp_param_to_va(params + 1);
	if (!old_fname)
		return TEE_ERROR_BAD_PARAMETERS;

	new_fname = tee_supp_param_to_va(params + 2);
	if (!new_fname)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(new_fname) > RKSS_NAME_MAX_LENGTH) {
		printf("%s: new file name too long. %s\n", __func__, new_fname);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s: rename: %s -> %s\n", __func__, old_fname, new_fname);

	ret = rkss_get_fileinfo_by_name(old_fname, &p, &area_index);
	if (ret < 0) {
		printf("%s: filename: %s no found.\n", __func__, old_fname);
		return TEE_ERROR_ITEM_NOT_FOUND;
	}

	strcpy(p.name, new_fname);

	ret = rkss_write_back_ptable(ret, &p);
	if (ret < 0) {
		printf("%s: write ptable error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

static u32 ree_fs_new_opendir(size_t num_params,
			      struct optee_msg_param *params)
{
	char *dirname;
	int ret;

	dirname = tee_supp_param_to_va(params + 1);
	if (!dirname)
		return TEE_ERROR_BAD_PARAMETERS;

	dir_seek = 0;
	ret = rkss_get_dirs_by_name(dirname);
	if (ret < 0) {
		printf("%s: rkss_get_dirs_by_name error\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	debug("%s: %s, seek/num:%d/%d\n",
		__func__, dirname, dir_seek, dir_num);
	return TEE_SUCCESS;
}

static u32 ree_fs_new_closedir(size_t num_params,
			       struct optee_msg_param *params)
{
	if (num_params != 1 ||
	    (params[0].attr & TEE_PARAM_ATTR_TYPE_MASK) !=
			TEE_PARAM_ATTR_TYPE_VALUE_INPUT)
		return TEE_ERROR_BAD_PARAMETERS;

	dir_seek = 0;
	dir_num = 0;

	return TEE_SUCCESS;
}

static u32 ree_fs_new_readdir(size_t num_params,
			      struct optee_msg_param *params)
{
	char *dirname;
	size_t len;
	size_t dirname_len;

	dirname = tee_supp_param_to_va(params + 1);
	if (!dirname)
		return TEE_ERROR_BAD_PARAMETERS;
	len = params[1].u.rmem.size;

	debug("%s: seek/num:%d/%d\n", __func__, dir_seek, dir_num);
	if (dir_seek == dir_num) {
		params[1].u.rmem.size = 0;
		debug("%s: END\n", __func__);
		return TEE_ERROR_EXCESS_DATA;
	}

	dirname_len = strlen(dir_cache[dir_seek]) + 1;
	params[1].u.rmem.size = dirname_len;
	if (dirname_len > len)
		return TEE_ERROR_SHORT_BUFFER;

	strcpy(dirname, dir_cache[dir_seek]);
	++dir_seek;

	debug("%s: %s\n", __func__, dirname);

	return TEE_SUCCESS;
}

int tee_supp_rk_fs_init_v2(void)
{
	assert(sizeof(struct rkss_file_table) == RKSS_TABLE_SIZE);
	assert(RKSS_DATA_LEN / sizeof(struct rkss_file_table) == RKSS_EACH_FILEFOLDER_COUNT);

	if (check_security_exist(0) < 0)
		return -1;

	/* clean secure storage */
#ifdef DEBUG_CLEAN_RKSS
	if (rkss_storage_reset() < 0)
		return -1;
#endif

	for (uint32_t i = 0; i < RKSS_ACTIVE_AREA_NUM; i++) {
		if (rkss_storage_init(i) < 0)
			return -1;
	}

	if (rkss_check_sync_done() < 0)
		return -1;

#ifdef DEBUG_RKSS
	rkss_dump_ptable();
	rkss_dump_usedflags();
#endif

	return 0;
}

static int rkss_step;
int tee_supp_rk_fs_process_v2(size_t num_params,
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
		rkss_storage_write();
		rkss_storage_clean_sync();
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
		rkss_storage_write();
		rkss_storage_clean_sync();
		break;
	case OPTEE_MRF_RENAME:
		debug(">>>>>>> [%d] OPTEE_MRF_RENAME!\n", rkss_step++);
		ret = ree_fs_new_rename(num_params, params);
		rkss_storage_write();
		rkss_storage_clean_sync();
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
	return ret;
}
