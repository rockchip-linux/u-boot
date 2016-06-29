/*
 * drivers/video/rockchip/bmp_helper.h
 *
 * Copyright (C) 2012 Rockchip Corporation
 * Author: Mark Yao <mark.yao@rock-chips.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BMP_HELPER_H_
#define _BMP_HELPER_H_

#define BMP_RLE8_ESCAPE		0
#define BMP_RLE8_EOL		0
#define BMP_RLE8_EOBMP		1
#define BMP_RLE8_DELTA		2

#define range(x, min, max) ((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x))

int bmpdecoder(void *bmp_addr, void *dst, int dst_bpp);
#endif /* _BMP_HELPER_H_ */
