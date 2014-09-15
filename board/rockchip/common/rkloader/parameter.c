/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * Configuation settings for the rk3xxx chip platform.
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

#include <common.h>
#include <malloc.h>

#include "../config.h"
#include "parameter.h"


BootInfo gBootInfo;
uint32 parameter_lba;

static int find_mtd_part(cmdline_mtd_partition* this_mtd, const char* part_name)
{
	int i=0;
	for(i=0; i<this_mtd->num_parts; i++)
	{
		if( !strcmp(part_name, (const char*)this_mtd->parts[i].name) )
			return i;
	}
	
	return -1;
}

static int get_rdInfo(PBootInfo pboot_info)
{
	char *token = NULL;
	char cmdline[MAX_LINE_CHAR];
	int len = strlen("initrd=");

	strcpy(cmdline, pboot_info->cmd_line);
	pboot_info->ramdisk_load_addr = 0;
	pboot_info->ramdisk_offset = 0;
	pboot_info->ramdisk_size = 0;
    
	token = strtok(cmdline, " ");
	while(token != NULL)
	{
		if( !strncmp(token, "initrd=", len) )
		{
			token += len;
			pboot_info->ramdisk_load_addr = simple_strtoull(token, &token, 0);
			pboot_info->ramdisk_size = 0x80000;
            
			if(*token == ',')
			{
				token++;
				pboot_info->ramdisk_size = simple_strtoull(token, NULL, 0);
			}
            
			break;
		}
		token = strtok(NULL, " ");
	}
	return 0;
}

static unsigned long memparse (char *ptr, char **retptr)
{
	unsigned long ret = simple_strtoull (ptr, retptr, 0);

	switch (**retptr) {
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		(*retptr)++;
	default:
		break;
	}
	return ret;
}

static int get_part(char* parts, disk_partition_t* this_part, int* part_index)
{
	char delim;
	unsigned int mask_flags;
	unsigned int size = 0;
	unsigned int offset = 0;
	char name[PART_NAME] = "\0";

	if (*parts == '-')
	{	/* assign all remaining space to this partition */
		size = SIZE_REMAINING;
		parts++;
	}
	else
	{
		size = memparse(parts, &parts);
	}
	
	/* check for offset */
	if (*parts == '@')
	{
		parts++;
		offset = memparse(parts, &parts);
	}

	mask_flags = 0; /* this is going to be a regular partition */
	delim = 0;

	/* now look for name */
	if (*parts == '(')
	{
		delim = ')';
	}
	
	if (delim)
	{
		char *p;
		
		if ((p = strchr(parts+1, delim)) == 0)
		{
			return 0;
		}
		strncpy(name, parts+1, p-(parts+1));
		parts = p + 1;
	}
	else
	{ /* Partition_000 */
		sprintf(name, "Partition_%03d", *part_index);
	}
	
	/* test for options */
	if (strncmp(parts, "ro", 2) == 0)
	{
		mask_flags |= MTD_WRITEABLE;
		parts += 2;
	}
	
	/* if lk is found do NOT unlock the MTD partition*/
	if (strncmp(parts, "lk", 2) == 0)
	{
		mask_flags |= MTD_POWERUP_LOCK;
		parts += 2;
	}

	this_part->size = size;
	this_part->start = offset;
	this_part->blksz = RK_BLK_SIZE;
	snprintf((char*)this_part->name, sizeof(this_part->name), "%s", name);
	snprintf((char*)this_part->type, sizeof(this_part->type), "%s", "raw");

	if( (++(*part_index) < CONFIG_MAX_PARTITIONS) && (*parts == ',') )
	{
		get_part(++parts, this_part+1, part_index);
	}

	return 1;
}

