/*
 * (C) Copyright 2008-2015 Rockchip Electronics
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
#else
	#error "PLS config grf-rkxx.h!"
#endif

#endif /* __RKXX_GRF_H */
