/*
 * Copyright (c) 2016, Fuzhou Rockchip Electronics Co.,Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *																		Created by jeffry.zhang@rock-chips.com
 */
#include <common.h>
#include <stdlib.h>
#include <command.h>

//#define DEBUG_RKFSS
//#define DEBUG_CLEAN_RKSS

/*
 * Operations and defines shared with TEE.
 */
#define TEE_FS_OPEN       1
#define TEE_FS_CLOSE      2
#define TEE_FS_READ       3
#define TEE_FS_WRITE      4
#define TEE_FS_SEEK       5
#define TEE_FS_UNLINK     6
#define TEE_FS_RENAME     7
#define TEE_FS_TRUNC      8
#define TEE_FS_MKDIR      9
#define TEE_FS_OPENDIR   10
#define TEE_FS_CLOSEDIR  11
#define TEE_FS_READDIR   12
#define TEE_FS_RMDIR     13
#define TEE_FS_ACCESS    14
#define TEE_FS_LINK      15

/*
 * Open flags, defines shared with TEE.
 */
#define TEE_FS_O_RDONLY 0x1
#define TEE_FS_O_WRONLY 0x2
#define TEE_FS_O_RDWR   0x4
#define TEE_FS_O_CREAT  0x8
#define TEE_FS_O_EXCL   0x10
#define TEE_FS_O_APPEND 0x20

/*
 * Seek flags, defines shared with TEE.
 */
#define TEE_FS_SEEK_SET 0x1
#define TEE_FS_SEEK_END 0x2
#define TEE_FS_SEEK_CUR 0x4

/*
 * Mkdir flags, defines shared with TEE.
 */
#define TEE_FS_S_IWUSR 0x1
#define TEE_FS_S_IRUSR 0x2

/*
 * Access flags, X_OK not supported, defines shared with TEE.
 */
#define TEE_FS_R_OK    0x1
#define TEE_FS_W_OK    0x2
#define TEE_FS_F_OK    0x4

/*
 *	RK Secure Storage Ctrl
 *		Storage Size : 512 kb
 *		Header Size : 8 byte * 2 for each top of 512 byte
 *		Partision Table Size : 128 * 512 b (24 Files And Folder)
 *		File number: 128 * 4 = 512
 *		Data Size : 895 * 512 b
 *
 *	------ RKSS Structure --------
 *	- 512 byte patition table1 [0]
 *		- 126 * 4 = 504 byte table info
 *		- 8 byte verification
 *	- 512 byte patition table2 [1]
 *	             ...
 *	- 512 byte patition table128 [127]
 *	- 512 byte section used refs [128]
 *		- 1 byte = 2 flag
 *	- 895 * 512 byte data	[129 - 1023]
 *	------------------------------
 *
 */
#define RKSS_DATA_SECTION_COUNT		1024
#define RKSS_DATA_LEN			512
#define RKSS_PARTITION_TABLE_COUNT	128		// total size 512 * 128
#define RKSS_EACH_FILEFOLDER_COUNT	4		// 504 / 126 = 4
#define RKSS_NAME_MAX_LENGTH		117		// 116 char + "\0"
#define RKSS_USEDFLAGS_INDEX		RKSS_PARTITION_TABLE_COUNT

typedef struct rkss_file_info
{
	uint8_t		used;
	char 		name[RKSS_NAME_MAX_LENGTH];
	uint16_t	index;	// from 129 to 1024
	uint16_t	size;	// size of data
	uint16_t	father;
	uint8_t 	id; // file folder count index
#define RK_FS_R    0x1
#define RK_FS_W    0x2
#define RK_FS_D    0x8
	uint8_t		flags;
}rkss_file_info; // 126 byte for each

#define RKSS_VERSION		(uint32_t)0x1
#define RKSS_CHECK_STR	(uint32_t)0x12345678
#define RKSS_CHECK_PT		(uint8_t)0xFC
typedef struct rkss_file_verification
{
	uint32_t version;
	uint32_t checkstr;
}rkss_file_verification; // 8 byte

typedef struct rk_secure_storage
{
	unsigned long index;
	unsigned char data[RKSS_DATA_LEN];
}rk_secure_storage;

/* Path to all secure storage dev. */
#define RKSS_DEV "/dev/block/rknand_security"

/* Function Defines */
#define UNREFERENCED_PARAMETER(P) (P=P)
#define CHECKFLAG(flags, flag) (flags & flag)
#define ADDFLAG(flags, flag) (flags | flag)

/* RK Secure Storage Calls */
static int file_seek = 0;
static char dir_cache[RKSS_NAME_MAX_LENGTH][12];
static int dir_num = 0;
static int dir_seek = 0;

