/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#ifndef RKIMAGE_H
#define RKIMAGE_H

#include <fastboot.h>
#include "../common/armlinux/config.h"

//from boot.c
typedef struct tagDRM_KEY_INFO
{
    uint32 drmtag;           // "DRMK" 0x4B4D5244
    uint32 drmLen;           // 504
    uint32 keyBoxEnable;     // 0:flash 1:emmc 2:sdcard1 3:sdcard2
    uint32 drmKeyLen;        //0 disable , 1~N : part 1~N
    uint32 publicKeyLen;     //0 disable , 1:enable
    uint32 secureBootLock;   //0 disable , 1:lock
    uint32 secureBootLockKey;//¼Ó½âÃÜÊÇÊ¹ÓÃ
    uint32 reserved0[(0x40-0x1C)/4];
    uint8  drmKey[0x80];      // key data
    uint32 reserved2[(0xFC-0xC0)/4];
    uint8  publicKey[0x104];      // key data
}DRM_KEY_INFO,*pDRM_KEY_INFO;

extern DRM_KEY_INFO gDrmKeyInfo;



#define TAG_KERNEL          0x4C4E524B

typedef struct tagKernelImg {
    uint32  tag;
    uint32  size;
    char    image[1];
    uint32  crc;
}KernelImg;

static int loadImage(uint32 offset, unsigned char *load_addr, size_t *image_size);

int loadRkImage(struct fastboot_boot_img_hdr *hdr, fbt_partition_t *boot_ptn, \
        fbt_partition_t *kernel_ptn);

#define BCD2INT(num) (((((num)>>4)&0x0F)*10)+((num)&0x0F))

#define  BOOT_RESERVED_SIZE 59

typedef enum
{
        RK27_DEVICE=1,
        RK28_DEVICE=2,
        RKNANO_DEVICE=4
}ENUM_RKDEVICE_TYPE;
typedef enum
{
    ENTRY471=1,
    ENTRY472=2,
    ENTRYLOADER=4
}ENUM_RKBOOTENTRY;

typedef PACKED1 struct
{
    unsigned short usYear;
    unsigned char  ucMonth;
    unsigned char  ucDay;
    unsigned char  ucHour;
    unsigned char  ucMinute;
    unsigned char  ucSecond;
} PACKED2 STRUCT_RKTIME,*PSTRUCT_RKTIME;

typedef struct
{
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
    unsigned char reserved[BOOT_RESERVED_SIZE];
} PACKED2 STRUCT_RKBOOT_HEAD,*PSTRUCT_RKBOOT_HEAD;

typedef struct
{
    unsigned char ucSize;
    ENUM_RKBOOTENTRY emType;
    unsigned char szName[40];
    unsigned int dwDataOffset;
    unsigned int dwDataSize;
    unsigned int dwDataDelay;//ÒÔÃëÎªµ¥Î»
} PACKED2 STRUCT_RKBOOT_ENTRY,*PSTRUCT_RKBOOT_ENTRY;

int handleRkFlash(char *name,
        struct cmd_fastboot_interface *priv);

#define LOADER_MAGIC_SIZE     16
#define LOADER_HASH_SIZE      32
#define RK_UBOOT_MAGIC        "LOADER  "
#define RK_UBOOT_SIGN_TAG     0x4E474953
#define RK_UBOOT_SIGN_LEN     128
typedef struct tag_second_loader_hdr
{
    uint8_t magic[LOADER_MAGIC_SIZE];  // "LOADER  "

    uint32_t loader_load_addr;           /* physical load addr ,default is 0x60000000*/
    uint32_t loader_load_size;           /* size in bytes */
    uint32_t crc32;                      /* crc32 */
    uint32_t hash_len;                   /* 20 or 32 , 0 is no hash*/
    uint8_t hash[LOADER_HASH_SIZE];     /* sha */

    uint8_t reserved[1024-32-32];
    uint32_t signTag; //0x4E474953
    uint32_t signlen; //128
    uint8_t rsaHash[256];
    uint8_t reserved2[2048-1024-256-8];
}second_loader_hdr;

#endif /* RKIMAGE_H */
