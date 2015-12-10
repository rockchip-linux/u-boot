/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _EFUSE_H
#define _EFUSE_H


/*
 * efuse read api
 * base: efuse io base address.
 *
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
extern int32 SecureEfuseRead(void *base, void *buff, uint32 addr, uint32 size);
extern int32 FtEfuseRead(void *base, void *buff, uint32 addr, uint32 size);

#endif