extern struct blk_desc *rockchip_get_bootdev(void);
extern int part_get_info_by_name(struct blk_desc *dev_desc, const char *name,
	disk_partition_t *info);
extern unsigned long blk_dread(struct blk_desc *block_dev, lbaint_t start,
			lbaint_t blkcnt, void *buffer);

extern unsigned long blk_dwrite(struct blk_desc *block_dev, lbaint_t start,
			 lbaint_t blkcnt, const void *buffer);

static struct blk_desc *dev_desc = NULL;
static disk_partition_t part_info;
static int rkss_read_multi_sections(unsigned char *data, unsigned long index, unsigned int num)
{
	unsigned long ret;

	if (dev_desc == NULL) {
		dev_desc = rockchip_get_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device\n", __func__);
			return -1;
		}

		if (part_get_info_by_name(dev_desc, "security", &part_info) < 0) {
			printf("Could not find security partition\n");
			return -1;
		}
	}
	ret = blk_dread(dev_desc, part_info.start + index, num, data);
	if (ret != num) {
		printf("blk_dread fail \n");
		return -1;
	}
	return 0;
}

static int rkss_read_section(struct rk_secure_storage *rkss)
{
	return rkss_read_multi_sections(rkss->data, rkss->index, 1);
}

static int rkss_write_multi_sections(unsigned char *data, unsigned long index, unsigned int num)
{
	unsigned long ret;

	if (dev_desc == NULL) {
		dev_desc = rockchip_get_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device\n", __func__);
			return -1;
		}

		if (part_get_info_by_name(dev_desc, "security", &part_info) < 0) {
			printf("Could not find security partition\n");
			return -1;
		}
	}
	ret = blk_dwrite(dev_desc, part_info.start + index, num, data);
	if (ret != num) {
		printf("blk_dwrite fail \n");
		return -1;
	}
	return 0;
}

static int rkss_write_section(struct rk_secure_storage *rkss)
{
	return rkss_write_multi_sections(rkss->data, rkss->index, 1);
}

static int rkss_read_patition_tables(unsigned char *data)
{
	unsigned long ret;

	if (dev_desc == NULL) {
		dev_desc = rockchip_get_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device\n", __func__);
			return -1;
		}

		if (part_get_info_by_name(dev_desc, "security", &part_info) < 0) {
			printf("Could not find security partition\n");
			return -1;
		}
	}
	ret = blk_dread(dev_desc, part_info.start, RKSS_PARTITION_TABLE_COUNT, data);
	if (ret != RKSS_PARTITION_TABLE_COUNT) {
		printf("blk_dread fail \n");
		return -1;
	}
	return 0;
}

#ifdef DEBUG_RKFSS
static void rkss_dump(void* data, unsigned int len)
{
	char *p = (char *)data;
	unsigned int i = 0;
	printf("-------------- DUMP %d --------------", len);
	for (i = 0; i < len; i++)
	{
		printf("%02x ", *(p + i));
	}
	printf("\n");
	printf("------------- DUMP END -------------");
}

static void rkss_dump_ptable(void)
{
	printf("-------------- DUMP ptable --------------");
	int i = 0, ret;
	unsigned char *table_data;

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		free(table_data);
		return;
	}

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++)
	{
		struct rk_secure_storage rkss = {0};
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);

		int n ;
		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++)
		{
			void *pdata = rkss.data;
			struct rkss_file_info *p = (struct rkss_file_info *)pdata;
			p += n;

			printf("[%02d][%c] %s , inx:%d, size:%d",
					i*RKSS_EACH_FILEFOLDER_COUNT+n, p->used == 0 ? 'F':'T' ,p->name,
					p->index, p->size);
		}
	}
	free(table_data);
	printf("-------------- DUMP END --------------");
}

static void rkss_dump_usedflags(void)
{
	struct rk_secure_storage rkss = {0};
	rkss.index = RKSS_USEDFLAGS_INDEX;
	int ret = rkss_read_section(&rkss);
	if (ret < 0)
	{
		printf("rkss_read_section fail ! ret: %d.", ret);
		return;
	}
	rkss_dump(rkss.data, RKSS_DATA_LEN);
}
#endif

static int rkss_verify_ptable(unsigned char *table_data)
{
	unsigned char *cp, *vp;
	struct rkss_file_verification *verify;
	int ret, i;

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		cp = table_data + (i * RKSS_DATA_LEN);
		vp = cp + RKSS_DATA_LEN - sizeof(struct rkss_file_verification);
		verify = (struct rkss_file_verification *)(void *)vp;

		if (verify->version != RKSS_VERSION
				|| verify->checkstr != RKSS_CHECK_STR) {
			printf("verify [%d] fail, cleanning ....", i);
			memset(cp, 0, RKSS_DATA_LEN);
			verify->checkstr = RKSS_CHECK_STR;
			verify->version = RKSS_VERSION;
		}
	}
	ret = rkss_write_multi_sections(table_data, 0, RKSS_PARTITION_TABLE_COUNT);
	if (ret < 0) {
		printf("rkss_write_multi_sections failed!!! ret: %d.", ret);
		return -1;
	}
	debug("verify ptable success.");
	return 0;
}

