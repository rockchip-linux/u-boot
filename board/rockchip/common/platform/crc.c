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
#include <u-boot/crc.h>

uint32 CRC_32CheckBuffer(unsigned char * aData, uint32 aSize)
{
	uint32 crc = 0;
	int i=0;
	if( aSize <= 4 )
	{
		return 0;
	}
	aSize -= 4;

	for(i=3; i>=0; i--)
		crc = (crc<<8)+(*(aData+aSize+i));

	if( crc32(0, aData, aSize) == crc )
		return crc;

	return 0;
}
