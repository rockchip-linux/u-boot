/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_CRU_H
#define __RKXX_CRU_H


#define cru_readl(offset)	readl(RKIO_CRU_PHYS + offset)
#define cru_writel(v, offset)	do { writel(v, RKIO_CRU_PHYS + offset); } while (0)


#if defined(CONFIG_RKCHIP_RK3368)
	#include "cru-rk3368.h"
#elif defined(CONFIG_RKCHIP_RK3366)
	#include "cru-rk3366.h"
#elif defined(CONFIG_RKCHIP_RK3399)
	#include "cru-rk3399.h"
#elif defined(CONFIG_RKCHIP_RK322XH)
	#include "cru-rk322xh.h"
#else
	#error "PLS config cru-rkxx.h!"
#endif

#endif /* __RKXX_CRU_H */