static int mtdpart_parse(const char* string, cmdline_mtd_partition* this_mtd)
{
	char *token = NULL;
	char *s=NULL;
	char *p = NULL;
	char cmdline[MAX_LINE_CHAR];

	strcpy(cmdline, string);

	token = strtok(cmdline, " ");
	while(token != NULL)
	{
		if( !strncmp(token, "mtdparts=", strlen("mtdparts=")) )
		{
			s = token + strlen("mtdparts=");
			break;
			
		}
		token = strtok(NULL, " ");
	}
	
	if(s != NULL)
	{
		if (!(p = strchr(s, ':')))
			return 0;

		strncpy(this_mtd->mtd_id, s, p-s);
		s = p+1;
		get_part(s, this_mtd->parts+this_mtd->num_parts, &(this_mtd->num_parts));
	}

	return 1;
}


static int parse_cmdline(PBootInfo pboot_info)
{
// cmy: 解析命令行，获取分区信息
	pboot_info->cmd_mtd.num_parts = 0;
	pboot_info->cmd_mtd.mtd_id[0] = '\0';
 
	if(mtdpart_parse(pboot_info->cmd_line, &pboot_info->cmd_mtd))
	{
		pboot_info->index_misc = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_MISC);
		pboot_info->index_kernel = find_mtd_part(&pboot_info->cmd_mtd, 	PARTNAME_KERNEL);
		pboot_info->index_boot= find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_BOOT);
		pboot_info->index_recovery = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_RECOVERY);
		pboot_info->index_system = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_SYSTEM);
		pboot_info->index_backup = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_BACKUP);

		get_rdInfo(pboot_info);
        
		if(pboot_info->index_misc < 0 ||
			pboot_info->index_kernel < 0 ||
			pboot_info->index_boot < 0 ||
			pboot_info->index_recovery < 0)
		{
			return -1;
		}
	}
	else
	{
		//RkPrintf("ERROR: parse cmdline failed!");
		return -2;
	}

	return 0;
}

/* -1:error. */
static int ParseAtoi( const char * line )
{
	int base = 10;
	char max = '9';
	int v = 0;

	EATCHAR(line, ' ');
	if(*line == 0) return 0;
    
	if( line[1] == 'x' || line[1] == 'X' ) {
		base = 16;
		max = 'f';      /* F*/
		line += 2;
	}
	if( base == 10 ) {
		while( *line >= '0' && *line <= max ) {
			v *= base ;
			v += *line-'0';
			line++;
		}
	} else {
		while( *line >= '0' && *line <= max ) {
                        v *= base ;
                        if( *line >= 'a' )
                                v += *line-'a'+10;
                        else if ( *line >= 'A' )
                                v += *line-'A'+10;
                        else
                                v += *line-'0';
                        line++;
                }
	}

	return v;
}


