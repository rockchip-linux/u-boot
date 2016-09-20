/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_GRF_H
#define __RKXX_GRF_H

#include <asm/io.h>

#define grf_readl(offset)	readl(RKIO_GRF_PHYS + offset)
#define grf_writel(v, offset)	do { writel(v, RKIO_GRF_PHYS + offset); } while (0)


#if defined(CONFIG_RKCHIP_RK3368)
	#include "grf-rk3368.h"
#elif defined(CONFIG_RKCHIP_RK3366)
	#include "grf-rk3366.h"
#elif defined(CONFIG_RKCHIP_RK3399)
	#include "grf-rk3399.h"
#elif defined(CONFIG_RKCHIP_RK322XH)
	#include "grf-rk322xh.h"
#else
	#error "PLS config grf-rkxx.h!"
#endif

#endif /* __RKXX_GRF_H */
