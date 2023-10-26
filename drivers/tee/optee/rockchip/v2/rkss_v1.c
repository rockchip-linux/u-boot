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
 *#define DEBUG_RKFSS
 *#define DEBUG_CLEAN_RKSS
 */

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
 *	------ RKSS Backup Structure --------
 *	- 512 byte backup header  [1024]
 *		- 1 * rkss_backup_verification + 31 * rkss_backup_info
 *	- 255 * 512 byte backup data [1025 - 1279]
 *
 */
#define RKSS_DATA_SECTION_COUNT		1024
#define RKSS_DATA_SECTION_LEN		512
#define RKSS_PARTITION_TABLE_COUNT	128		/* total size 512 * 128*/
#define RKSS_EACH_SECTION_FILECOUNT	4		/* 504 / 126 = 4*/
#define RKSS_MAX_NAME_LENGTH		117		/* 116 char + "\0"*/
#define RKSS_USED_FLAGS_INDEX		RKSS_PARTITION_TABLE_COUNT
#define RKSS_BACKUP_INDEX		RKSS_DATA_SECTION_COUNT
#define RKSS_BACKUP_COUNT		256

#define RKSS_BACKUP_VERSION		(unsigned int)0x1
#define RKSS_BACKUP_ENABLE		(unsigned int)0x55667788
#define RKSS_BACKUP_USEDFLAG		(unsigned int)0xAABBCCDD

struct rkss_backup_verification {
	unsigned int backup_version;
	unsigned int backup_count;
	unsigned int reserve;
	unsigned int backup_enable;
};

struct rkss_backup_info {
	unsigned int backup_index;
	unsigned int backup_num;
	unsigned int backup_data_index;
	unsigned int backup_usedflag;
};

typedef struct rkss_file_info {
	uint8_t		used;
	char 		name[RKSS_MAX_NAME_LENGTH];
	uint16_t	index;	/* from 129 to 1023*/
	uint16_t	size;	/* size of data*/
	uint16_t	father;
	uint8_t 	id; /* file folder count index*/
	uint8_t		flags;
} rkss_file_info; /* 126 byte for each*/

#define RKSS_CHECK_STR	((uint32_t)0x12345678)
#define RKSS_CHECK_PT		((uint8_t)0xFC)
typedef struct rkss_file_verification {
	uint32_t version;
	uint32_t checkstr;
} rkss_file_verification;

typedef struct rk_secure_storage {
	unsigned long index;
	unsigned char data[RKSS_DATA_SECTION_LEN];
} rk_secure_storage;

/* RK Secure Storage Calls */
static char dir_cache[RKSS_MAX_NAME_LENGTH][12];
static int dir_num;
static int dir_seek;

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

/*
 * action1:
 * rkss_begin_commit	set enable flag
 * rkss_backup_sections	backup data
 * blk_dwrite
 * rkss_finish_commit	clear enable flag, clear backup data
 * reboot
 * rkss_resume	not find enable flag, do nothing
 *
 * action2:
 * rkss_begin_commit	set enable flag
 * rkss_backup_sections	backup data
 * power off when blk_dwrite
 *
 * power on
 * rkss_resume	find enable flag, resume all backup data
 */