static int rkss_verify_usedflags(struct rk_secure_storage *rkss)
{
	uint8_t *flags = (uint8_t *)rkss->data;
	int i, duel, flag, n, value, ret;
	uint8_t *flagw;

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT + 1; i++) {
		duel = *(flags + (int)i/2);
		flag = i & 0x1 ? duel & 0x0F : (duel & 0xF0) >> 4;
		if (flag != 0x1) {
			debug("init usedflags section ...");
			memset(rkss->data, 0x00, RKSS_DATA_LEN);
			for (n = 0; n < RKSS_PARTITION_TABLE_COUNT + 1; n++) {
				flagw = (uint8_t *)rkss->data + (int)n/2;
				value = 0x1;
				*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
						(*flagw & 0x0F) | (value << 4);
			}
			ret = rkss_write_multi_sections(rkss->data, rkss->index, 1);
			if (ret < 0) {
				printf("clean usedflags section failed!!! ret: %d.", ret);
				return -1;
			}

			return 0;
		}
	}
	debug("rkss_verify_usedflags: sucess.");
	return 0;
}

static int rkss_get_fileinfo_by_index(int fd, struct rkss_file_info *pfileinfo)
{
	int i = fd / RKSS_EACH_FILEFOLDER_COUNT;
	int n = fd - (RKSS_EACH_FILEFOLDER_COUNT * i);
	struct rk_secure_storage rkss = {0};
	int ret;
	void *pdata;
	struct rkss_file_info *p;

	rkss.index = i;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
		return -1;
	}

	pdata = rkss.data;
	p = (struct rkss_file_info *)pdata;
	p += n;

	if (p->used != 1) {
		debug("error: unused section! ");
		return -1;
	}
	debug("rkss_get_fileinfo_by_index p->used = %d p->name=%s p->index=%d p->size=%d \n",
		p->used, p->name, p->index, p->size);
	memcpy(pfileinfo, p, sizeof(struct rkss_file_info));

	return 0;
}

static int rkss_get_fileinfo_by_name(
		char* filename, struct rkss_file_info *pfileinfo)
{
	int i = 0, ret;
	uint8_t n = 0;
	unsigned int len;
	unsigned char *table_data;

	len = strlen(filename);
	if (len > RKSS_NAME_MAX_LENGTH - 1)
	{
		printf("filename is too long. length:%u", len);
		return -1;
	}

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return -1;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		free(table_data);
		return -1;
	}

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++)
	{
		struct rk_secure_storage rkss = {0};
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);

		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++)
		{
			void *pdata = rkss.data;
			struct rkss_file_info *p = (struct rkss_file_info *)pdata;
			p += n;

			if (p->used == 0)
				continue;

			if (!strcmp(p->name, filename))
			{
				debug("rkss_get_fileinfo_by_name: hit table[%d/%d], index[%d/%d]",
						i, RKSS_PARTITION_TABLE_COUNT, n, RKSS_EACH_FILEFOLDER_COUNT);
				memcpy(pfileinfo, p, sizeof(struct rkss_file_info));
				free(table_data);
				return i * RKSS_EACH_FILEFOLDER_COUNT + n;
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

			} while(cur_inpos && cur_svpos);

			debug("Matched folder: %s", p->name);
			free(table_data);
			return -100;
UNMATCHFOLDER:
			debug("Unmatched ...");
		}
	}
	debug("rkss_get_fileinfo_by_name: file or dir no found!");
	free(table_data);
	return -1;
}

static int rkss_get_dirs_by_name(char* filename)
{
	int i = 0, ret;
	uint8_t n = 0;
	unsigned int len;
	unsigned char *table_data;

	len = strlen(filename);
	if (len > RKSS_NAME_MAX_LENGTH - 1)
	{
		printf("filename is too long. length:%u", len);
		return -1;
	}

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return -1;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		free(table_data);
		return -1;
	}

	dir_num = 0;
	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++)
	{
		struct rk_secure_storage rkss = {0};
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);

		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++)
		{
			void *pdata = rkss.data;
			struct rkss_file_info *p = (struct rkss_file_info *)pdata;
			p += n;

			if (p->used == 0)
				continue;

			// Full Matching
			ret = memcmp(p->name, filename, strlen(filename));
			debug("comparing [fd:%d] : %s ?= %s , ret:%d",
					i*RKSS_EACH_FILEFOLDER_COUNT+n, p->name, filename, ret);
			if (!ret && strlen(p->name) > strlen(filename))
			{
				char *chk = p->name + strlen(filename);
				if (*chk == '/')
				{
					char *file = p->name + strlen(filename) + 1;
					char *subdir = strtok(file, "/");
					printf("found: %s", subdir);
					strcpy(dir_cache[dir_num], subdir);
					++dir_num;
				}
			}
		}
	}
	free(table_data);
	return dir_num;
}

