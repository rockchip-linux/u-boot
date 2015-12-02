/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "../config.h"
#include <u-boot/crc.h>

uint32 CRC_32CheckBuffer(unsigned char * aData, uint32 aSize)
{
	uint32 crc_check = 0, crc_calc = 0;
	int i=0;
	if(aSize <= 4) {
		return 0;
	}
	aSize -= 4;

	for(i=3; i>=0; i--)
		crc_check = (crc_check<<8)+(*(aData+aSize+i));

	crc_calc = crc32_rk(0, aData, aSize);
	debug("rk crc32 check: crc_check = 0x%x, crc_calc = 0x%x\n", crc_check, crc_calc);
	if(crc_calc == crc_check)
		return crc_check;

	return 0;
}
