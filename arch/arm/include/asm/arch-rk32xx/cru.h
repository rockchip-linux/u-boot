/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_CRU_H
#define __RKXX_CRU_H


#define cru_readl(offset)	readl(RKIO_CRU_PHYS + offset)
#define cru_writel(v, offset)	do { writel(v, RKIO_CRU_PHYS + offset); } while (0)


#if defined(CONFIG_RKCHIP_RK3288)
	#include "cru-rk3288.h"
#elif defined(CONFIG_RKCHIP_RK3036)
	#include "cru-rk3036.h"
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#include "cru-rk312X.h"
#elif defined(CONFIG_RKCHIP_RK322X)
	#include "cru-rk322x.h"
#else
	#error "PLS config cru-rkxx.h!"
#endif

#endif /* __RKXX_CRU_H */