static int rkss_get_empty_section_from_usedflags(int section_size)
{
	struct rk_secure_storage rkss = {0};
	rkss.index = RKSS_USEDFLAGS_INDEX;
	int ret = rkss_read_section(&rkss);
	if (ret < 0)
	{
		printf("rkss_read_section fail ! ret: %d.", ret);
		return -1;
	}

	int i = 0;
	int count0 = 0;
	for (i = 0; i < RKSS_DATA_SECTION_COUNT; i++)
	{
		uint8_t *flag = (uint8_t *)rkss.data + (int)i/2;
		uint8_t value = i & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;

		if (value == 0x0)
		{
			if (++count0 == section_size)
			{
				return (i + 1 - section_size);
			}
		}
		else
		{
			count0 = 0;
		}
	}

	printf("Not enough space available in secure storage !");
	return -10;
}

static int rkss_incref_multi_usedflags_sections(unsigned int index, unsigned int num)
{
	struct rk_secure_storage rkss = {0};
	int ret, value, i;
	uint8_t *flag;

	if ((index + num) >= RKSS_DATA_SECTION_COUNT) {
		printf("index[%d] out of range.", index);
		return -1;
	}

	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
		return -1;
	}

	for (i = 0; i < num; i++, index++) {
		flag = (uint8_t *)rkss.data + (int)index/2;
		value = index & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (++value > 0xF) {
			printf("reference out of data: %d", value);
			value = 0xF;
		}
		*flag = index & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_write_multi_sections fail ! ret: %d.", ret);
		return -1;
	}
	return 0;
}

static int rkss_decref_multi_usedflags_sections(unsigned int index, unsigned int num)
{
	struct rk_secure_storage rkss = {0};
	int ret, value, i;
	uint8_t *flag;

	if ((index + num) >= RKSS_DATA_SECTION_COUNT) {
		printf("index[%d] out of range.", index);
		return -1;
	}

	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
		return -1;
	}
	for (i = 0; i < num; i++, index++) {
		flag = (uint8_t *)rkss.data + (int)index/2;
		value = index & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (--value < 0) {
			printf("reference out of data: %d", value);
			value = 0x0;
		}
		*flag = index & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_write_multi_sections fail ! ret: %d.", ret);
		return -1;
	}
	return 0;
}

static int rkss_write_empty_ptable(struct rkss_file_info *pfileinfo)
{
	int i = 0, ret;
	unsigned char *table_data;

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return -1;
	}

	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		free(table_data);
		return -1;
	}

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++)
	{
		struct rk_secure_storage rkss = {0};
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);

		int n = 0;
		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++)
		{
			void *pdata = rkss.data;
			struct rkss_file_info *p = (struct rkss_file_info *)pdata;
			p += n;
			if (p->used == 0)
			{
				debug("write ptable in [%d][%d] .",i ,n);
				memcpy(p, pfileinfo, sizeof(struct rkss_file_info));
				p->used = 1;
				p->id = n;
				debug("write emt ptable : [%d,%d] name:%s, index:%d, size:%d, used:%d",
						i,n,p->name,p->index,p->size,p->used);
				ret = rkss_write_section(&rkss);
				if (ret < 0)
				{
					printf("rkss_write_section fail ! ret: %d.", ret);
					free(table_data);
					return -1;
				}
				free(table_data);
				return i * RKSS_EACH_FILEFOLDER_COUNT + n;
			}
		}
	}
	printf("No enough ptable space available in secure storage.");
	free(table_data);
	return -1;
}

static int rkss_write_back_ptable(int fd, struct rkss_file_info *pfileinfo)
{
	int i = fd / RKSS_EACH_FILEFOLDER_COUNT;
	int n = fd - (RKSS_EACH_FILEFOLDER_COUNT * i);

	struct rk_secure_storage rkss = {0};
	rkss.index = i;
	int ret = rkss_read_section(&rkss);
	if (ret < 0)
	{
		debug("rkss_read_section fail ! ret: %d.", ret);
		return -1;
	}

	void *pdata = rkss.data;
	struct rkss_file_info *p = (struct rkss_file_info *)pdata;
	p += n;

	memcpy(p, pfileinfo, sizeof(struct rkss_file_info));
	debug("write ptable : [%d,%d] name:%s, index:%d, size:%d, used:%d",
			i,n,p->name,p->index,p->size,p->used);

	ret = rkss_write_section(&rkss);
	if (ret < 0)
	{
		debug("rkss_write_section fail ! ret: %d.", ret);
		return -1;
	}

	return 0;
}