static void ParseLine(PBootInfo pboot_info, char *line)
{
	if( !memcmp(line, "MAGIC:", strlen("MAGIC:")) )
	    pboot_info->magic_code = ParseAtoi(line+strlen("MAGIC:"));
	else if( !memcmp(line, "ATAG:", strlen("ATAG:")) )
	    pboot_info->atag_addr = ParseAtoi(line+strlen("ATAG:"));
	else if( !memcmp(line, "MACHINE:", strlen("MACHINE:")) )
	    pboot_info->machine_type = ParseAtoi(line+strlen("MACHINE:"));
	else if( !memcmp(line, "CHECK_MASK:", strlen("CHECK_MASK:")) )
	    pboot_info->check_mask = ParseAtoi(line+strlen("CHECK_MASK:"));
	else if( !memcmp(line, "KERNEL_IMG:", strlen("KERNEL_IMG:")) )
	    pboot_info->kernel_load_addr = ParseAtoi(line+strlen("KERNEL_IMG:"));
	else if( !memcmp(line, "BOOT_IMG:", strlen("BOOT_IMG:")) )
	    pboot_info->boot_offset = ParseAtoi(line+strlen("BOOT_IMG:"));
	else if( !memcmp(line, "RECOVERY_IMG:", strlen("RECOVERY_IMG:")) )
	    pboot_info->recovery_offset = ParseAtoi(line+strlen("RECOVERY_IMG:"));
	else if( !memcmp(line, "MISC_IMG:", strlen("MISC_IMG:")) )
	    pboot_info->misc_offset = ParseAtoi(line+strlen("MISC_IMG:"));
#if 0
	else if( !memcmp(line, "RECOVER_KEY:", strlen("RECOVER_KEY:")) )
	{//  RECOVER_KEY: 0,4,A,2,0  ==> GPIO4 PA2 低有效
		line += strlen("RECOVER_KEY:");
		EATCHAR(line, ' ');
		setup_key(line, &key_rockusb);
	}
	else if( !memcmp(line, "PRODUCT:", strlen("PRODUCT:")))
	{
		line += strlen("PRODUCT:");
		EATCHAR(line, ' ');
		strncpy(PRODUCT_NAME, line ,sizeof(PRODUCT_NAME));
	}
	else if( !memcmp(line, "PWR_HLD:", strlen("PWR_HLD:")) )
	{//  RECOVER_KEY: 0,4,A,2,0  ==> GPIO4 PA2 低有效
		line += strlen("PWR_HLD:");
		EATCHAR(line, ' ');
		setup_key(line, &pin_powerHold);
		if(pin_powerHold.key.gpio.valid)
			powerOn();
		}
	else if( !memcmp(line, "COMBINATION_KEY:", strlen("COMBINATION_KEY:")) )
	{
		line += strlen("COMBINATION_KEY:");
		EATCHAR(line, ' ');
		setup_key(line, &key_combination[KeyCombinationNum]);
		KeyCombinationNum++;
	}
#endif
	else if( !memcmp(line, "CMDLINE:", strlen("CMDLINE:")) )
	{
		line += strlen("CMDLINE:");
		EATCHAR(line, ' ');
		strcpy( pboot_info->cmd_line, line );

		//printf("ORG CMDLINE: %s\n", pboot_info->cmd_line);
		parse_cmdline(pboot_info);
	}
	// cmy: get firmware version
	else if( !memcmp(line, "FIRMWARE_VER:", strlen("FIRMWARE_VER:")) )
	{
		line += strlen("FIRMWARE_VER:");
		EATCHAR(line, ' ');
		strcpy( pboot_info->fw_version, line );
		//printf("FIRMWARE_VER: %s\n", pboot_info->fw_version);
	}
	//add by cjf, for fdt
	else if (!memcmp(line, "FDT_NAME:", strlen("FDT_NAME:")) )
	{
		line += strlen("FDT_NAME:");
		EATCHAR(line, ' ');
		strcpy( pboot_info->fdt_name, line );
		//printf("FDT_NAME: %s\n", pboot_info->fdt_name);
	}
	else if (!memcmp(line, "UART", strlen("UART"))) {
		rkplat_uart2UsbEn(1);
	}
	else
		printf("Unknow param: %s!\n", line);
}

//
// param  字符串
// line   获取到的一行数据存放在该变量中
// 返回值 偏移的位置
//
static char* getline(char* param, int32 len, char *line)
{
	int i=0;
	
	for(i=0; i<len; i++)
	{
		if(param[i]=='\n' || param[i]=='\0')
			break;
		
		if(param[i] != '\r')
			*(line++) = param[i];
	}
	
	*line='\0';
	
	return param+i+1;
}

int CheckParam(PLoaderParam pParam)
{
	uint32 crc = 0;
//	PRINT_D("Enter\n");
	
	if(pParam->tag != PARM_TAG)
	{
		printf("W: Invalid Parameter's tag (0x%08X)!\n", (unsigned int)pParam->tag);
		return -2;
	}

	if( pParam->length > (MAX_LOADER_PARAM-12) )
	{
		printf("E: Invalid parameter length(%d)!\n", (int)pParam->length);
		return -3;
	}

	crc = CRC_32CheckBuffer((unsigned char*)pParam->parameter, pParam->length+4);
	if(!crc)
	{
		printf("E: Para CRC failed!\n");
		return -4;
	}

//	pParam->crc = crc;

//	PRINT_D("Leave!\n");
//	RkPrintf("Check ok!\n");
	return 0;
}

