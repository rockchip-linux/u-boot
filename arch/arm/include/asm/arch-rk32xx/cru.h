/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#else
	#error "PLS config cru-rkxx.h!"
#endif

#endif /* __RKXX_CRU_H */
