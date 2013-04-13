#ifndef _AFPTOOL_H_
#define _AFPTOOL_H_

#ifndef PART_NAME
	#define PART_NAME				32
#endif

#define RELATIVE_PATH				64

#ifndef MAX_PARTS
	#define MAX_PARTS				20
#endif

#ifndef MAX_MACHINE_MODEL
	#define MAX_MACHINE_MODEL		64
#endif

#ifndef MAX_MANUFACTURER
	#define MAX_MANUFACTURER		64
#endif

#define MAX_PACKAGE_FILES			16

typedef struct tagRKIMAGE_ITEM
{
	char name[PART_NAME];// 分区名称
	char file[RELATIVE_PATH];// 相对路径名，提取文件时用到
	unsigned int offset;// 文件在Image中的偏移
	unsigned int flash_offset;// 烧写到Flash中的位置(以sector为单位)
	unsigned int usespace;// 文件占用空间（按PAGE对齐)
	unsigned int size;// 字节数，实际文件大小
}RKIMAGE_ITEM;

typedef struct tagRKIMAGE_HDR
{
	unsigned int tag;
	unsigned int size;// 文件大小，不含末尾的CRC校验码
	char machine_model[MAX_MACHINE_MODEL];
	char manufacturer[MAX_MANUFACTURER];
	int item_count;
	RKIMAGE_ITEM item[MAX_PACKAGE_FILES];
}RKIMAGE_HDR;

#define RKIMAGE_TAG				0x46414B52

#define PAGESIZE				2048

#define BOOTLOADER				"Rock28Boot(L).bin"
#define PARTNAME_BOOTLOADER		"bootloader"

#endif
