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


/***************************************************************************
函数描述:
调用函数:
***************************************************************************/
void * ftl_memset(void *s, int c, unsigned int n)
{
	return ((void*)memset(s, c, n));
}

/***************************************************************************
函数描述:
调用函数:
***************************************************************************/
void* ftl_memcpy(void* pvTo, const void* pvForm, unsigned int size)
{
	if(((int)pvTo & 0x3)||((int)pvForm & 0x3)) {
		int i;
		char * pTo = (char *)pvTo;
		char * pForm = (char *)pvForm;

		for(i=0;i<size;i++) {
			*pTo++ = *pForm++;
		}

		return 0;
	}

	return ((void*)memcpy(pvTo, pvForm, size));
}

/***************************************************************************
函数描述:
调用函数:
***************************************************************************/
int ftl_memcmp(void *str1, void *str2, unsigned int count)
{
	return (memcmp(str1, str2, count));
}

