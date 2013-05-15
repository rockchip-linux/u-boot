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

#endif /* RKIMAGE_H */