int32 GetParam(uint32 param_addr, void *buf)
{
	PLoaderParam param = (PLoaderParam)buf;
	int iResult = 0;
	int i=0;
	int iRet = 0;
	int read_sec = MAX_LOADER_PARAM>>9;
	printf("GetParam\n");

	for(i=0; i<PARAMETER_NUM; i++)
	{
		if(StorageReadLba( param_addr+i*PARAMETER_OFFSET, buf, read_sec)==0 )//read_sec
		{
			iResult = CheckParam(param);
			if(iResult >= 0)
			{
				printf("check parameter success\n");
				return 0;
			}
			else
			{
				printf("Invalid parameter\n");
				iRet = -1;
			}
		}
		else
		{
			printf("read parameter fail\n");
			iRet = -2;
		}
	}

	return iRet;
}

int32 GetBackupParam(void *buf)
{
	int iResult = -1;
	return iResult;
}

int32 BackupParam(void *buf)
{
	return 0;
}

void ParseParam(PBootInfo pboot_info, char *param, uint32 len)
{
// 一行最多1024Bytes
	char *prev_param=NULL;
	char line[MAX_LINE_CHAR];
	int32 remain_len = (int32)len;
	
//	RkPrintf("Enter\n");
//	RkPrintf("--------------- PARAM ---------------\n");
	while(remain_len>0)
	{
		// 获取一行数据(不含回车换行符，且左边不含空格)
		prev_param = param;
		param = getline(param, remain_len, line);
		remain_len -= (param-prev_param);

//		RkPrintf("%s\n", line);
		// 去除空行及注释行
		if( line[0] != 0 && line[0] != '#' )
		{// 该行不是空行，并且不是注释行
			ParseLine(pboot_info, line);
		}
	}
//	RkPrintf("--------------------------------------\n");
//	RkPrintf("Leave\n");
}

void dump_disk_partitions(void)
{
	int i;

	printf("dump_disk_partitions:\n");
	for(i = 0;i < gBootInfo.cmd_mtd.num_parts;i++) {
		printf("partition(%s): start=0x%08lX, size=0x%08lX, type=%s\n", \
				gBootInfo.cmd_mtd.parts[i].name, gBootInfo.cmd_mtd.parts[i].start, \
				gBootInfo.cmd_mtd.parts[i].size, gBootInfo.cmd_mtd.parts[i].type);
	}
}

const disk_partition_t* get_disk_partition(const char* name)
{
	int part_index = find_mtd_part(&gBootInfo.cmd_mtd, name);
	if (part_index < 0) {
		printf("failed to find part:%s\n", name);
		return NULL;
	}
	return &gBootInfo.cmd_mtd.parts[part_index];
}

int load_disk_partitions(void)
{
	int i = 0;
	int ret = -1;
	cmdline_mtd_partition *cmd_mtd;
	PLoaderParam param = (PLoaderParam)memalign(ARCH_DMA_MINALIGN, MAX_LOADER_PARAM * PARAMETER_NUM);

	if (!GetParam(0, param)) {
		ParseParam( &gBootInfo, param->parameter, param->length );
		cmd_mtd = &(gBootInfo.cmd_mtd);
		for(i = 0;i < cmd_mtd->num_parts;i++) {
			if (i >= CONFIG_MAX_PARTITIONS) {
				printf("Failed! Too much partition: %d(%d)\n",
						cmd_mtd->num_parts, CONFIG_MAX_PARTITIONS);
				goto end;
			}
#ifdef DEBUG
			printf("partition(%s): offset=0x%08lX, size=0x%08lX\n", \
					cmd_mtd->parts[i].name, cmd_mtd->parts[i].start, \
					cmd_mtd->parts[i].size);
#endif
		}
		ret = 0;
	}
end:
	if (param)
		free(param);

	return ret;
}

