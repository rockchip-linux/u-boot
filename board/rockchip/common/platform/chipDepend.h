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

#ifndef _CHIP_DEPEND_H
#define _CHIP_DEPEND_H

extern void DRVDelayUs(uint32 count);
extern void DRVDelayMs(uint32 count);
extern void DRVDelayS(uint32 count);

extern void ISetLoaderFlag(uint32 flag);
extern uint32 IReadLoaderFlag(void);

extern void FW_NandDeInit(void);

extern void rkplat_uart2UsbEn(uint32 en);

#endif
