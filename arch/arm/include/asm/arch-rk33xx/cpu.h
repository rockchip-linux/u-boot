/*
 * (C) Copyright 2008-2015 Rockchip Electronics
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


/* rockchip cpu type */
#define CONFIG_RK3368           (ROCKCHIP_CPU_RK3368 | 0x00)    /* rk3368 chip */


#endif	/* _RKXX_CPU_H */