/*
 * Structure for file related RPC calls
 *
 * @op     The operation like open, close, read, write etc
 * @flags  Flags to the operation shared with secure world
 * @arg    Argument to operation
 * @fd     NW file descriptor
 * @len    Length of buffer at the end of this struct
 * @res    Result of the operation
 */
struct tee_fs_rpc {
	int op;
	int flags;
	int arg;
	int fd;
	uint32_t len;
	int res;
};

static int tee_fs_open(struct tee_fs_rpc *fsrpc)
{
	int make_newfile = 0;
	char *filename = (char *)(fsrpc + 1);

	if (strlen(filename) > RKSS_NAME_MAX_LENGTH)
	{
		debug("tee_fs_open: file name too long. %s",filename);
		return -1;
	}

	debug("tee_fs_open open file: %s, len: %zu", filename, strlen(filename));
	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_name(filename, &p);
	if (ret < 0)
	{
		debug("tee_fs_open : no such file. %s", filename);
		make_newfile = 1;
	}
	else
	{
		fsrpc->fd = ret;
		file_seek = 0;
		if (CHECKFLAG(fsrpc->flags, TEE_FS_O_APPEND))
		{
			file_seek = p.size;
		}
	}

	if (make_newfile)
	{
		if (CHECKFLAG(fsrpc->flags, TEE_FS_O_CREAT))
		{
			debug("tee_fs_open create new file: %s", filename);
			strcpy(p.name, filename);
			p.index = 0;
			p.size = fsrpc->len;
			p.used = 1;
			p.flags = RK_FS_R | RK_FS_W;
			ret = rkss_write_empty_ptable(&p);
			if (ret < 0)
			{
				printf("tee_fs_open : error. %s", filename);
				return -1;
			}
			fsrpc->fd = ret;
			file_seek = 0;
		}
		else
		{
			debug("and no create flag found.");
			return -1;
		}
	}

	debug("tee_fs_open ! %s , fd:%d, flag: %x, len: %d",
			filename, fsrpc->fd, fsrpc->flags, fsrpc->len);

	return fsrpc->fd;
}

static int tee_fs_close(struct tee_fs_rpc *fsrpc)
{
	debug("tee_fs_close !");
	UNREFERENCED_PARAMETER(fsrpc);
	return 0;
}

static int tee_fs_read(struct tee_fs_rpc *fsrpc)
{
	debug("tee_fs_read! fd:%d, len:%d", fsrpc->fd, fsrpc->len);
	void *data = (void *)(fsrpc + 1);

	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_index(fsrpc->fd, &p);
	if (ret < 0)
	{
		printf("unavailable fd !");
		return -1;
	}

	if (file_seek != 0)
	{
		printf("warning !!! file_seek != 0. unsupported now.");
	}

	int num = fsrpc->len / RKSS_DATA_LEN + 1;
	int di = 0;
	debug("reading section[%d], fd:%d, len:%d, filesize:%d",
			p.index, fsrpc->fd, fsrpc->len, p.size);

	uint8_t *temp_file_data = malloc(num * RKSS_DATA_LEN);
	ret = rkss_read_multi_sections(temp_file_data, p.index, num);
	if (ret < 0) {
		printf("unavailable file index");
		free(temp_file_data);
		return -1;
	}
	di = fsrpc->len > p.size ? p.size : fsrpc->len;
	memcpy(data, temp_file_data, di);
	free(temp_file_data);
	temp_file_data = 0;
	return di;
}

static int tee_fs_write(struct tee_fs_rpc *fsrpc)
{
	debug("tee_fs_write ! fd:%d, lenth:%d",fsrpc->fd, fsrpc->len);
	void *data = (void *)(fsrpc + 1);

	if (fsrpc->fd < 0)
	{
		printf("tee_fs_write error ! wrong fd : %d",fsrpc->fd);
		return -1;
	}

	if (file_seek != 0)
	{
		printf("warning !!! file_seek != 0. unsupported now.");
	}

	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_index(fsrpc->fd, &p);
	if (ret < 0)
	{
		printf("tee_fs_write: fd unvailable!");
		return -1;
	}

	p.size = fsrpc->len;
	int num = fsrpc->len / RKSS_DATA_LEN + 1;
	p.index = rkss_get_empty_section_from_usedflags(num);
	debug("Get Empty section in %d", p.index);
	p.used = 1;

	ret = rkss_incref_multi_usedflags_sections(p.index, num);
	if (ret < 0) {
		printf("rkss_incref_multi_usedflags_sections error !");
		ret = -1;
	}

	ret = rkss_write_back_ptable(fsrpc->fd, &p);
	if (ret < 0)
	{
		printf("tee_fs_write: write ptable error!");
		return -1;
	}

	uint8_t *temp_file_data = malloc(num * RKSS_DATA_LEN);
	memset(temp_file_data, 0, num * RKSS_DATA_LEN);
	memcpy(temp_file_data, data, p.size);
	rkss_write_multi_sections(temp_file_data, p.index, num);
	free(temp_file_data);
	temp_file_data = 0;

#ifdef DEBUG_RKFSS
	rkss_dump_usedflags();
#endif
	return fsrpc->len;
}

