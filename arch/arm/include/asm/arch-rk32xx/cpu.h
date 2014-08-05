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

#ifndef _RK3XXX_CPU_H
#define _RK3XXX_CPU_H

/* define rockchip cpu chiptype */
#define RKCHIP_UNKNOWN		0

/* rockchip cpu mask */
#define ROCKCHIP_CPU_MASK	0xffff0000
#define ROCKCHIP_CPU_RK3026	0x30260000
#define ROCKCHIP_CPU_RK3036	0x30360000 /* audi-s */
#define ROCKCHIP_CPU_RK30XX	0x30660000
#define ROCKCHIP_CPU_RK30XXB	0x31680000
#define ROCKCHIP_CPU_RK312X	0x31260000 /* audi */
#define ROCKCHIP_CPU_RK3188	0x31880000
#define ROCKCHIP_CPU_RK319X	0x31900000
#define ROCKCHIP_CPU_RK3288	0x32880000

/* rockchip cpu type */
#define CONFIG_RK3036           (ROCKCHIP_CPU_RK3036 | 0x00)    /* rk3036 chip */
#define CONFIG_RK3066		(ROCKCHIP_CPU_RK30XX | 0x01)	/* rk3066 chip */
#define CONFIG_RK3126           (ROCKCHIP_CPU_RK312X | 0x00)    /* rk3126 chip */
#define CONFIG_RK3128           (ROCKCHIP_CPU_RK312X | 0x01)    /* rk3128 chip */
#define CONFIG_RK3168		(ROCKCHIP_CPU_RK30XXB | 0x01)	/* rk3168 chip */
#define CONFIG_RK3188		(ROCKCHIP_CPU_RK3188 | 0x00)	/* rk3188 chip */
#define CONFIG_RK3188_PLUS	(ROCKCHIP_CPU_RK3188 | 0x10)	/* rk3188 plus chip */
#define CONFIG_RK3288		(ROCKCHIP_CPU_RK3288 | 0x00)	/* rk3288 chip */

#endif	/* _RK3XXX_CPU_H */

