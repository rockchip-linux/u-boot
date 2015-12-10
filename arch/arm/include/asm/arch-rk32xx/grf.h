/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_GRF_H
#define __RKXX_GRF_H


#define grf_readl(offset)	readl(RKIO_GRF_PHYS + offset)
#define grf_writel(v, offset)	do { writel(v, RKIO_GRF_PHYS + offset); } while (0)


#if defined(CONFIG_RKCHIP_RK3288)
	#include "grf-rk3288.h"
#elif defined(CONFIG_RKCHIP_RK3036)
	#include "grf-rk3036.h"
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#include "grf-rk312X.h"
#elif defined(CONFIG_RKCHIP_RK322X)
	#include "grf-rk322x.h"
#else
	#error "PLS config grf-rkxx.h!"
#endif

#endif /* __RKXX_GRF_H */
