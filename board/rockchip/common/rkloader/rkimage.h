/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
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
	uint16_t usYear;
	uint8_t  ucMonth;
	uint8_t  ucDay;
	uint8_t  ucHour;
	uint8_t  ucMinute;
	uint8_t  ucSecond;
} PACKED2 STRUCT_RKTIME, *PSTRUCT_RKTIME;

typedef PACKED1 struct {
	uint32_t uiTag;
	uint16_t usSize;
	uint32_t  dwVersion;
	uint32_t  dwMergeVersion;
	STRUCT_RKTIME stReleaseTime;
	ENUM_RKDEVICE_TYPE emSupportChip;
	uint8_t temp[12];
	uint8_t ucLoaderEntryCount;
	uint32_t dwLoaderEntryOffset;
	uint8_t ucLoaderEntrySize;
	uint8_t ucSignFlag;
	uint8_t ucRc4Flag;
	uint8_t reserved[BOOT_RESERVED_SIZE];
} PACKED2 STRUCT_RKBOOT_HEAD, *PSTRUCT_RKBOOT_HEAD;

typedef PACKED1 struct {
	uint8_t ucSize;
	ENUM_RKBOOTENTRY emType;
	uint8_t szName[40];
	uint32_t dwDataOffset;
	uint32_t dwDataSize;
	uint32_t dwDataDelay;
} PACKED2 STRUCT_RKBOOT_ENTRY, *PSTRUCT_RKBOOT_ENTRY;


#define TAG_KERNEL          0x4C4E524B

typedef struct tag_rk_kernel_image {
	uint32_t  tag;
	uint32_t  size;
	int8_t    image[1];
	uint32_t  crc;
} rk_kernel_image;


/* Android bootimage file format */
#define BOOT_MAGIC		"ANDROID!"
#define BOOT_MAGIC_SIZE		8
#define BOOT_NAME_SIZE		16
#define BOOT_ARGS_SIZE		512

typedef struct tag_rk_boot_img_hdr {
	uint8_t magic[BOOT_MAGIC_SIZE];

	uint32_t kernel_size;  /* size in bytes */
	uint32_t kernel_addr;  /* physical load addr */

	uint32_t ramdisk_size; /* size in bytes */
	uint32_t ramdisk_addr; /* physical load addr */

	uint32_t second_size;  /* size in bytes */
	uint32_t second_addr;  /* physical load addr */

	uint32_t tags_addr;    /* physical addr for kernel tags */
	uint32_t page_size;    /* flash page size we assume */
	uint32_t unused[2];    /* future expansion: should be 0 */

	uint8_t name[BOOT_NAME_SIZE]; /* asciiz product name */

	uint8_t cmdline[BOOT_ARGS_SIZE];

	uint32_t id[8]; /* timestamp / checksum / sha1 / etc */

	/* Add for sha256 and sha512 */
	uint32_t unused2[3];    /* future expansion: should be 0 */
	uint32_t sha_flag;      /* sha flag: 256 or 512 */
	uint32_t sha[16];  /* sha data */

	uint8_t reserved[0x400-0x260-0x50];
	/* start at 1K offset */
	uint32_t signTag; /* 0x4E474953 */
	uint32_t signlen; /* maybe 128 or 256 */
	uint8_t rsaHash[256]; /* maybe 128 or 256, using max size 256 */
	uint8_t rsaHash2[256]; /* 256 */
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
#define LOGO_NAME       "logo"
#define FACTORY_NAME    "factory"


int rkimage_load_image(rk_boot_img_hdr *hdr, const disk_partition_t *boot_ptn, \
		const disk_partition_t *kernel_ptn);
int rkimage_store_image(const char *name, const disk_partition_t *ptn, \
		struct cmd_fastboot_interface *priv);
int rkimage_handleDownload(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv);

int rkimage_partition_erase(const disk_partition_t *ptn);

resource_content rkimage_load_fdt(const disk_partition_t* ptn);
resource_content rkimage_load_fdt_ram(void *addr, size_t len);
void rkimage_prepare_fdt(void);

#endif /* RKIMAGE_H */
