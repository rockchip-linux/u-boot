/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include "../config.h"
#include "rkloader.h"


#if 0
#define MaxFlashReadSize  16384  //8MB
int32 rkloader_CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec)
{
	uint8 * pSdram = (uint8*)dest_addr;
	uint16 sec = 0;
	uint32 LBA = src_addr;
	uint32 remain_sec = total_sec;

	//  RkPrintf("Enter >> src_addr=0x%08X, dest_addr=0x%08X, total_sec=%d\n", src_addr, dest_addr, total_sec);

	//  RkPrintf("(0x%X->0x%X)  size: %d\n", src_addr, dest_addr, total_sec);

	while(remain_sec > 0)
	{
		sec = (remain_sec > MaxFlashReadSize) ? MaxFlashReadSize : remain_sec;
		if(StorageReadLba(LBA,(uint8*)pSdram, sec) != 0)
		{
			return -1;
		}
		remain_sec -= sec;
		LBA += sec;
		pSdram += sec*512;
	}

	//  RkPrintf("Leave\n");
	return 0;
}
#else
#define MaxFlashReadSize  128  //64KB
int32 rkloader_CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec)
{
#ifdef CONFIG_RK_NVME_BOOT_EN
	ALLOC_ALIGN_BUFFER(u8, buf, RK_BLK_SIZE * MaxFlashReadSize, SZ_4K);
#else
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, RK_BLK_SIZE * MaxFlashReadSize);
#endif
	uint8 * pSdram = (uint8 *)(unsigned long)dest_addr;
	uint16 sec = 0;
	uint32 LBA = src_addr;
	uint32 remain_sec = total_sec;

	while(remain_sec > 0)
	{
		sec = (remain_sec > MaxFlashReadSize) ? MaxFlashReadSize : remain_sec;
		if(StorageReadLba(LBA, (uint8*)buf, sec) != 0) {
			return -1;
		}
		memcpy(pSdram, buf, RK_BLK_SIZE * sec);
		remain_sec -= sec;
		LBA += sec;
		pSdram += sec * RK_BLK_SIZE;
	}

	return 0;
}
#endif

int rkloader_CopyMemory2Flash(uint32 src_addr, uint32 dest_offset, int sectors)
{
	uint16 sec = 0;
	uint32 remain_sec = sectors;

	while(remain_sec > 0)
	{
		sec = (remain_sec>32)?32:remain_sec;

		if(StorageWriteLba(dest_offset, (void *)(unsigned long)src_addr, sec, 0) != 0)
		{
			return -2;
		}

		remain_sec -= sec;
		src_addr += sec*512;
		dest_offset += sec;
	}

	return 0;
}


