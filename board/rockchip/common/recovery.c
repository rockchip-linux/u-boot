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

#include "config.h"


// 将字符串string从开头开始的replen个字符，替换为字符串newstr
static void replace_fore_string(char* string, int replen, const char* newstr)
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

void change_cmd_for_recovery(PBootInfo boot_info, char * rec_cmd)
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
			int i;
			char replace_str[64]="";
			//parameter is 4M.
			sprintf(replace_str, "0x00002000@0x%08X(%s),", 0, "parameter");
			s += strlen(szFind);
			replace_fore_string(s, 0, replace_str);
		}

		strcat(boot_info->cmd_line, rec_cmd);
		ISetLoaderFlag(SYS_KERNRL_REBOOT_FLAG|BOOT_RECOVER); //会丢失 recovery的参数
	}
}

