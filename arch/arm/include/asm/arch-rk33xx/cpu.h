/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _RKXX_CPU_H
#define _RKXX_CPU_H


/* define rockchip cpu chiptype */
#define RKCHIP_UNKNOWN		0

/* rockchip cpu mask */
#define ROCKCHIP_CPU_MASK	0xffff0000
#define ROCKCHIP_CPU_RK3368	0x33680000
#define ROCKCHIP_CPU_RK3366	0x33660000
#define ROCKCHIP_CPU_RK3399	0x33990000

/* rockchip cpu type */
#define CONFIG_RK3368           (ROCKCHIP_CPU_RK3368 | 0x00)    /* rk3368 chip */
#define CONFIG_RK3366           (ROCKCHIP_CPU_RK3366 | 0x00)    /* rk3366 chip */
#define CONFIG_RK3399           (ROCKCHIP_CPU_RK3399 | 0x00)    /* rk3399 chip */


#endif	/* _RKXX_CPU_H */
