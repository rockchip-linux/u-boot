/*
 * (C) Copyright 2008-2014 Rockchip Electronics
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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef RKIMAGE_H
#define RKIMAGE_H

#include <fastboot.h>
#include "../config.h"
#include <resource.h>


#define BCD2INT(num) (((((num)>>4)&0x0F)*10)+((num)&0x0F))

#define BOOT_RESERVED_SIZE 57

typedef enum {
	RK27_DEVICE = 1,
	RK28_DEVICE = 2,
	RKNANO_DEVICE = 4
} ENUM_RKDEVICE_TYPE;

typedef enum {
	ENTRY471 = 1,
	ENTRY472 = 2,
	ENTRYLOADER = 4
} ENUM_RKBOOTENTRY;

typedef PACKED1 struct {
	unsigned short usYear;
	unsigned char  ucMonth;
	unsigned char  ucDay;
	unsigned char  ucHour;
	unsigned char  ucMinute;
	unsigned char  ucSecond;
} PACKED2 STRUCT_RKTIME, *PSTRUCT_RKTIME;

typedef PACKED1 struct {
	unsigned int uiTag;
	unsigned short usSize;
	unsigned int  dwVersion;
	unsigned int  dwMergeVersion;
	STRUCT_RKTIME stReleaseTime;
	ENUM_RKDEVICE_TYPE emSupportChip;
	unsigned char temp[12];
	unsigned char ucLoaderEntryCount;
	unsigned int dwLoaderEntryOffset;
	unsigned char ucLoaderEntrySize;
	unsigned char ucSignFlag;
	unsigned char ucRc4Flag;
	unsigned char reserved[BOOT_RESERVED_SIZE];
} PACKED2 STRUCT_RKBOOT_HEAD, *PSTRUCT_RKBOOT_HEAD;

typedef PACKED1 struct {
	unsigned char ucSize;
	ENUM_RKBOOTENTRY emType;
	unsigned char szName[40];
	unsigned int dwDataOffset;
	unsigned int dwDataSize;
	unsigned int dwDataDelay;//ÒÔÃëÎªµ¥Î»
} PACKED2 STRUCT_RKBOOT_ENTRY, *PSTRUCT_RKBOOT_ENTRY;


#define TAG_KERNEL          0x4C4E524B

typedef struct tag_rk_kernel_iamge {
	uint32  tag;
	uint32  size;
	char    image[1];
	uint32  crc;
} rk_kernel_iamge;


/* Android bootimage file format */
#define BOOT_MAGIC		"ANDROID!"
#define BOOT_MAGIC_SIZE		8
#define BOOT_NAME_SIZE		16
#define BOOT_ARGS_SIZE		512

typedef struct tag_rk_boot_img_hdr {
	unsigned char magic[BOOT_MAGIC_SIZE];

	unsigned int kernel_size;  /* size in bytes */
	unsigned int kernel_addr;  /* physical load addr */

	unsigned int ramdisk_size; /* size in bytes */
	unsigned int ramdisk_addr; /* physical load addr */

	unsigned int second_size;  /* size in bytes */
	unsigned int second_addr;  /* physical load addr */

	unsigned int tags_addr;    /* physical addr for kernel tags */
	unsigned int page_size;    /* flash page size we assume */
	unsigned int unused[2];    /* future expansion: should be 0 */

	unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

	unsigned char cmdline[BOOT_ARGS_SIZE];

	unsigned int id[8]; /* timestamp / checksum / sha1 / etc */

	unsigned char reserved[0x400-0x260];
	unsigned long signTag; /* 0x4E474953 */
	unsigned long signlen; /* maybe 128 or 256 */
	unsigned char rsaHash[256]; /* maybe 128 or 256, using max size 256 */
} rk_boot_img_hdr;


#define LOADER_MAGIC_SIZE     16
#define LOADER_HASH_SIZE      32
#define RK_UBOOT_MAGIC        "LOADER  "
#define RK_UBOOT_SIGN_TAG     0x4E474953
#define RK_UBOOT_SIGN_LEN     128
typedef struct tag_second_loader_hdr
{
	uint8_t magic[LOADER_MAGIC_SIZE];	/* "LOADER  " */

	uint32_t loader_load_addr;		/* physical load addr ,default is 0x60000000*/
	uint32_t loader_load_size;		/* size in bytes */
	uint32_t crc32;				/* crc32 */
	uint32_t hash_len;			/* 20 or 32 , 0 is no hash*/
	uint8_t hash[LOADER_HASH_SIZE];		/* sha */

	uint8_t reserved[1024-32-32];
	uint32_t signTag;			/* 0x4E474953 */
	uint32_t signlen;			/* maybe 128 or 256 */
	uint8_t rsaHash[256];			/* maybe 128 or 256, using max size 256 */
	uint8_t reserved2[2048-1024-256-8];
} second_loader_hdr;


/* rk image name define */
#define PARAMETER_NAME  "parameter"
#define LOADER_NAME     "loader"
#define UBOOT_NAME      "uboot"
#define MISC_NAME       "misc"
#define KERNEL_NAME     "kernel"
#define BOOT_NAME       "boot"
#define RECOVERY_NAME   "recovery"
#define SYSTEM_NAME     "system"
#define BACKUP_NAME     "backup"
#define RESOURCE_NAME   "resource"


int rkimage_load_image(rk_boot_img_hdr *hdr, const disk_partition_t *boot_ptn, \
		const disk_partition_t *kernel_ptn);
int rkimage_store_image(const char *name, const disk_partition_t *ptn, \
		struct cmd_fastboot_interface *priv);
int rkimage_handleDownload(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv);

int rkimage_partition_erase(const disk_partition_t *ptn);

resource_content rkimage_load_fdt(const disk_partition_t* ptn);
void rkimage_prepare_fdt(void);

#endif /* RKIMAGE_H */
