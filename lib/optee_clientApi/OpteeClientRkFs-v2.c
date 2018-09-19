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
 */
#include <common.h>
#include <stdlib.h>
#include <command.h>
#include <optee_include/OpteeClientRkFs-v2.h>
#include <optee_include/tee_client_api.h>

/*
 *#define DEBUG_RKFSS
 *#define DEBUG_CLEAN_RKSS
 */

/*
 * Operations and defines shared with TEE.
 */
#define OPTEE_MRF_OPEN			0
#define OPTEE_MRF_CREATE		1
#define OPTEE_MRF_CLOSE			2
#define OPTEE_MRF_READ			3
#define OPTEE_MRF_WRITE			4
#define OPTEE_MRF_TRUNCATE		5
#define OPTEE_MRF_REMOVE		6
#define OPTEE_MRF_RENAME		7
#define OPTEE_MRF_OPENDIR		8
#define OPTEE_MRF_CLOSEDIR		9
#define OPTEE_MRF_READDIR		10

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
#define RKSS_PARTITION_TABLE_COUNT	128		/* total size 512 * 128*/
#define RKSS_EACH_FILEFOLDER_COUNT	4		/* 504 / 126 = 4*/
#define RKSS_NAME_MAX_LENGTH		117		/* 116 char + "\0"*/
#define RKSS_USEDFLAGS_INDEX		RKSS_PARTITION_TABLE_COUNT

#define RK_FS_R    0x1
#define RK_FS_W    0x2
#define RK_FS_D    0x8

typedef struct rkss_file_info {
	uint8_t		used;
	char 		name[RKSS_NAME_MAX_LENGTH];
	uint16_t	index;	/* from 129 to 1023*/
	uint16_t	size;	/* size of data*/
	uint16_t	father;
	uint8_t 	id; /* file folder count index*/
	uint8_t		flags;
} rkss_file_info; /* 126 byte for each*/

#define RKSS_VERSION		((uint32_t)0x1)
#define RKSS_CHECK_STR	((uint32_t)0x12345678)
#define RKSS_CHECK_PT		((uint8_t)0xFC)
typedef struct rkss_file_verification {
	uint32_t version;
	uint32_t checkstr;
} rkss_file_verification;

typedef struct rk_secure_storage {
	unsigned long index;
	unsigned char data[RKSS_DATA_LEN];
} rk_secure_storage;

/* Path to all secure storage dev. */
#define RKSS_DEV_NAND "/dev/block/rknand_security"
#define RKSS_DEV_EMMC "/dev/block/by-name/security"

/* Function Defines */
#define UNREFERENCED_PARAMETER(P) (P = P)
#define CHECKFLAG(flags, flag) (flags & flag)
#define ADDFLAG(flags, flag) (flags | flag)

/* RK Secure Storage Calls */
static char dir_cache[RKSS_NAME_MAX_LENGTH][12];
static int dir_num;
static int dir_seek;

static struct blk_desc *dev_desc = NULL;
static disk_partition_t part_info;
static int rkss_read_multi_sections(unsigned char *data, unsigned long index, unsigned int num)
{
	unsigned long ret;

	if (dev_desc == NULL) {
		dev_desc = rockchip_get_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device\n", __func__);
			return TEEC_ERROR_GENERIC;
		}

		if (part_get_info_by_name(dev_desc, "security", &part_info) < 0) {
			printf("Could not find security partition\n");
			return TEEC_ERROR_GENERIC;
		}
	}
	ret = blk_dread(dev_desc, part_info.start + index, num, data);
	if (ret != num) {
		printf("blk_dread fail \n");
		return TEEC_ERROR_GENERIC;
	}
	return TEEC_SUCCESS;
}

static int rkss_write_multi_sections(unsigned char *data, unsigned long index, unsigned int num)
{
	unsigned long ret;

	if (dev_desc == NULL) {
		dev_desc = rockchip_get_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device\n", __func__);
			return TEEC_ERROR_GENERIC;
		}

		if (part_get_info_by_name(dev_desc, "security", &part_info) < 0) {
			printf("Could not find security partition\n");
			return TEEC_ERROR_GENERIC;
		}
	}
	ret = blk_dwrite(dev_desc, part_info.start + index, num, data);
	if (ret != num) {
		printf("blk_dwrite fail \n");
		return TEEC_ERROR_GENERIC;
	}
	return TEEC_SUCCESS;
}