static int tee_fs_seek(struct tee_fs_rpc *fsrpc)
{
	debug("tee_fs_seek ! fd:%d, seek:%d, flag:%x", fsrpc->fd, fsrpc->arg, fsrpc->flags);

	if (fsrpc->flags == TEE_FS_SEEK_CUR)
	{
		fsrpc->res = file_seek + fsrpc->arg;
	}
	else if (fsrpc->flags == TEE_FS_SEEK_SET)
	{
		file_seek = fsrpc->arg;
		fsrpc->res = file_seek;
	}
	else if (fsrpc->flags == TEE_FS_SEEK_END)
	{
		struct rkss_file_info p = {0};
		int ret = rkss_get_fileinfo_by_index(fsrpc->fd, &p);
		if (ret < 0)
		{
			printf("unavilable fd.");
			return -1;
		}
		file_seek = p.size + fsrpc->arg;
		fsrpc->res = file_seek;
	}
	else
	{
		printf("tee_fs_seek: unsupport seed mode.");
		return -1;
	}

	return fsrpc->res;
}

static int tee_fs_unlink(struct tee_fs_rpc *fsrpc)
{
	char *filename = (char *)(fsrpc + 1);

	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_name(filename, &p);
	if (ret < 0)
	{
		printf("tee_fs_unlink : no such file. %s", filename);
		return 0;
	}
	int fd = ret;

	debug("tee_fs_unlink ! %s fd:%d index:%d size:%d", filename, fd, p.index, p.size);

	/* decrease ref from usedflags */
	int num = p.size / RKSS_DATA_LEN + 1;
	ret = rkss_decref_multi_usedflags_sections(p.index, num);
	if (ret < 0)
	{
		printf("rkss_decref_multi_usedflags_sections error !");
		return -1;
	}

	/* rm from ptable */
	memset(&p, 0, sizeof(struct rkss_file_info));
	ret = rkss_write_back_ptable(fd, &p);
	if (ret < 0)
	{
		printf("tee_fs_unlink : write back error %d", ret);
		return -1;
	}

#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
#endif

	return 0;
}

static int tee_fs_link(struct tee_fs_rpc *fsrpc)
{
	char *filename = (char *)(fsrpc + 1);
	size_t offset_new_fn = strlen(filename) + 1;
	char *newfilename = filename + offset_new_fn;
	debug("tee_fs_link ! %s -> %s", filename, newfilename);

	struct rkss_file_info p_old = {0};
	int ret = rkss_get_fileinfo_by_name(filename, &p_old);
	if (ret < 0)
	{
		printf("cannot find src file %s.", filename);
		return -1;
	}

	struct rkss_file_info p_check = {0};
	ret = rkss_get_fileinfo_by_name(newfilename, &p_check);
	if (!ret)
	{
		printf("file exist ! %s.", newfilename);
		return -1;
	}

	struct rkss_file_info p_new = {0};
	memcpy(&p_new, &p_old, sizeof(struct rkss_file_info));
	strcpy(p_new.name, newfilename);
	ret = rkss_write_empty_ptable(&p_new);
	if (ret < 0)
	{
		printf("tee_fs_open : error. %s", filename);
		return -1;
	}

	int num = p_new.size / RKSS_DATA_LEN + 1;
	ret = rkss_incref_multi_usedflags_sections(p_new.index, num);
	if (ret < 0)
	{
		printf("rkss_incref_multi_usedflags_sections error !");
		return -1;
	}

#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
#endif

	return 0;
}

static int tee_fs_rename(struct tee_fs_rpc *fsrpc)
{
	char *filenames = (char *)(fsrpc + 1);
	char *newnames = filenames + strlen(filenames) + 1;
	debug("rename: %s -> %s", filenames, newnames);

	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_name(filenames, &p);
	if (ret < 0)
	{
		printf("filename no found .");
		return -1;
	}

	strcpy(p.name, newnames);

	ret = rkss_write_back_ptable(ret, &p);
	if (ret < 0)
	{
		printf("write ptable error!");
		return -1;
	}

	return 0;
}

