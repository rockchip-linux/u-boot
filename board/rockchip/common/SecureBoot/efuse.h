/*
 * (C) Copyright 2008-2014 Rockchip Electronics
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
#ifndef _EFUSE_H
#define _EFUSE_H


/*
 * efuse read api
 * buff: void unit, the data buffer for read.
 *	 rk3128: char unit
 *	 rk3288: word uint
 *
 * addr: char unit, efuse data start address.
 *	 rk3128: addr = addr is real efuse address
 *	 rk3288: addr = addr / 4, change to word unit
 *
 * size: char unit, efuse read size.
 *	 rk3128: size = size is real efuse size
 *	 rk3288: size = size / 4, change to word unit
 */
extern int32 EfuseRead(void *buff, uint32 addr, uint32 size);

#endif

