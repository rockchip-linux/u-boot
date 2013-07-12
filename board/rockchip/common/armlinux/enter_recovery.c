#include "config.h"
#include "mk_various_file.h"

extern int parse_cmdline(PBootInfo pboot_info);

#ifdef USE_RECOVER
// 将字符串string从开头开始的replen个字符，替换为字符串newstr
void replace_fore_string(char* string, int replen, const char* newstr)
{
	int newlen = strlen(newstr);
	char *p1 = string;
	char *p2 = string;
	
	if(newlen == replen)
	{
		strncpy(string, newstr, newlen);
	}
	else if(newlen < replen)
	{
		strncpy(string, newstr, newlen);
		p1 += newlen;
		p2 += replen;
		while(*p2) *(p1++) = *(p2++);
		*p1 = 0;
	}
	else if(newlen > replen)
	{
		int i=0;
		int len = strlen(string);
		p1 = string+len;
		p2 = p1 + (newlen - replen);
		for(i=0; i<(len - replen); i++)
			*(p2--) = *(p1--);
		strncpy(string, newstr, newlen);
	}
}
#endif

#ifdef USE_RECOVER
void change_cmd_for_recovery(PBootInfo boot_info , char * rec_cmd )
{
	if(boot_info->index_recovery >= 0)
	{
		char* s = NULL;
		char szFind[128]="";
	
		sprintf(szFind, "%s=%s:", "mtdparts", boot_info->cmd_mtd.mtd_id);
		s = strstr(boot_info->cmd_line, szFind);
		// 修改分区表，显示parameter分区
		if( s != NULL )
		{
			mtd_partition *misc_part = boot_info->cmd_mtd.parts+boot_info->index_misc;
			char replace_str[64]="";
			sprintf(replace_str, "0x%08X@0x%08X(%s),", misc_part->offset, 0, PARTNAME_PARAMETER);
			s += strlen(szFind);
			replace_fore_string(s, 0, replace_str);
		}
		// 重新解析命令行
		parse_cmdline(boot_info);
		strcat(boot_info->cmd_line,rec_cmd);
        ISetLoaderFlag(SYS_KERNRL_REBOOT_FLAG|BOOT_RECOVER); //会丢失 recovery的参数
	}
	else
		;//PRINT_E("Try to boot recovery system, but can't find recovery partition!\n");
}
#endif

