/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_PMU_H
#define __RKXX_PMU_H

#include <asm/io.h>


#define pmu_readl(offset)	readl(RKIO_PMU_PHYS + offset)
#define pmu_writel(v, offset)	do { writel(v, RKIO_PMU_PHYS + offset); } while (0)


#if defined(CONFIG_RKCHIP_RK3368)
	#include "pmu-rk3368.h"
#elif defined(CONFIG_RKCHIP_RK3366)
	#include "pmu-rk3366.h"
#elif defined(CONFIG_RKCHIP_RK3399)
	#include "pmu-rk3399.h"
#elif defined(CONFIG_RKCHIP_RK322XH)
	#include "pmu-rk322xh.h"
#else
	#error "PLS config pmu-rkxx.h!"
#endif


#endif /* __RKXX_PMU_H */