static int rkss_begin_commit(void)
{
	unsigned char *data = NULL;
	struct rkss_backup_verification p;
	unsigned long ret;

	if (check_security_exist(1) < 0)
		return -1;

	p.backup_version = RKSS_BACKUP_VERSION;
	p.backup_enable = RKSS_BACKUP_ENABLE;
	p.backup_count = 0;

	data = memalign(CONFIG_SYS_CACHELINE_SIZE, RKSS_DATA_SECTION_LEN);
	if (!data) {
		printf("%s: malloc data failed!\n", __func__);
		return -1;
	}
	memset(data, 0, RKSS_DATA_SECTION_LEN);
	memcpy(data, &p, sizeof(p));

	ret = blk_dwrite(dev_desc, part_info.start + RKSS_BACKUP_INDEX, 1, data);
	free(data);
	if (ret != 1) {
		printf("%s: blk_dwrite failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int rkss_finish_commit(void)
{
	unsigned char *data = NULL;
	unsigned long ret;

	if (check_security_exist(1) < 0)
		return -1;

	data = memalign(CONFIG_SYS_CACHELINE_SIZE, RKSS_DATA_SECTION_LEN);
	if (!data) {
		printf("%s: malloc data failed!\n", __func__);
		return -1;
	}
	memset(data, 0, RKSS_DATA_SECTION_LEN);

	ret = blk_dwrite(dev_desc, part_info.start + RKSS_BACKUP_INDEX, 1, data);
	free(data);
	if (ret != 1) {
		printf("%s: blk_dwrite failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int rkss_backup_sections(unsigned long index, unsigned int num)
{
	unsigned char *data = NULL;
	unsigned char *backup_data = NULL;
	struct rkss_backup_verification p;
	struct rkss_backup_info info_last, info_current;
	unsigned long ret;

	if (check_security_exist(1) < 0)
		return -1;

	data = memalign(CONFIG_SYS_CACHELINE_SIZE, RKSS_DATA_SECTION_LEN);
	if (!data) {
		printf("%s: malloc data failed!\n", __func__);
		goto error;
	}
	ret = blk_dread(dev_desc, part_info.start + RKSS_BACKUP_INDEX, 1, data);
	if (ret != 1) {
		printf("%s: blk_dread failed!\n", __func__);
		goto error;
	}

	memcpy(&p, data, sizeof(p));
	if (p.backup_version == RKSS_BACKUP_VERSION &&
	    p.backup_enable == RKSS_BACKUP_ENABLE) {
		if (p.backup_count == 0) {
			info_current.backup_usedflag = RKSS_BACKUP_USEDFLAG;
			info_current.backup_index = index;
			info_current.backup_num = num;
			info_current.backup_data_index = RKSS_BACKUP_INDEX + 1;
		} else {
			memcpy(&info_last, data + sizeof(p) +
			       (p.backup_count - 1) * sizeof(info_last),
			       sizeof(info_last));
			info_current.backup_usedflag = RKSS_BACKUP_USEDFLAG;
			info_current.backup_index = index;
			info_current.backup_num = num;
			info_current.backup_data_index =
				info_last.backup_data_index +
				info_last.backup_num;
		}
		if ((info_current.backup_data_index + info_current.backup_num) >
			(RKSS_BACKUP_INDEX + RKSS_BACKUP_COUNT)) {
			printf("%s: Not enough backup sections!\n", __func__);
			goto error;
		}
		debug("%s: index=0x%lx num=0x%x backup_data_index=0x%x\n",
		      __func__, index, num, info_current.backup_data_index);

		backup_data = memalign(CONFIG_SYS_CACHELINE_SIZE, num * RKSS_DATA_SECTION_LEN);
		if (!backup_data) {
			printf("%s: malloc backup_data failed!\n", __func__);
			goto error;
		}

		ret = blk_dread(dev_desc, part_info.start + index, num, backup_data);
		if (ret != num) {
			printf("%s: blk_dread failed!\n", __func__);
			goto error;
		}

		ret = blk_dwrite(dev_desc, part_info.start + info_current.backup_data_index,
				 num, backup_data);
		if (ret != num) {
			printf("%s: blk_dwrite failed!\n", __func__);
			goto error;
		}
		free(backup_data);

		p.backup_count += 1;

		memcpy(data, &p, sizeof(p));
		memcpy(data + sizeof(p) +
		       (p.backup_count - 1) * sizeof(info_current),
		       &info_current, sizeof(info_current));

		ret = blk_dwrite(dev_desc, part_info.start + RKSS_BACKUP_INDEX, 1, data);
		if (ret != 1) {
			printf("%s: blk_dwrite failed!\n", __func__);
			goto error;
		}
	}
	free(data);
	return 0;

error:
	if (data)
		free(data);
	if (backup_data)
		free(backup_data);
	return -1;
}

static int rkss_resume(void)
{
	unsigned char *data = NULL;
	unsigned char *backup_data = NULL;
	struct rkss_backup_verification p;
	struct rkss_backup_info info_current;
	unsigned int i;
	unsigned long ret;

	if (check_security_exist(1) < 0)
		return -1;

	data = memalign(CONFIG_SYS_CACHELINE_SIZE, RKSS_DATA_SECTION_LEN);
	if (!data) {
		printf("%s: malloc data failed!\n", __func__);
		goto error;
	}
	ret = blk_dread(dev_desc, part_info.start + RKSS_BACKUP_INDEX, 1, data);
	if (ret != 1) {
		printf("%s: blk_dread failed!\n", __func__);
		goto error;
	}

	memcpy(&p, data, sizeof(p));
	if (p.backup_version == RKSS_BACKUP_VERSION &&
	    p.backup_enable == RKSS_BACKUP_ENABLE) {
		for (i = p.backup_count; i > 0; i--) {
			memcpy(&info_current, data + sizeof(p) + (i - 1) *
			       sizeof(info_current), sizeof(info_current));

			if (info_current.backup_usedflag ==
			    RKSS_BACKUP_USEDFLAG) {
				debug("%s: rkss_resume backup_index=0x%x \
				      backup_num=0x%x \
				      info_current.backup_data_index=0x%x\n",
				      __func__,
				      info_current.backup_index,
				      info_current.backup_num,
				      info_current.backup_data_index);
				if ((info_current.backup_data_index +
				    info_current.backup_num) >
				    (RKSS_BACKUP_INDEX + RKSS_BACKUP_COUNT)) {
					printf("%s: backup sections error!\n", __func__);
					goto error;
				}
				if ((info_current.backup_index +
				    info_current.backup_num) >
				    RKSS_DATA_SECTION_COUNT) {
					printf("%s: original sections error!\n", __func__);
					goto error;
				}
				backup_data = memalign(CONFIG_SYS_CACHELINE_SIZE,
						       info_current.backup_num * RKSS_DATA_SECTION_LEN);
				if (!backup_data) {
					printf("%s: malloc backup_data failed!\n", __func__);
					goto error;
				}

				ret = blk_dread(dev_desc,
						part_info.start + info_current.backup_data_index,
						info_current.backup_num,
						backup_data);
				if (ret != info_current.backup_num) {
					printf("%s: blk_dread failed!\n", __func__);
					goto error;
				}

				ret = blk_dwrite(dev_desc,
						 part_info.start + info_current.backup_index,
						 info_current.backup_num,
						 backup_data);
				if (ret != info_current.backup_num) {
					printf("%s: blk_dwrite failed!\n", __func__);
					goto error;
				}
				free(backup_data);
			}
		}
	}
	memset(data, 0, RKSS_DATA_SECTION_LEN);
	ret = blk_dwrite(dev_desc, part_info.start + RKSS_BACKUP_INDEX, 1, data);
	if (ret != 1) {
		printf("%s: blk_dwrite failed\n", __func__);
		goto error;
	}
	free(data);
	return 0;

error:
	if (data)
		free(data);
	if (backup_data)
		free(backup_data);
	return -1;
}

static int rkss_read_multi_sections(unsigned char *data, unsigned long index, unsigned int num)
{
	unsigned char *tmp_data = NULL;
	unsigned long ret;

	if (check_security_exist(1) < 0)
		return -1;

	tmp_data = memalign(CONFIG_SYS_CACHELINE_SIZE, num * RKSS_DATA_SECTION_LEN);
	if (!tmp_data) {
		printf("%s: malloc tmp_data failed!\n", __func__);
		return -1;
	}
	ret = blk_dread(dev_desc, part_info.start + index, num, tmp_data);
	if (ret != num) {
		printf("%s: blk_dread failed!\n", __func__);
		free(tmp_data);
		return -1;
	}
	memcpy(data, tmp_data, num * RKSS_DATA_SECTION_LEN);
	free(tmp_data);
	return 0;
}

static int rkss_write_multi_sections(unsigned char *data, unsigned long index, unsigned int num)
{
	unsigned char *tmp_data = NULL;
	unsigned long ret;
	int result;

	result = rkss_backup_sections(index, num);
	if (result < 0) {
		printf("%s: rkss_backup_sections failed!\n", __func__);
		return -1;
	}

	if (check_security_exist(1) < 0)
		return -1;

	tmp_data = memalign(CONFIG_SYS_CACHELINE_SIZE, num * RKSS_DATA_SECTION_LEN);
	if (!tmp_data) {
		printf("%s: malloc tmp_data failed!\n", __func__);
		return -1;
	}
	memcpy(tmp_data, data, num * RKSS_DATA_SECTION_LEN);
	ret = blk_dwrite(dev_desc, part_info.start + index, num, tmp_data);
	free(tmp_data);
	if (ret != num) {
		printf("%s: blk_dwrite failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int rkss_read_patition_tables(unsigned char *data)
{
	unsigned long ret;
	unsigned char *table_data = NULL;

	if (check_security_exist(1) < 0)
		return -1;

	table_data = memalign(CONFIG_SYS_CACHELINE_SIZE,
			      RKSS_DATA_SECTION_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (!table_data) {
		printf("%s: malloc table_data failed!\n", __func__);
		return -1;
	}
	ret = blk_dread(dev_desc, part_info.start, RKSS_PARTITION_TABLE_COUNT, table_data);
	if (ret != RKSS_PARTITION_TABLE_COUNT) {
		printf("%s: blk_dread failed!\n", __func__);
		free(table_data);
		return -1;
	}
	memcpy(data, table_data, RKSS_DATA_SECTION_LEN * RKSS_PARTITION_TABLE_COUNT);
	free(table_data);
	return 0;
}

#ifdef DEBUG_RKFSS
static void rkss_dump(void *data, unsigned int len)
{
	char *p = (char *)data;
	unsigned int i = 0;
	printf("-------------- DUMP %d --------------\n", len);
	for (i = 0; i < len; i++) {
		printf("%02x ", *(p + i));
	}
	printf("\n");
	printf("------------- DUMP END -------------\n");
}

static void rkss_dump_ptable(void)
{
	struct rk_secure_storage rkss = {0};
	int i, n, ret;
	void *pdata;
	struct rkss_file_info *p;
	unsigned char *table_data;

	table_data = malloc(RKSS_PARTITION_TABLE_COUNT * RKSS_DATA_SECTION_LEN);
	if (table_data == NULL) {
		printf("%s: malloc table_data failed!\n", __func__);
		return;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("%s: rkss_read_patition_tables failed!\n", __func__);
		return;
	}

	printf("-------------- DUMP ptable --------------\n");
	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_SECTION_LEN, RKSS_DATA_SECTION_LEN);

		for (n = 0; n < RKSS_EACH_SECTION_FILECOUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;
			printf("[%02d][%c] %s , inx:%d, size:%d",
				i*RKSS_EACH_SECTION_FILECOUNT+n, p->used == 0 ? 'F':'T', p->name,
				p->index, p->size);
		}
	}
	free(table_data);
	printf("-------------- DUMP END --------------\n");
}

static void rkss_dump_usedflags(void)
{
	struct rk_secure_storage rkss = {0};
	int ret;

	rkss.index = RKSS_USED_FLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_read_multi_sections failed!\n", __func__);
		return;
	}
	rkss_dump(rkss.data, RKSS_DATA_SECTION_LEN);
}
#endif

static int rkss_verify_ptable(unsigned char *table_data)
{
	unsigned char *cp, *vp;
	struct rkss_file_verification *verify;
	int ret, i, write_table_flag = 0;

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		cp = table_data + (i * RKSS_DATA_SECTION_LEN);
		vp = cp + RKSS_DATA_SECTION_LEN - sizeof(struct rkss_file_verification);
		verify = (struct rkss_file_verification *)(void *)vp;

		if (verify->version != RKSS_VERSION_V1 ||
		    verify->checkstr != RKSS_CHECK_STR) {
			printf("verify [%d] fail, cleanning ....", i);
			memset(cp, 0, RKSS_DATA_SECTION_LEN);
			verify->checkstr = RKSS_CHECK_STR;
			verify->version = RKSS_VERSION_V1;
			write_table_flag = 1;
		}
	}
	if (write_table_flag == 1) {
		ret = rkss_write_multi_sections(table_data, 0, RKSS_PARTITION_TABLE_COUNT);
		if (ret < 0) {
			printf("%s: rkss_write_multi_sections failed!\n", __func__);
			return TEE_ERROR_GENERIC;
		}
	}
	debug("%s: verify ptable success.\n", __func__);
	return TEE_SUCCESS;
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
			debug("%s: init usedflags section ...\n", __func__);
			memset(rkss->data, 0x00, RKSS_DATA_SECTION_LEN);
			for (n = 0; n < RKSS_PARTITION_TABLE_COUNT + 1; n++) {
				flagw = (uint8_t *)rkss->data + (int)n/2;
				value = 0x1;
				*flagw = n & 0x1 ? (*flagw & 0xF0) | (value & 0x0F) :
					 (*flagw & 0x0F) | (value << 4);
			}
			ret = rkss_write_multi_sections(rkss->data, rkss->index, 1);
			if (ret < 0) {
				printf("%s: clean usedflags section failed!\n", __func__);
				return TEE_ERROR_GENERIC;
			}

			return TEE_SUCCESS;
		}
	}
	debug("%s: rkss_verify_usedflags: sucess.\n", __func__);
	return TEE_SUCCESS;
}

static int rkss_get_fileinfo_by_index(int fd, struct rkss_file_info *pfileinfo)
{
	int i = fd / RKSS_EACH_SECTION_FILECOUNT;
	int n = fd - (RKSS_EACH_SECTION_FILECOUNT * i);
	struct rk_secure_storage rkss = {0};
	int ret;
	void *pdata;
	struct rkss_file_info *p;

	rkss.index = i;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_read_multi_sections failed! \n", __func__);
		return TEE_ERROR_GENERIC;
	}

	pdata = rkss.data;
	p = (struct rkss_file_info *)pdata;
	p += n;

	if (p->used != 1) {
		debug("%s: error: unused section!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	debug("%s: p->used = %d p->name=%s p->index=%d p->size=%d\n",
		__func__, p->used, p->name, p->index, p->size);
	memcpy(pfileinfo, p, sizeof(struct rkss_file_info));

	return TEE_SUCCESS;
}

static int rkss_get_fileinfo_by_name(char *filename,
				     struct rkss_file_info *pfileinfo)
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
	if (len > RKSS_MAX_NAME_LENGTH - 1) {
		printf("%s: filename is too long. length:%u\n", __func__, len);
		return TEE_ERROR_GENERIC;
	}

	table_data = malloc(RKSS_DATA_SECTION_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("%s: malloc table_data failed\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("%s: rkss_read_patition_tables failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_SECTION_LEN, RKSS_DATA_SECTION_LEN);

		for (n = 0; n < RKSS_EACH_SECTION_FILECOUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;

			if (p->used == 0)
				continue;

			/* Full Matching*/
			if (!strcmp(p->name, filename)) {
				debug("%s: hit table[%d/%d], index[%d/%d]\n", __func__,
				       i, RKSS_PARTITION_TABLE_COUNT, n, RKSS_EACH_SECTION_FILECOUNT);
				memcpy(pfileinfo, p, sizeof(struct rkss_file_info));
				free(table_data);
				return i * RKSS_EACH_SECTION_FILECOUNT + n;
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

			debug("%s: Matched folder: %s\n", __func__, p->name);
			free(table_data);
			return TEE_ERROR_GENERIC;
UNMATCHFOLDER:
			debug("%s: Unmatched ...\n", __func__);
		}
	}
	debug("%s: file or dir no found!\n", __func__);
	free(table_data);
	return TEE_ERROR_GENERIC;
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
	if (len > RKSS_MAX_NAME_LENGTH - 1) {
		printf("%s: filename is too long. length:%u\n", __func__, len);
		return TEE_ERROR_GENERIC;
	}

	table_data = malloc(RKSS_DATA_SECTION_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("%s: malloc table_data failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("%s: rkss_read_patition_tables failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	dir_num = 0;
	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_SECTION_LEN, RKSS_DATA_SECTION_LEN);

		for (n = 0; n < RKSS_EACH_SECTION_FILECOUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;

			if (p->used == 0)
				continue;

			/* Full Matching*/
			ret = memcmp(p->name, filename, strlen(filename));
			debug("%s: comparing [fd:%d] : %s ?= %s , ret:%d\n", __func__,
			       i * RKSS_EACH_SECTION_FILECOUNT + n, p->name, filename, ret);
			if (!ret && strlen(p->name) > strlen(filename)) {
				chk = p->name + strlen(filename);
				if (*chk == '/') {
					file = p->name + strlen(filename) + 1;
					subdir = strtok(file, "/");
					debug("%s: found: %s\n", __func__, subdir);
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

	rkss.index = RKSS_USED_FLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_read_multi_sections failed!\n", __func__);
		return TEE_ERROR_GENERIC;
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

	printf("%s: Not enough space available in secure storage!\n", __func__);
	return TEE_ERROR_GENERIC;
}

static int rkss_incref_multi_usedflags_sections(unsigned int index, unsigned int num)
{
	struct rk_secure_storage rkss = {0};
	int ret, value, i;
	uint8_t *flag;

	if ((index + num) >= RKSS_DATA_SECTION_COUNT) {
		printf("%s: index[%d] out of range.\n", __func__, index);
		return TEE_ERROR_GENERIC;
	}

	rkss.index = RKSS_USED_FLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_read_multi_sections failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	for (i = 0; i < num; i++, index++) {
		flag = (uint8_t *)rkss.data + (int)index/2;
		value = index & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (++value > 0xF) {
			printf("%s: reference out of data: %d\n", __func__, value);
			value = 0xF;
		}
		*flag = index & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_write_multi_sections failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	return TEE_SUCCESS;
}

static int rkss_decref_multi_usedflags_sections(unsigned int index, unsigned int num)
{
	struct rk_secure_storage rkss = {0};
	int ret, value, i;
	uint8_t *flag;

	if ((index + num) >= RKSS_DATA_SECTION_COUNT) {
		printf("%s: index[%d] out of range.\n", __func__, index);
		return TEE_ERROR_GENERIC;
	}

	rkss.index = RKSS_USED_FLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_read_multi_sections failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	for (i = 0; i < num; i++, index++) {
		flag = (uint8_t *)rkss.data + (int)index/2;
		value = index & 0x1 ? *flag & 0x0F : (*flag & 0xF0) >> 4;
		if (--value < 0) {
			printf("%s: reference out of data: %d\n", __func__, value);
			value = 0x0;
		}
		*flag = index & 0x1 ? (*flag & 0xF0) | (value & 0x0F) :
				(*flag & 0x0F) | (value << 4);
	}
	ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_write_multi_sections failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	return TEE_SUCCESS;
}

static int rkss_write_empty_ptable(struct rkss_file_info *pfileinfo)
{
	unsigned char *table_data;
	int ret, i, n;
	struct rk_secure_storage rkss = {0};
	void *pdata;
	struct rkss_file_info *p;

	table_data = malloc(RKSS_DATA_SECTION_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("%s: malloc table_data failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("%s: rkss_read_patition_tables failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	for (i = 0; i < RKSS_PARTITION_TABLE_COUNT; i++) {
		rkss.index = i;
		memcpy(rkss.data, table_data + rkss.index * RKSS_DATA_SECTION_LEN, RKSS_DATA_SECTION_LEN);
		for (n = 0; n < RKSS_EACH_SECTION_FILECOUNT; n++) {
			pdata = rkss.data;
			p = (struct rkss_file_info *)pdata;
			p += n;
			if (p->used == 0) {
				debug("%s: write ptable in [%d][%d].\n", __func__, i, n);
				memcpy(p, pfileinfo, sizeof(struct rkss_file_info));
				p->used = 1;
				p->id = n;
				debug("%s: write emt ptable : [%d,%d] name:%s, index:%d,\n",
					__func__, i, n, p->name, p->index);
				debug("%s: size:%d, used:%d\n", __func__, p->size, p->used);
				ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
				if (ret < 0) {
					printf("%s: rkss_write_multi_sections failed!\n", __func__);
					free(table_data);
					return TEE_ERROR_GENERIC;
				}

				free(table_data);
				return i * RKSS_EACH_SECTION_FILECOUNT + n;
			}
		}
	}
	debug("%s: No enough ptable space available in secure storage.\n", __func__);
	free(table_data);
	return TEE_ERROR_GENERIC;
}

static int rkss_write_back_ptable(int fd, struct rkss_file_info *pfileinfo)
{
	int i = fd / RKSS_EACH_SECTION_FILECOUNT;
	int n = fd - (RKSS_EACH_SECTION_FILECOUNT * i);
	struct rk_secure_storage rkss = {0};
	int ret;
	void *pdata;
	struct rkss_file_info *p;

	rkss.index = i;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_read_multi_sections failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	pdata = rkss.data;
	p = (struct rkss_file_info *)pdata;
	p += n;

	memcpy(p, pfileinfo, sizeof(struct rkss_file_info));
	debug("%s: write ptable : [%d,%d] name:%s, index:%d, size:%d, used:%d\n",
		__func__, i, n, p->name, p->index, p->size, p->used);

	ret = rkss_write_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_write_multi_sections failed!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
#endif

	return TEE_SUCCESS;
}

static uint32_t ree_fs_new_open(size_t num_params,
				struct optee_msg_param *params)
{
	char *filename;
	int fd;
	struct rkss_file_info p = {0};

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(filename) > RKSS_MAX_NAME_LENGTH) {
		printf("%s: file name too long. %s\n", __func__, filename);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s: open file: %s, len: %zu\n", __func__, filename, strlen(filename));
	fd = rkss_get_fileinfo_by_name(filename, &p);
	if (fd < 0) {
		debug("%s: no such file. %s\n", __func__, filename);
		return TEE_ERROR_ITEM_NOT_FOUND;
	}

	debug("%s: filename=%s, fd=%d\n", __func__, filename, fd);

	params[2].u.value.a = fd;
	return TEE_SUCCESS;
}

static u32 ree_fs_new_create(size_t num_params,
			     struct optee_msg_param *params)
{
	char *filename;
	int fd;
	int ret, num;
	struct rkss_file_info p = {0};

	/* file open flags: O_RDWR | O_CREAT | O_TRUNC
	 * if file exists, we must remove it first.
	 */
	ret = rkss_begin_commit();
	if (ret < 0) {
		printf("%s: rkss_begin_commit failed!", __func__);
		return -1;
	}

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(filename) > RKSS_MAX_NAME_LENGTH) {
		printf("%s: file name too long. %s\n", __func__, filename);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s: create file: %s, len: %zu\n", __func__, filename, strlen(filename));
	fd = rkss_get_fileinfo_by_name(filename, &p);
	if (fd >= 0) {
		debug("%s: file exist, clear it. %s\n", __func__, filename);
		/* decrease ref from usedflags */
		num = p.size / RKSS_DATA_SECTION_LEN + 1;
		ret = rkss_decref_multi_usedflags_sections(p.index, num);
		if (ret < 0) {
			printf("%s: rkss_decref_multi_usedflags_sections error!\n", __func__);
			return TEE_ERROR_GENERIC;
		}

		/* rm from ptable */
		memset(&p, 0, sizeof(struct rkss_file_info));
		ret = rkss_write_back_ptable(fd, &p);
		if (ret < 0) {
			printf("%s: write back error %d\n", __func__, ret);
			return TEE_ERROR_GENERIC;
		}
	}

	debug("%s: create new file: %s\n", __func__, filename);
	strcpy(p.name, filename);
	p.index = 0;
	p.size = 0;
	p.used = 1;
	p.flags = RK_FS_R | RK_FS_W;
	fd = rkss_write_empty_ptable(&p);
	if (fd < 0) {
		printf("%s: write empty ptable error. %s\n", __func__, filename);
		return TEE_ERROR_GENERIC;
	}

	debug("%s: filename=%s, fd=%d.\n", __func__, filename, fd);

	params[2].u.value.a = fd;

	ret = rkss_finish_commit();
	if (ret < 0) {
		printf("%s: rkss_finish_commit failed!\n", __func__);
		return -1;
	}
	return TEE_SUCCESS;
}

static u32 ree_fs_new_close(size_t num_params,
			    struct optee_msg_param *params)
{
	debug("%s: close file!\n", __func__);
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
	struct rkss_file_info p = {0};
	int di, section_num;
	uint8_t *temp_file_data;

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;


	data = tee_supp_param_to_va(params + 1);
	if (!data)
		return TEE_ERROR_BAD_PARAMETERS;
	len = params[1].u.rmem.size;

	debug("%s: fd:%d, len:%zu, offs:%ld\n", __func__, fd, len, offs);

	ret = rkss_get_fileinfo_by_index(fd, &p);
	if (ret < 0) {
		debug("%s: unavailable fd: %d!\n", __func__, fd);
		return TEE_ERROR_GENERIC;
	}

	if (offs >= p.size)
		return TEE_ERROR_BAD_PARAMETERS;

	section_num = p.size / RKSS_DATA_SECTION_LEN + 1;
	temp_file_data = malloc(section_num * RKSS_DATA_SECTION_LEN);
	ret = rkss_read_multi_sections(temp_file_data, p.index, section_num);
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
	struct rkss_file_info p = {0};
	int ret, fd, new_size;
	int section_num;
	uint8_t *file_data = 0, *temp_file_data = 0;

	ret = rkss_begin_commit();
	if (ret < 0) {
		printf("%s: rkss_begin_commit failed!\n", __func__);
		return -1;
	}

	fd = params[0].u.value.b;
	offs = params[0].u.value.c;

	data = tee_supp_param_to_va(params + 1);
	if (!data)
		return TEE_ERROR_BAD_PARAMETERS;
	len = params[1].u.rmem.size;

	debug("%s: fd:%d, len:%zu, offs:%ld \n", __func__, fd, len, offs);

	ret = rkss_get_fileinfo_by_index(fd, &p);
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
		section_num = p.size / RKSS_DATA_SECTION_LEN + 1;
		temp_file_data = malloc(section_num * RKSS_DATA_SECTION_LEN);
		ret = rkss_read_multi_sections(temp_file_data, p.index, section_num);
		if (ret < 0) {
			printf("%s: unavailable file index %d section_num %d\n",
					__func__, p.index, section_num);
			ret = TEE_ERROR_GENERIC;
			goto out;
		}
		memcpy(file_data, temp_file_data, p.size);
		free(temp_file_data);
		temp_file_data = 0;
		ret = rkss_decref_multi_usedflags_sections(p.index, section_num);
		if (ret < 0) {
			printf("%s: rkss_decref_multi_usedflags_sections error!\n", __func__);
			ret = TEE_ERROR_GENERIC;
			goto out;
		}
	}

	/* update new file info */
	memcpy(file_data + offs, data, len);
	p.size = new_size;
	section_num = new_size / RKSS_DATA_SECTION_LEN + 1;
	p.index = rkss_get_empty_section_from_usedflags(section_num);
	debug("%s: get empty section in %d\n", __func__, p.index);
	p.used = 1;
	ret = rkss_incref_multi_usedflags_sections(p.index, section_num);
	if (ret < 0) {
		printf("%s: rkss_incref_multi_usedflags_sections error!\n", __func__);
		ret = TEE_ERROR_GENERIC;
		goto out;
	}

	ret = rkss_write_back_ptable(fd, &p);
	if (ret < 0) {
		printf("TEEC: ree_fs_new_write: write ptable error!\n");
		ret = TEE_ERROR_GENERIC;
		goto out;
	}

	/* write new file data */
	temp_file_data = malloc(section_num * RKSS_DATA_SECTION_LEN);
	memset(temp_file_data, 0, section_num * RKSS_DATA_SECTION_LEN);
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
	ret = rkss_finish_commit();
	if (ret < 0) {
		printf("%s: rkss_finish_commit failed!\n", __func__);
		return -1;
	}

	return TEE_SUCCESS;
}

/* TODO: update file data space */
static u32 ree_fs_new_truncate(size_t num_params,
			       struct optee_msg_param *params)
{
	size_t len;
	int fd, ret;
	struct rkss_file_info p = {0};
	unsigned int section_num_old, section_num_new;

	ret = rkss_begin_commit();
	if (ret < 0) {
		printf("%s: rkss_begin_commit failed!\n", __func__);
		return -1;
	}

	fd = params[0].u.value.b;
	len = params[0].u.value.c;

	debug("%s: fd:%d, lenth:%zu\n", __func__, fd, len);

	ret = rkss_get_fileinfo_by_index(fd, &p);
	if (ret < 0) {
		printf("%s: fd:%d unvailable!\n", __func__, fd);
		return TEE_ERROR_GENERIC;
	}
	if (len > p.size) {
		printf("%s: truncate error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}
	section_num_old = p.size / RKSS_DATA_SECTION_LEN + 1;
	section_num_new = len / RKSS_DATA_SECTION_LEN + 1;
	ret = rkss_decref_multi_usedflags_sections(p.index + section_num_new, section_num_old - section_num_new);
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
	ret = rkss_finish_commit();
	if (ret < 0) {
		printf("%s: rkss_finish_commit failed!\n", __func__);
		return -1;
	}

	return TEE_SUCCESS;
}

static u32 ree_fs_new_remove(size_t num_params,
			     struct optee_msg_param *params)
{
	char *filename;
	struct rkss_file_info p = {0};
	int ret, fd, num;

	ret = rkss_begin_commit();
	if (ret < 0) {
		printf("%s: rkss_begin_commit failed!\n", __func__);
		return -1;
	}

	filename = tee_supp_param_to_va(params + 1);
	if (!filename)
		return TEE_ERROR_BAD_PARAMETERS;

	ret = rkss_get_fileinfo_by_name(filename, &p);
	if (ret < 0) {
		printf("%s: no such file. %s\n", __func__, filename);
		return 0;
	}
	fd = ret;

	debug("%s: filename:%s fd:%d index:%d size:%d\n",
			__func__, filename, fd, p.index, p.size);

	/* decrease ref from usedflags */
	num = p.size / RKSS_DATA_SECTION_LEN + 1;
	ret = rkss_decref_multi_usedflags_sections(p.index, num);
	if (ret < 0) {
		printf("%s: rkss_decref_multi_usedflags_sections error!\n", __func__);
		return TEE_ERROR_GENERIC;
	}

	/* rm from ptable */
	memset(&p, 0, sizeof(struct rkss_file_info));
	ret = rkss_write_back_ptable(fd, &p);
	if (ret < 0) {
		printf("%s: ree_fs_new_remove: write back error %d\n", __func__, ret);
		return TEE_ERROR_GENERIC;
	}

#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
	rkss_dump_usedflags();
#endif
	ret = rkss_finish_commit();
	if (ret < 0) {
		printf("%s: rkss_finish_commit failed!\n", __func__);
		return -1;
	}

	return TEE_SUCCESS;
}

static u32 ree_fs_new_rename(size_t num_params,
			     struct optee_msg_param *params)
{
	char *old_fname;
	char *new_fname;
	struct rkss_file_info p = {0};
	int ret;

	ret = rkss_begin_commit();
	if (ret < 0) {
		printf("%s: rkss_begin_commit failed!\n", __func__);
		return -1;
	}

	old_fname = tee_supp_param_to_va(params + 1);
	if (!old_fname)
		return TEE_ERROR_BAD_PARAMETERS;

	new_fname = tee_supp_param_to_va(params + 2);
	if (!new_fname)
		return TEE_ERROR_BAD_PARAMETERS;

	if (strlen(new_fname) > RKSS_MAX_NAME_LENGTH) {
		printf("%s: new file name too long. %s\n", __func__, new_fname);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	debug("%s: rename: %s -> %s\n", __func__, old_fname, new_fname);

	ret = rkss_get_fileinfo_by_name(old_fname, &p);
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
	ret = rkss_finish_commit();
	if (ret < 0) {
		printf("%s: rkss_finish_commit failed!\n", __func__);
		return -1;
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

	debug("%s: %s, seek/num:%d/%d\n", __func__, dirname, dir_seek, dir_num);
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
		debug("%s: END \n", __func__);
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

int tee_supp_rk_fs_init_v1(void)
{
	assert(sizeof(struct rkss_file_info) == 126);
	assert(512 / sizeof(struct rkss_file_info) == RKSS_EACH_SECTION_FILECOUNT);

	int ret;
	struct rk_secure_storage rkss = {0};
	unsigned char *table_data;

	if (check_security_exist(0) < 0)
		return -1;

	ret = rkss_resume();
	if (ret < 0) {
		printf("%s: rkss_resume failed!\n", __func__);
		return -1;
	}

	/* clean secure storage*/
#ifdef DEBUG_CLEAN_RKSS
	int i = 0;
	for (i = 0; i < RKSS_DATA_SECTION_COUNT; i++) {
		memset(rkss.data, 0, RKSS_DATA_SECTION_LEN);
		rkss.index = i;
		rkss_write_multi_sections(rkss.data, rkss.index, 1);
		printf("cleaned [%d] ", i);
	}
#endif
	ret = rkss_begin_commit();
	if (ret < 0) {
		printf("%s: rkss_begin_commit failed!\n", __func__);
		return -1;
	}

	table_data = malloc(RKSS_DATA_SECTION_LEN * RKSS_PARTITION_TABLE_COUNT);
	if (table_data == NULL) {
		printf("%s: malloc table_data failed\n", __func__);
		return -1;
	}
	ret = rkss_read_patition_tables(table_data);
	if (ret < 0) {
		printf("%s: rkss_read_patition_tables failed!\n", __func__);
		return -1;
	}

	/* Verify Partition Table*/
	rkss_verify_ptable(table_data);
	free(table_data);
	table_data = NULL;

	/* Verify Usedflags Section*/
	rkss.index = RKSS_USED_FLAGS_INDEX;
	ret = rkss_read_multi_sections(rkss.data, rkss.index, 1);
	if (ret < 0) {
		printf("%s: rkss_read_multi_sections failed!\n", __func__);
		return -1;
	}
	ret = rkss_verify_usedflags(&rkss);
	if (ret < 0) {
		printf("%s: rkss_verify_usedflags failed!\n", __func__);
		return -1;
	}

#ifdef DEBUG_RKFSS
	rkss_dump_ptable();
	rkss_dump_usedflags();
#endif

	ret = rkss_finish_commit();
	if (ret < 0) {
		printf("%s: rkss_finish_commit failed!\n", __func__);
		return -1;
	}

	return 0;
}

static int rkss_step;
int tee_supp_rk_fs_process_v1(size_t num_params,
			      struct optee_msg_param *params)
{
	if (!num_params || !tee_supp_param_is_value(params))
		return TEE_ERROR_BAD_PARAMETERS;

	switch (params->u.value.a) {
	case OPTEE_MRF_OPEN:
		debug(">>>>>>> [%d] OPTEE_MRF_OPEN!\n", rkss_step++);
		return ree_fs_new_open(num_params, params);
	case OPTEE_MRF_CREATE:
		debug(">>>>>>> [%d] OPTEE_MRF_CREATE!\n", rkss_step++);
		return ree_fs_new_create(num_params, params);
	case OPTEE_MRF_CLOSE:
		debug(">>>>>>> [%d] OPTEE_MRF_CLOSE!\n", rkss_step++);
		return ree_fs_new_close(num_params, params);
	case OPTEE_MRF_READ:
		debug(">>>>>>> [%d] OPTEE_MRF_READ!\n", rkss_step++);
		return ree_fs_new_read(num_params, params);
	case OPTEE_MRF_WRITE:
		debug(">>>>>>> [%d] OPTEE_MRF_WRITE!\n", rkss_step++);
		return ree_fs_new_write(num_params, params);
	case OPTEE_MRF_TRUNCATE:
		debug(">>>>>>> [%d] OPTEE_MRF_TRUNCATE!\n", rkss_step++);
		return ree_fs_new_truncate(num_params, params);
	case OPTEE_MRF_REMOVE:
		debug(">>>>>>> [%d] OPTEE_MRF_REMOVE!\n", rkss_step++);
		return ree_fs_new_remove(num_params, params);
	case OPTEE_MRF_RENAME:
		debug(">>>>>>> [%d] OPTEE_MRF_RENAME!\n", rkss_step++);
		return ree_fs_new_rename(num_params, params);
	case OPTEE_MRF_OPENDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_OPENDIR!\n", rkss_step++);
		return ree_fs_new_opendir(num_params, params);
	case OPTEE_MRF_CLOSEDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_CLOSEDIR!\n", rkss_step++);
		return ree_fs_new_closedir(num_params, params);
	case OPTEE_MRF_READDIR:
		debug(">>>>>>> [%d] OPTEE_MRF_READDIR!\n", rkss_step++);
		return ree_fs_new_readdir(num_params, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
