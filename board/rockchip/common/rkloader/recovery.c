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

#include "../config.h"

//add parameter partition to cmdline, ota may update this partition.
void change_cmd_for_recovery(PBootInfo boot_info, char * rec_cmd)
{
	if(boot_info->index_recovery >= 0)
	{
		char* s = NULL;
		char szFind[128]="";

		sprintf(szFind, "%s=%s:", "mtdparts", boot_info->cmd_mtd.mtd_id);
		s = strstr(boot_info->cmd_line, szFind);
		if( s != NULL )
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

