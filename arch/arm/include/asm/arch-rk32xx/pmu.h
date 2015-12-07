/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_PMU_H
#define __RKXX_PMU_H

#if defined(CONFIG_RKCHIP_RK3288)
	#include "pmu-rk3288.h"
#elif defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK322X)
	/* rk3036/rk322x no pmu module */
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#include "pmu-rk312X.h"
#else
	#error "PLS config pmu-rkxx.h!"
#endif

#endif /* __RKXX_PMU_H */