static int execute_cmd(PBootInfo pboot_info, char* cmdlist, bool* reboot)
{
	char* cmd = cmdlist;

	*reboot = FALSE;
	while(*cmdlist)
	{
		if(*cmdlist=='\n') *cmdlist='\0';
		++cmdlist;
	}

	while(*cmd)
	{
		PRINT_I("bootloader cmd: %s\n", cmd);

		if(!strcmp(cmd, "update-bootloader"))
		{
			PRINT_I("--- update bootloader ---\n");
			if(rkidb_update_loader(false) == 0)
			{
				*reboot = TRUE;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			PRINT_I("Unsupport cmd: %s\n", cmd);
		}

		cmd += strlen(cmd)+1;
	}

	return 0;
}


extern uint8* g_32secbuf;

static int dispose_bootloader_cmd(struct bootloader_message *msg,
		const disk_partition_t *misc_part)
{
	int ret = 0;

	if (0 == strcmp(msg->command, "bootloader")
			|| 0 == strcmp(msg->command, "loader")) // 新Loader才能支持"loader"命令
	{
		bool reboot;

		FW_ReIntForUpdate();
		if(execute_cmd(&gBootInfo, msg->recovery, &reboot))
		{
			ret = -1;
		}

		{// 不管成功与否，将misc清0
			int i=0;
			memset(g_32secbuf, 0, 32*528);
			for(i=0; i<3; i++)
			{
				StorageWriteLba(misc_part->start+i*32, (void*)g_32secbuf,32, 0);
			}

			if(reboot)
			{
				PRINT_I("reboot\n");
				ISetLoaderFlag(SYS_LOADER_REBOOT_FLAG | BOOT_NORMAL);
				reset_cpu(0);
			}
		} 
	}

	return ret;
}

//add parameter partition to cmdline, ota may update this partition.
void rkloader_change_cmd_for_recovery(PBootInfo boot_info, char * rec_cmd)
{
	if(boot_info->index_recovery >= 0)
	{
		char *s = NULL;
		char szFind[128] = "";

		sprintf(szFind, "%s=%s:", "mtdparts", boot_info->cmd_mtd.mtd_id);
		s = strstr(boot_info->cmd_line, szFind);
		if(s != NULL)
		{
			s += strlen(szFind);
			char tmp[MAX_LINE_CHAR] = "\0";
			int max_size = sizeof(boot_info->cmd_line) -
				(s - boot_info->cmd_line);
			//parameter is 4M.
			snprintf(tmp, sizeof(tmp),
					"0x00002000@0x00000000(parameter),%s", s);
			snprintf(s, max_size, "%s", tmp);
		}

		strcat(boot_info->cmd_line, rec_cmd);
		ISetLoaderFlag(SYS_KERNRL_REBOOT_FLAG|BOOT_RECOVER);//set recovery flag.
	}
}


#define MISC_PAGES          3
#define MISC_COMMAND_PAGE   1
#define PAGE_SIZE           (16 * 1024)//16K
#define MISC_SIZE           (MISC_PAGES * PAGE_SIZE)//48K
#define MISC_COMMAND_OFFSET (MISC_COMMAND_PAGE * PAGE_SIZE / RK_BLK_SIZE)//32

int rkloader_run_misc_cmd(void)
{
	struct bootloader_message *bmsg = NULL;
#ifdef CONFIG_RK_NVME_BOOT_EN
	ALLOC_ALIGN_BUFFER(u8, buf, DIV_ROUND_UP(sizeof(struct bootloader_message),
			RK_BLK_SIZE) * RK_BLK_SIZE, SZ_4K);
#else
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, DIV_ROUND_UP(sizeof(struct bootloader_message),
			RK_BLK_SIZE) * RK_BLK_SIZE);
#endif
	const disk_partition_t *ptn = get_disk_partition(MISC_NAME);
	
	if (!ptn) {
		printf("misc partition not found!\n");
		return false;
	}
	
	bmsg = (struct bootloader_message *)buf;
	if (StorageReadLba(ptn->start + MISC_COMMAND_OFFSET, buf, DIV_ROUND_UP(
					sizeof(struct bootloader_message), RK_BLK_SIZE)) != 0) {
		printf("failed to read misc\n");
		return false;
	}
	if(!strcmp(bmsg->command, "boot-recovery")) {
		printf("got recovery cmd from misc.\n");
#ifdef CONFIG_CMD_BOOTRK
		char *const boot_cmd[] = {"bootrk", "recovery"};
		do_bootrk(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
#endif
		return false;
	} else if (!strcmp(bmsg->command, "boot-factory")) {
                printf("got factory cmd from misc.\n");
#ifdef CONFIG_CMD_BOOTRK
                char *const boot_cmd[] = {"bootrk", "factory"};
                do_bootrk(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
#endif
		return false;
	} else if((!strcmp(bmsg->command, "bootloader")) ||
			(!strcmp(bmsg->command, "loader"))) {
		printf("got bootloader cmd from misc.\n");
		const disk_partition_t* misc_part = get_disk_partition(MISC_NAME);
		if (!misc_part) {
			printf("misc partition not found!\n");
			return false;
		}
		return dispose_bootloader_cmd(bmsg, misc_part);
	}

	return false;
}


void rkloader_fixInitrd(PBootInfo pboot_info, int ramdisk_addr, int ramdisk_sz)
{
#define MAX_BUF_SIZE 100
	char str[MAX_BUF_SIZE];
	char *cmd_line = strdup(pboot_info->cmd_line);
	char *s_initrd_start = NULL;
	char *s_initrd_end = NULL;
	int len = 0;

	if (!cmd_line)
		return;

	s_initrd_start = strstr(cmd_line, "initrd=");
	if (s_initrd_start) {
		len = strlen(cmd_line);
		s_initrd_end = strstr(s_initrd_start, " ");
		if (!s_initrd_end)
			*s_initrd_start = '\0';
		else {
			len = cmd_line + len - s_initrd_end;
			memcpy(s_initrd_start, s_initrd_end, len);
			*(s_initrd_start + len) = '\0';
		}
	}
	snprintf(str, sizeof(str), "initrd=0x%08X,0x%08X", ramdisk_addr, ramdisk_sz);

#ifndef CONFIG_OF_LIBFDT
	snprintf(pboot_info->cmd_line, sizeof(pboot_info->cmd_line),
			"%s %s", str, cmd_line);
#else
	snprintf(pboot_info->cmd_line, sizeof(pboot_info->cmd_line),
			"%s", cmd_line);
#endif
	free(cmd_line);
}


int rkloader_set_bootloader_msg(struct bootloader_message* bmsg)
{
#ifdef CONFIG_RK_NVME_BOOT_EN
	ALLOC_ALIGN_BUFFER(u8, buf, DIV_ROUND_UP(sizeof(struct bootloader_message),
			RK_BLK_SIZE) * RK_BLK_SIZE, SZ_4K);
#else
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, DIV_ROUND_UP(sizeof(struct bootloader_message),
			RK_BLK_SIZE) * RK_BLK_SIZE);
#endif
	memcpy(buf, bmsg, sizeof(struct bootloader_message));
	const disk_partition_t *ptn = get_disk_partition(MISC_NAME);
	if (!ptn) {
		printf("misc partition not found!\n");
		return -1;
	}

	return rkloader_CopyMemory2Flash((uint32)(unsigned long)buf, ptn->start + MISC_COMMAND_OFFSET,
			DIV_ROUND_UP(sizeof(struct bootloader_message), RK_BLK_SIZE));
}