static int tee_fs_truncate(struct tee_fs_rpc *fsrpc)
{
	debug("tee_fs_truncate: fd:%d, lenth:%d", fsrpc->fd, fsrpc->arg);
	if (fsrpc->fd < 0)
	{
		printf("tee_fs_truncate: fd unavilable !");
		return -1;
	}

	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_index(fsrpc->fd, &p);
	if (ret < 0)
	{
		printf("fd unvailable!");
		return -1;
	}

	p.size = fsrpc->arg;
	ret = rkss_write_back_ptable(fsrpc->fd, &p);
	if (ret < 0)
	{
		printf("tee_fs_write: write ptable error!");
		return -1;
	}
	return 0;
}

static int tee_fs_mkdir(struct tee_fs_rpc *fsrpc)
{
	char *dirname = (char *)(fsrpc + 1);
	UNREFERENCED_PARAMETER(dirname);
	debug("tee_fs_mkdir: %s",dirname);
	return 0;
}

static int tee_fs_opendir(struct tee_fs_rpc *fsrpc)
{
	char *dirname = (char *)(fsrpc + 1);
	dir_seek = 0;
	int ret = rkss_get_dirs_by_name(dirname);
	if (ret < 0)
	{
		printf("tee_fs_opendir: error");
	}
	debug("tee_fs_opendir: %s, seek/num:%d/%d", dirname, dir_seek, dir_num);
	return 0;
}

static int tee_fs_closedir(struct tee_fs_rpc *fsrpc)
{
	char *dirname = (char *)(fsrpc + 1);
	UNREFERENCED_PARAMETER(dirname);
	debug("tee_fs_closedir: %s", dirname);
	dir_seek = 0;
	dir_num = 0;
	return 0;
}

static int tee_fs_readdir(struct tee_fs_rpc *fsrpc)
{
	char *dirname = (char *)(fsrpc + 1);
	printf("seek/num:%d/%d",dir_seek, dir_num);
	if (dir_seek == dir_num)
	{
		dirname = NULL;
		fsrpc->len = 0;
		printf("tee_fs_readdir: END");
		return -1;
	}

	strcpy(dirname, dir_cache[dir_seek]);
	fsrpc->len = strlen(dir_cache[dir_seek]) + 1;
	++dir_seek;

	debug("tee_fs_readdir: %s", dirname);
	return 0;
}

static int tee_fs_rmdir(struct tee_fs_rpc *fsrpc)
{
	char *dirname = (char *)(fsrpc + 1);
	debug("tee_fs_rmdir: %s", dirname);

	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_name(dirname, &p);
	if (ret == -100)
	{
		printf("dir is not empty.");
		return -1;
	}
	else if (ret >= 0)
	{
		printf("%s is not a dir.", p.name);
		return -1;
	}
	debug("rmdir success.");
	return 0;
}

static int tee_fs_access(struct tee_fs_rpc *fsrpc)
{
	char *filename = (char *)(fsrpc + 1);
	debug("tee_fs_access: name:%s,flag:%x",filename,fsrpc->flags);

	struct rkss_file_info p = {0};
	int ret = rkss_get_fileinfo_by_name(filename, &p);
	if (ret < 0 && ret != -100)
	{
		debug("tee_fs_access: %s no such file or directory.", filename);
		return -1;
	}

	if (CHECKFLAG(fsrpc->flags, TEE_FS_R_OK))
	{
		if (!CHECKFLAG(p.flags, RK_FS_R))
		{
			printf("tee_fs_access: no permission FS_R_OK in %x.", p.flags);
			return -1;
		}
	}

	if (CHECKFLAG(fsrpc->flags, TEE_FS_W_OK))
	{
		if (!CHECKFLAG(p.flags, RK_FS_W))
		{
			printf("tee_fs_access: no permission FS_W_OK in %x.", p.flags);
			return -1;
		}
	}
	return 0;
}

