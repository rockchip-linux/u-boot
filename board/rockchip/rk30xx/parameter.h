#ifndef _PARAMETER_H_
#define _PARAMETER_H_

#include "mtdpart.h"

#define PARAMETER_NUM		8			    // parameter文件的备份个数
#define MAX_LINE_CHAR		(1024*64)	        // Parameters有多个Line组成，限制每个Line最大占1024 Bytes
#define MAX_LOADER_PARAM	(128*512)		// Parameters所占的最大Sector数(含tag、length、crc等)
#define PARM_TAG			0x4D524150
#define MAGIC_CODE			0x00280028
#define EATCHAR(x, c) for (; *(x) == (c); (x)++) ; // 去除字符串x中左边为c的字符


typedef struct tagLoaderParam
{
	uint32	tag;
	uint32	length;
	char	parameter[1];
//	char*	parameter;
	uint32	crc;
}LoaderParam, *PLoaderParam;

typedef struct tagBootInfo
{
	uint32 magic_code;
	uint16 machine_type;
	uint16 boot_index;		// 0 - normal boot, 1 - recovery
	uint32 atag_addr;
	uint32 misc_offset;
	uint32 kernel_load_addr;
	uint32 boot_offset;		// 以Sector为单位
	uint32 recovery_offset;		// 以Sector为单位
	uint32 ramdisk_offset;	// 以Sector为单位
	uint32 ramdisk_size;	// 以Byte为单位
	uint32 ramdisk_load_addr;
	uint32 is_kernel_in_boot;
	
	uint32 check_mask;	// 00 - 不校验， 01 - check kernel, 10 - check ramdisk, 11 - both check
	char cmd_line[MAX_LINE_CHAR];
	cmdline_mtd_partition cmd_mtd;

	int index_misc;
	int index_kernel;
	int index_boot;
	int index_recovery;
	int index_system;
	int index_backup;
	int index_snapshot;
	char fw_version[MAX_LINE_CHAR];
}BootInfo, *PBootInfo;

extern BootInfo			gBootInfo;
//extern uint32			gLoaderTlb[];
//extern uint8			gParamBuffer[];
extern uint32			parameter_lba;

/*
extern key_config		key_recover;
extern int KeyCombinationNum;
extern key_config		key_recover;
extern key_config       key_powerHold;
extern key_config		key_combination[MAX_COMBINATION_KEY];
*/

extern int32 rk30_getParameter(void);
extern int32 rk30_bootParameter(void);
extern int rk30_get_system_parameter(int32 *flashAddr,int32 *flashSize);
#endif