static int rkss_read_patition_tables(unsigned char *data)
{
	unsigned long ret;

	if (dev_desc == NULL) {
		dev_desc = rockchip_get_bootdev();
		if (!dev_desc) {
			printf("%s: Could not find device\n", __func__);
			return TEEC_ERROR_GENERIC;
		}

		if (part_get_info_by_name(dev_desc, "security", &part_info) < 0) {
			printf("Could not find security partition\n");
			return TEEC_ERROR_GENERIC;
		}
	}
	ret = blk_dread(dev_desc, part_info.start, RKSS_PARTITION_TABLE_COUNT, data);
	if (ret != RKSS_PARTITION_TABLE_COUNT) {
		printf("blk_dread fail \n");
		return TEEC_ERROR_GENERIC;
	}
	return TEEC_SUCCESS;
}

#ifdef DEBUG_RKFSS
static void rkss_dump(void *data, unsigned int len)
{
	char *p = (char *)data;
	unsigned int i = 0;
	printf("-------------- DUMP %d --------------", len);
	for (i = 0; i < len; i++) {
		printf("%02x ", *(p + i));
	}
	printf("\n");
	printf("------------- DUMP END -------------");
}

static void rkss_dump_ptable(void)
{
	struct rk_secure_storage rkss = {0};
	int i, n, ret;
	void *pdata;
	struct rkss_file_info *p;
	unsigned char *table_data;

	table_data = malloc(RKSS_PARTITION_TABLE_COUNT * RKSS_DATA_LEN);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		return;
	}

	printf("-------------- DUMP ptable --------------");
	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);

		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;
			printf("[%02d][%c] %s , inx:%d, size:%d",
					i*RKSS_EACH_FILEFOLDER_COUNT+n, p->used == 0 ? 'F':'T', p->name,
					p->index, p->size);
		}
	}
	free(table_data);
	printf("-------------- DUMP END --------------");
}

static void rkss_dump_usedflags(void)
{
	struct rk_secure_storage rkss = {0};
	int ret;

	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
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
		return TEEC_ERROR_GENERIC;
	}
	debug("verify ptable success.");
	return TEEC_SUCCESS;
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
				return TEEC_ERROR_GENERIC;
			}

			return TEEC_SUCCESS;
		}
	}
	debug("rkss_verify_usedflags: sucess.");
	return TEEC_SUCCESS;
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
		return TEEC_ERROR_GENERIC;
	}

	pdata = rkss.data;
	p = (struct rkss_file_info *)pdata;
	p += n;

	if (p->used != 1) {
		debug("error: unused section! ");
		return TEEC_ERROR_GENERIC;
	}
	debug("rkss_get_fileinfo_by_index p->used = %d p->name=%s p->index=%d p->size=%d \n",
		p->used, p->name, p->index, p->size);
	memcpy(pfileinfo, p, sizeof(struct rkss_file_info));

	return TEEC_SUCCESS;
}

