/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
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
#if defined(CONFIG_RKCHIP_RK3288)
	#include "io-rk3288.h"
#elif defined(CONFIG_RKCHIP_RK3036)
	#include "io-rk3036.h"
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#include "io-rk312X.h"
#elif defined(CONFIG_RKCHIP_RK322X)
	#include "io-rk322x.h"
#else
	#error "PLS config io-rkxx.h!"
#endif


#endif /* __RKXX_IO_H */
