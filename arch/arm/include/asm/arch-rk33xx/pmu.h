/*
 * (C) Copyright 2008-2015 Rockchip Electronics
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
#else
	#error "PLS config pmu-rkxx.h!"
#endif


#endif /* __RKXX_PMU_H */