static int rkss_get_fileinfo_by_name(
		char *filename, struct rkss_file_info *pfileinfo)
{
	int i, ret;
	uint8_t n = 0;
	unsigned int len;
	unsigned char *table_data;
	struct rk_secure_storage rkss = {0};
	void *pdata;
	struct rkss_file_info *p;
	const char *split = "/";
	char *last_inpos;
	char *last_svpos;
	char *cur_inpos;
	char *cur_svpos;
	int size_in, size_sv;

	len = strlen(filename);
	if (len > RKSS_NAME_MAX_LENGTH - 1) {
		printf("filename is too long. length:%u", len);
		return TEEC_ERROR_GENERIC;
	}

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return TEEC_ERROR_GENERIC;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);

		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;

			if (p->used == 0)
				continue;

			/* Full Matching*/
			if (!strcmp(p->name, filename)) {
				debug("rkss_get_fileinfo_by_name: hit table[%d/%d], index[%d/%d] \n",
						i, RKSS_PARTITION_TABLE_COUNT, n, RKSS_EACH_FILEFOLDER_COUNT);
				memcpy(pfileinfo, p, sizeof(struct rkss_file_info));
				free(table_data);
				return i * RKSS_EACH_FILEFOLDER_COUNT + n;
			}

			/* Folder Matching*/
			last_inpos = filename;
			last_svpos = p->name;
			cur_inpos = NULL;
			cur_svpos = NULL;
			do {
				cur_inpos = strstr(last_inpos, split);
				cur_svpos = strstr(last_svpos, split);
				size_in = cur_inpos == NULL ?
						(int)strlen(last_inpos) : cur_inpos - last_inpos;
				size_sv = cur_svpos == NULL ?
						(int)strlen(last_svpos) : cur_svpos - last_svpos;

				ret = memcmp(last_inpos, last_svpos, size_in);
				last_inpos = cur_inpos + 1;
				last_svpos = cur_svpos + 1;

				if (size_in != size_sv || ret)
					goto UNMATCHFOLDER;

			} while (cur_inpos && cur_svpos);

			debug("Matched folder: %s \n", p->name);
			free(table_data);
			return TEEC_ERROR_GENERIC;
UNMATCHFOLDER:
			debug("Unmatched ... \n");
		}
	}
	debug("rkss_get_fileinfo_by_name: file or dir no found!");
	free(table_data);
	return TEEC_ERROR_GENERIC;
}

static int rkss_get_dirs_by_name(char *filename)
{
	int i, ret;
	uint8_t n = 0;
	unsigned int len;
	unsigned char *table_data;
	struct rk_secure_storage rkss = {0};
	void *pdata;
	struct rkss_file_info *p;
	char *chk, *file, *subdir;

	len = strlen(filename);
	if (len > RKSS_NAME_MAX_LENGTH - 1) {
		printf("filename is too long. length:%u", len);
		return TEEC_ERROR_GENERIC;
	}

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return TEEC_ERROR_GENERIC;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}

	dir_num = 0;
	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);

		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;

			if (p->used == 0)
				continue;

			/* Full Matching*/
			ret = memcmp(p->name, filename, strlen(filename));
			debug("comparing [fd:%d] : %s ?= %s , ret:%d \n",
					i*RKSS_EACH_FILEFOLDER_COUNT+n, p->name, filename, ret);
			if (!ret && strlen(p->name) > strlen(filename)) {
				chk = p->name + strlen(filename);
				if (*chk == '/') {
					file = p->name + strlen(filename) + 1;
					subdir = strtok(file, "/");
					debug("found: %s \n", subdir);
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
	int i, ret;
	int count0 = 0;
	uint8_t *flag;
	uint8_t value;

	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}

	for (i = 0; i < RKSS_DATA_SECTION_COUNT; i++) {
		flag = (uint8_t *)rkss.data + (int)i/2;
		value = i & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;

		if (value == 0x0) {
			if (++count0 == section_size) {
				return (i + 1 - section_size);
			}
		} else {
			count0 = 0;
		}
	}

	printf("Not enough space available in secure storage !");
	return TEEC_ERROR_GENERIC;
}

static int rkss_incref_multi_usedflags_sections(unsigned int index, unsigned int num)
{
	struct rk_secure_storage rkss = {0};
	int ret, value, i;
	uint8_t *flag;

	if ((index + num) >= RKSS_DATA_SECTION_COUNT) {
		printf("index[%d] out of range.", index);
		return TEEC_ERROR_GENERIC;
	}

	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
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
		return TEEC_ERROR_GENERIC;
	}
	return TEEC_SUCCESS;
}

static int rkss_decref_multi_usedflags_sections(unsigned int index, unsigned int num)
{
	struct rk_secure_storage rkss = {0};
	int ret, value, i;
	uint8_t *flag;

	if ((index + num) >= RKSS_DATA_SECTION_COUNT) {
		printf("index[%d] out of range.", index);
		return TEEC_ERROR_GENERIC;
	}

	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
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
		return TEEC_ERROR_GENERIC;
	}
	return TEEC_SUCCESS;
}

