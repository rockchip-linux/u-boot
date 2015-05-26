/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
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
