/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_IO_H
#define __RKXX_IO_H


/*
 * RKXX IO memory map:
 *
 */
#if defined(CONFIG_RKCHIP_RK3368)
	#include "io-rk3368.h"
#else
	#error "PLS config io-rkxx.h!"
#endif


#endif /* __RKXX_IO_H */