static int rkss_write_empty_ptable(struct rkss_file_info *pfileinfo)
{
	unsigned char *table_data;
	int ret, i, n;
	struct rk_secure_storage rkss = {0};
	void *pdata;
	struct rkss_file_info *p;

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return TEEC_ERROR_GENERIC;
	}

	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_LEN, RKSS_DATA_LEN);
		for (n = 0; n < RKSS_EACH_FILEFOLDER_COUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;
			if (p->used == 0) {
				debug("write ptable in [%d][%d] .", i, n);
				memcpy(p, pfileinfo, sizeof(struct rkss_file_info));
				p->used = 1;
				p->id = n;
				debug("write emt ptable : [%d,%d] name:%s, index:%d, ",
					i, n, p->name, p->index);
				debug("size:%d, used:%d \n",  p->size, p->used);
				ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
				if (ret < 0) {
					printf("rkss_write_multi_sections fail ! ret: %d.", ret);
					free(table_data);
					return TEEC_ERROR_GENERIC;
				}

				free(table_data);
				return i * RKSS_EACH_FILEFOLDER_COUNT + n;
			}
		}
	}
	debug("No enough ptable space available in secure storage.\n");
	free(table_data);
	return TEEC_ERROR_GENERIC;
}

static int rkss_write_back_ptable(int fd, struct rkss_file_info *pfileinfo)
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
		return TEEC_ERROR_GENERIC;
	}

	pdata = rkss.data;
	p = (struct rkss_file_info *)pdata;
	p += n;

	memcpy(p, pfileinfo, sizeof(struct rkss_file_info));
	debug("write ptable : [%d,%d] name:%s, index:%d, size:%d, used:%d \n",
			i, n, p->name, p->index, p->size, p->used);

	ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_write_multi_sections fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}
#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
#endif

	return TEEC_SUCCESS;
}