int tee_supp_rk_fs_init(void)
{
	assert(sizeof(struct rkss_file_info) == 126);
	assert(512 / sizeof(struct rkss_file_info) == RKSS_EACH_FILEFOLDER_COUNT);

	__maybe_unused int i = 0;
	unsigned char *table_data;
#ifdef DEBUG_CLEAN_RKSS // clean secure storage
	for (i = 0; i < RKSS_DATA_SECTION_COUNT; i++)
	{
		struct rk_secure_storage rkss = {0};
		memset(rkss.data, 0, RKSS_DATA_LEN);
		rkss.index = i;
		rkss_write_section(&rkss);
		printf("cleaned [%d]", i);
	}
#endif

	// Verify Partition Table
	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return -1;
	}
	int ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		free(table_data);
		return -1;
	}

	/* Verify Partition Table*/
	ret = rkss_verify_ptable(table_data);
	if (ret < 0) {
		printf("rkss_verify_ptable fail ! ret: %d.", ret);
		free(table_data);
		return -1;
	}
	free(table_data);
	table_data = NULL;

	// Verify Usedflags Section
	struct rk_secure_storage rkss = {0};
	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_section(&rkss);
	if (ret < 0)
	{
		printf("rkss_read_section fail ! ret: %d.", ret);
		return -1;
	}
	ret = rkss_verify_usedflags(&rkss);
	if (ret < 0)
	{
		printf("rkss_verify_usedflags fail ! ret: %d.", ret);
		return -1;
	}

#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
	rkss_dump_usedflags();
#endif

	return 0;
}
void OpteeClientRkFsInit(void)
{
	debug(" OpteeClientRkFsInit\n");
	tee_supp_rk_fs_init();
}

static int rkss_step = 0;
int tee_supp_rk_fs_process(void *cmd, size_t cmd_size)
{
	struct tee_fs_rpc *fsrpc = cmd;
	int ret = -1;

	if (cmd_size < sizeof(struct tee_fs_rpc))
	{
		printf(">>>cmd_size < sizeof(struct tee_fs_rpc) !");
		return ret;
	}

	if (cmd == NULL)
	{
		printf(">>>cmd == NULL !");
		return ret;
	}

	switch (fsrpc->op) {
	case TEE_FS_OPEN:
		debug(">>>>>>> [%d] TEE_FS_OPEN !", rkss_step++);
		ret = tee_fs_open(fsrpc);
		break;
	case TEE_FS_CLOSE:
		debug(">>>>>>> [%d] TEE_FS_CLOSE !", rkss_step++);
		ret = tee_fs_close(fsrpc);
		break;
	case TEE_FS_READ:
		debug(">>>>>>> [%d] TEE_FS_READ !", rkss_step++);
		ret = tee_fs_read(fsrpc);
		break;
	case TEE_FS_WRITE:
		debug(">>>>>>> [%d] TEE_FS_WRITE !", rkss_step++);
		ret = tee_fs_write(fsrpc);
		break;
	case TEE_FS_SEEK:
		debug(">>>>>>> [%d] TEE_FS_SEEK !", rkss_step++);
		ret = tee_fs_seek(fsrpc);
		break;
	case TEE_FS_UNLINK:
		debug(">>>>>>> [%d] TEE_FS_UNLINK !", rkss_step++);
		ret = tee_fs_unlink(fsrpc);
		break;
	case TEE_FS_RENAME:
		debug(">>>>>>> [%d] TEE_FS_RENAME !", rkss_step++);
		ret = tee_fs_rename(fsrpc);
		break;
	case TEE_FS_TRUNC:
		debug(">>>>>>> [%d] TEE_FS_TRUNC !", rkss_step++);
		ret = tee_fs_truncate(fsrpc);
		break;
	case TEE_FS_MKDIR:
		debug(">>>>>>> [%d] TEE_FS_MKDIR !", rkss_step++);
		ret = tee_fs_mkdir(fsrpc);
		debug(">>>>>>> ret = [%d]  !", ret);
		break;
	case TEE_FS_OPENDIR:
		debug(">>>>>>> [%d] TEE_FS_OPENDIR !", rkss_step++);
		ret = tee_fs_opendir(fsrpc);
		break;
	case TEE_FS_CLOSEDIR:
		debug(">>>>>>> [%d] TEE_FS_CLOSEDIR !", rkss_step++);
		ret = tee_fs_closedir(fsrpc);
		break;
	case TEE_FS_READDIR:
		debug(">>>>>>> [%d] TEE_FS_READDIR !", rkss_step++);
		ret = tee_fs_readdir(fsrpc);
		break;
	case TEE_FS_RMDIR:
		debug(">>>>>>> [%d] TEE_FS_RMDIR !", rkss_step++);
		ret = tee_fs_rmdir(fsrpc);
		break;
	case TEE_FS_ACCESS:
		debug(">>>>>>> [%d] TEE_FS_ACCESS !", rkss_step++);
		ret = tee_fs_access(fsrpc);
		break;
	case TEE_FS_LINK:
		debug(">>>>>>> [%d] TEE_FS_LINK !", rkss_step++);
		ret = tee_fs_link(fsrpc);
		break;
	default:
		printf(">>>>> DEFAULT !! %d",fsrpc->op);
		break;
	}

	fsrpc->res = ret;
	debug(">>>>>>> fsrpc->res = [%d]	!", fsrpc->res);

	return ret;
}