static uint32_t ree_fs_new_open(size_t num_params,
				   struct tee_ioctl_param *params)
{
	char *filename;
	int fd;
	struct rkss_file_info p = {0};

	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx \n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);

	filename = (char *)(size_t)params[1].u.memref.shm_id;
	if (!filename)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (strlen(filename) > RKSS_NAME_MAX_LENGTH) {
		printf("ree_fs_new_open: file name too long. %s", filename);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	debug("ree_fs_new_open open file: %s, len: %zu \n", filename, strlen(filename));
	fd = rkss_get_fileinfo_by_name(filename, &p);
	if (fd < 0) {
		debug("ree_fs_new_open : no such file. %s", filename);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	debug("ree_fs_new_open! %s, fd: %d \n", filename, fd);

	params[2].u.value.a = fd;
	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_create(size_t num_params,
				     struct tee_ioctl_param *params)
{
	char *filename;
	int fd;
	int ret, num;
	struct rkss_file_info p = {0};
	/* file open flags: O_RDWR | O_CREAT | O_TRUNC
	 * if file exists, we must remove it first.
	 */

	filename = (char *)(size_t)params[1].u.memref.shm_id;
	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx\n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);
	if (!filename)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (strlen(filename) > RKSS_NAME_MAX_LENGTH) {
		printf("ree_fs_new_create: file name too long. %s", filename);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	debug("ree_fs_new_create create file: %s, len: %zu \n", filename, strlen(filename));
	fd = rkss_get_fileinfo_by_name(filename, &p);
	if (fd >= 0) {
		debug("ree_fs_new_create : file exist, clear it. %s", filename);
		/* decrease ref from usedflags */
		num = p.size / RKSS_DATA_LEN + 1;
		ret = rkss_decref_multi_usedflags_sections(p.index, num);
		if (ret < 0) {
			printf("rkss_decref_multi_usedflags_sections error !");
			return TEEC_ERROR_GENERIC;
		}

		/* rm from ptable */
		memset(&p, 0, sizeof(struct rkss_file_info));
		ret = rkss_write_back_ptable(fd, &p);
		if (ret < 0) {
			printf("ree_fs_new_create : write back error %d", ret);
			return TEEC_ERROR_GENERIC;
		}
	}

	debug("ree_fs_new_create create new file: %s \n", filename);
	strcpy(p.name, filename);
	p.index = 0;
	p.size = 0;
	p.used = 1;
	p.flags = RK_FS_R | RK_FS_W;
	fd = rkss_write_empty_ptable(&p);
	if (fd < 0) {
		printf("ree_fs_new_create : write empty ptable error. %s", filename);
		return TEEC_ERROR_GENERIC;
	}

	debug("ree_fs_new_create ! %s, fd: %d. \n", filename, fd);

	params[2].u.value.a = fd;
	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_close(size_t num_params,
				    struct tee_ioctl_param *params)
{
	debug("ree_fs_new_close !");
	UNREFERENCED_PARAMETER(params);
	UNREFERENCED_PARAMETER(num_params);
	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_read(size_t num_params,
				   struct tee_ioctl_param *params)
{
	uint8_t *data;
	size_t len;
	off_t offs;
	int fd;
	int ret;
	struct rkss_file_info p = {0};
	int di, section_num;
	uint8_t *temp_file_data;

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;

	data = (uint8_t *)(size_t)params[1].u.memref.shm_id;
	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx\n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);

	if (!data)
		return TEEC_ERROR_BAD_PARAMETERS;
	len = params[1].u.memref.size;

	debug("ree_fs_new_read! fd:%d, len:%zu, offs:%ld \n", fd, len, offs);

	ret = rkss_get_fileinfo_by_index(fd, &p);
	if (ret < 0) {
		debug("unavailable fd: %d!", fd);
		return TEEC_ERROR_GENERIC;
	}

	if (offs >= p.size)
		return TEEC_ERROR_BAD_PARAMETERS;

	section_num = p.size / RKSS_DATA_LEN + 1;
	temp_file_data = malloc(section_num * RKSS_DATA_LEN);
	ret = rkss_read_multi_sections(temp_file_data, p.index, section_num);
	if (ret < 0) {
		printf("unavailable file index!");
		free(temp_file_data);
		return TEEC_ERROR_GENERIC;
	}
	di = (offs + len) > p.size ? (p.size - offs) : len;
	memcpy(data, temp_file_data + offs, di);
	free(temp_file_data);
	temp_file_data = 0;
	params[1].u.memref.size = di;

	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_write(size_t num_params,
				    struct tee_ioctl_param *params)
{
	uint8_t *data;
	size_t len;
	off_t offs;
	struct rkss_file_info p = {0};
	int ret, fd, new_size;
	int section_num;
	uint8_t *file_data=0, *temp_file_data=0;

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;

	data = (uint8_t *)(size_t)params[1].u.memref.shm_id;
	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx\n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);
	if (!data)
		return TEEC_ERROR_BAD_PARAMETERS;
	len = params[1].u.memref.size;

	debug("ree_fs_new_write ! fd:%d, len:%zu, offs:%ld \n", fd, len, offs);

	ret = rkss_get_fileinfo_by_index(fd, &p);
	if (ret < 0) {
		printf("ree_fs_new_write: fd:%d unvailable!", fd);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	new_size = offs + len > p.size ? offs + len : p.size;
	file_data = malloc(new_size);
	if (!file_data)
		return TEEC_ERROR_OUT_OF_MEMORY;

	if (p.size != 0) {
		/* Read old file data out */
		section_num = p.size / RKSS_DATA_LEN + 1;
		temp_file_data = malloc(section_num * RKSS_DATA_LEN);
		ret = rkss_read_multi_sections(temp_file_data, p.index, section_num);
		if (ret < 0) {
			printf("unavailable file index %d section_num %d", p.index, section_num);
			ret = TEEC_ERROR_GENERIC;
			goto out;
		}
		memcpy(file_data, temp_file_data, p.size);
		free(temp_file_data);
		temp_file_data = 0;
		ret = rkss_decref_multi_usedflags_sections(p.index, section_num);
		if (ret < 0) {
			printf("rkss_decref_multi_usedflags_sections error !");
			ret = TEEC_ERROR_GENERIC;
			goto out;
		}
	}

	/* update new file info */
	memcpy(file_data + offs, data, len);
	p.size = new_size;
	section_num = new_size / RKSS_DATA_LEN + 1;
	p.index = rkss_get_empty_section_from_usedflags(section_num);
	debug("Get Empty section in %d \n", p.index);
	p.used = 1;
	ret = rkss_incref_multi_usedflags_sections(p.index, section_num);
	if (ret < 0) {
		printf("rkss_incref_multi_usedflags_sections error !");
		ret = TEEC_ERROR_GENERIC;
		goto out;
	}

	ret = rkss_write_back_ptable(fd, &p);
	if (ret < 0) {
		printf("ree_fs_new_write: write ptable error!");
		ret = TEEC_ERROR_GENERIC;
		goto out;
	}

	/* write new file data */
	temp_file_data = malloc(section_num * RKSS_DATA_LEN);
	memset(temp_file_data, 0, section_num * RKSS_DATA_LEN);
	memcpy(temp_file_data, file_data, p.size);
	rkss_write_multi_sections(temp_file_data, p.index, section_num);
	free(temp_file_data);
	temp_file_data = 0;

#ifdef DEBUG_RKFSS
	rkss_dump_usedflags();
#endif

out:
	if (file_data)
		free(file_data);
	if (temp_file_data) {
		free(temp_file_data);
		temp_file_data = 0;
	}

	return TEEC_SUCCESS;
}

/* TODO: update file data space */
static TEEC_Result ree_fs_new_truncate(size_t num_params,
				       struct tee_ioctl_param *params)
{
	size_t len;
	int fd, ret;
	struct rkss_file_info p = {0};
	unsigned int section_num_old, section_num_new;

	fd = params[0].u.value.b;
	len = params[0].u.value.c;

	debug("ree_fs_new_truncate: fd:%d, lenth:%zu \n", fd, len);

	ret = rkss_get_fileinfo_by_index(fd, &p);
	if (ret < 0) {
		printf("fd:%d unvailable!", fd);
		return TEEC_ERROR_GENERIC;
	}
	if (len > p.size) {
		printf("truncate error!");
		return TEEC_ERROR_GENERIC;
	}
	section_num_old = p.size / RKSS_DATA_LEN + 1;
	section_num_new = len / RKSS_DATA_LEN + 1;
	ret = rkss_decref_multi_usedflags_sections(p.index + section_num_new, section_num_old - section_num_new);
	if (ret < 0) {
		printf("rkss_decref_multi_usedflags_sections error !");
		ret = TEEC_ERROR_GENERIC;
	}
	p.size = len;
	ret = rkss_write_back_ptable(fd, &p);
	if (ret < 0) {
		printf("ree_fs_new_truncate: write ptable error!");
		return TEEC_ERROR_GENERIC;
	}

	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_remove(size_t num_params,
				     struct tee_ioctl_param *params)
{
	char *filename;
	struct rkss_file_info p = {0};
	int ret, fd, num;

	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx \n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);

	filename = (char *)(size_t)params[1].u.memref.shm_id;
	if (!filename)
		return TEEC_ERROR_BAD_PARAMETERS;

	ret = rkss_get_fileinfo_by_name(filename, &p);
	if (ret < 0) {
		printf("ree_fs_new_remove: no such file. %s", filename);
		return 0;
	}
	fd = ret;

	debug("ree_fs_new_remove! %s fd:%d index:%d size:%d \n", filename, fd, p.index, p.size);

	/* decrease ref from usedflags */
	num = p.size / RKSS_DATA_LEN + 1;
	ret = rkss_decref_multi_usedflags_sections(p.index, num);
	if (ret < 0) {
		printf("rkss_decref_multi_usedflags_sections error !");
		return TEEC_ERROR_GENERIC;
	}

	/* rm from ptable */
	memset(&p, 0, sizeof(struct rkss_file_info));
	ret = rkss_write_back_ptable(fd, &p);
	if (ret < 0) {
		printf("ree_fs_new_remove: write back error %d", ret);
		return TEEC_ERROR_GENERIC;
	}

#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
	rkss_dump_usedflags();
#endif

	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_rename(size_t num_params,
				     struct tee_ioctl_param *params)
{
	char *old_fname;
	char *new_fname;
	struct rkss_file_info p = {0};
	int ret;

	old_fname = (char *)(size_t)params[1].u.memref.shm_id;
	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx\n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);
	if (!old_fname)
		return TEEC_ERROR_BAD_PARAMETERS;

	new_fname = (char *)(size_t)params[2].u.memref.shm_id;
	debug("params[2].u.memref.shm_id = 0x%llx params[2].u.memref.shm_offs = 0x%llx\n",
		params[2].u.memref.shm_id, params[2].u.memref.shm_offs);
	if (!new_fname)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (strlen(new_fname) > RKSS_NAME_MAX_LENGTH) {
		printf("new file name too long. %s", new_fname);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	debug("rename: %s -> %s \n", old_fname, new_fname);

	ret = rkss_get_fileinfo_by_name(old_fname, &p);
	if (ret < 0) {
		printf("filename: %s no found.", old_fname);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	strcpy(p.name, new_fname);

	ret = rkss_write_back_ptable(ret, &p);
	if (ret < 0) {
		printf("write ptable error!");
		return TEEC_ERROR_GENERIC;
	}

	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_opendir(size_t num_params,
				      struct tee_ioctl_param *params)
{
	char *dirname;
	int ret;

	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx \n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);

	dirname = (char *)(size_t)params[1].u.memref.shm_id;
	if (!dirname)
		return TEEC_ERROR_BAD_PARAMETERS;

	dir_seek = 0;
	ret = rkss_get_dirs_by_name(dirname);
	if (ret < 0) {
		printf("ree_fs_new_opendir: error");
		return TEEC_ERROR_GENERIC;
	}

	debug("ree_fs_new_opendir: %s, seek/num:%d/%d \n", dirname, dir_seek, dir_num);
	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_closedir(size_t num_params,
				       struct tee_ioctl_param *params)
{
	if (num_params != 1 ||
	    (params[0].attr & TEE_IOCTL_PARAM_ATTR_TYPE_MASK) !=
			TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT)
		return TEEC_ERROR_BAD_PARAMETERS;

	dir_seek = 0;
	dir_num = 0;

	return TEEC_SUCCESS;
}

static TEEC_Result ree_fs_new_readdir(size_t num_params,
				      struct tee_ioctl_param *params)
{
	char *dirname;
	size_t len;
	size_t dirname_len;

	dirname = (char *)(size_t)params[1].u.memref.shm_id;
	debug("params[1].u.memref.shm_id = 0x%llx params[1].u.memref.shm_offs = 0x%llx \n",
		params[1].u.memref.shm_id, params[1].u.memref.shm_offs);
	if (!dirname)
		return TEEC_ERROR_BAD_PARAMETERS;
	len = params[1].u.memref.size;

	debug("seek/num:%d/%d \n", dir_seek, dir_num);
	if (dir_seek == dir_num) {
		params[1].u.memref.size = 0;
		debug("ree_fs_new_readdir: END");
		return TEEC_ERROR_BAD_STATE;
	}

	dirname_len = strlen(dir_cache[dir_seek]) + 1;
	params[1].u.memref.size = dirname_len;
	if (dirname_len > len)
		return TEEC_ERROR_SHORT_BUFFER;

	strcpy(dirname, dir_cache[dir_seek]);
	++dir_seek;

	debug("ree_fs_new_readdir: %s \n", dirname);

	return TEEC_SUCCESS;
}

int tee_supp_rk_fs_init(void)
{
	assert(sizeof(struct rkss_file_info) == 126);
	assert(512 / sizeof(struct rkss_file_info) == RKSS_EACH_FILEFOLDER_COUNT);

	int ret;
	struct rk_secure_storage rkss = {0};
	unsigned char *table_data;

	/* clean secure storage*/
#ifdef DEBUG_CLEAN_RKSS
	int i = 0;
	for (i = 0; i < RKSS_DATA_SECTION_COUNT; i++) {
		memset(rkss.data, 0, RKSS_DATA_LEN);
		rkss.index = i;
		rkss_write_multi_sections(rkss.data, rkss.index, 1);
		printf("cleaned [%d]", i);
	}
#endif

	table_data = malloc(RKSS_DATA_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("malloc table_data fail \n");
		return TEEC_ERROR_GENERIC;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("rkss_read_patition_tables fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}

	/* Verify Partition Table*/
	rkss_verify_ptable(table_data);
	free(table_data);
	table_data = NULL;

	/* Verify Usedflags Section*/
	rkss.index = RKSS_USEDFLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("rkss_read_multi_sections fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}
	ret = rkss_verify_usedflags(&rkss);
	if (ret < 0) {
		printf("rkss_verify_usedflags fail ! ret: %d.", ret);
		return TEEC_ERROR_GENERIC;
	}

#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
	rkss_dump_usedflags();
#endif
	return TEEC_SUCCESS;
}

void OpteeClientRkFsInit(void)
{
	debug(" OpteeClientRkFsInit\n");
	tee_supp_rk_fs_init();
}
bool tee_supp_param_is_value(struct tee_ioctl_param *param)
{
	switch (param->attr & TEE_IOCTL_PARAM_ATTR_TYPE_MASK) {
	case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT:
	case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
	case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
		return true;
	default:
		return false;
	}
}

static int rkss_step;
int tee_supp_rk_fs_process(size_t num_params,
			struct tee_ioctl_param *params)
{
	if (!num_params || !tee_supp_param_is_value(params))
		return TEEC_ERROR_BAD_PARAMETERS;

	switch (params->u.value.a) {
	case OPTEE_MRF_OPEN:
		debug(">>>>>>> [%d] OPTEE_MRF_OPEN! \n", rkss_step++);
		return ree_fs_new_open(num_params, params);
	case OPTEE_MRF_CREATE:
		debug(">>>>>>> [%d] OPTEE_MRF_CREATE! \n", rkss_step++);
		return ree_fs_new_create(num_params, params);
	case OPTEE_MRF_CLOSE:
		debug(">>>>>>> [%d] OPTEE_MRF_CLOSE! \n", rkss_step++);
		return ree_fs_new_close(num_params, params);
	case OPTEE_MRF_READ:
		debug(">>>>>>> [%d] OPTEE_MRF_READ! \n", rkss_step++);
		return ree_fs_new_read(num_params, params);
	case OPTEE_MRF_WRITE:
		debug(">>>>>>> [%d] OPTEE_MRF_WRITE! \n", rkss_step++);
		return ree_fs_new_write(num_params, params);
	case OPTEE_MRF_TRUNCATE:
		debug(">>>>>>> [%d] OPTEE_MRF_TRUNCATE! \n", rkss_step++);
		return ree_fs_new_truncate(num_params, params);
	case OPTEE_MRF_REMOVE:
		debug(">>>>>>> [%d] OPTEE_MRF_REMOVE! \n", rkss_step++);
		return ree_fs_new_remove(num_params, params);
	case OPTEE_MRF_RENAME:
		debug(">>>>>>> [%d] OPTEE_MRF_RENAME! \n", rkss_step++);
		return ree_fs_new_rename(num_params, params);
	case OPTEE_MRF_OPENDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_OPENDIR! \n", rkss_step++);
		return ree_fs_new_opendir(num_params, params);
	case OPTEE_MRF_CLOSEDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_CLOSEDIR! \n", rkss_step++);
		return ree_fs_new_closedir(num_params, params);
	case OPTEE_MRF_READDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_READDIR! \n", rkss_step++);
		return ree_fs_new_readdir(num_params, params);
	default:
		return TEEC_ERROR_BAD_PARAMETERS;
	}
}
