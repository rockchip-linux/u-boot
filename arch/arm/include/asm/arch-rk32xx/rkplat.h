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
#ifndef __RKPLAT_H
#define __RKPLAT_H


/********************************************************************
**                      common head files                           *
********************************************************************/
#include <config.h>
#include <common.h>
#include <command.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <asm/errno.h>

#include "typedef.h"
#include "cpu.h"
#include "io.h"
#include "pmu.h"
#include "grf.h"
#include "cru.h"
#include "irqs.h"
#include "gpio.h"
#include "iomux.h"

#include "clock.h"
#include "uart.h"
#include "pwm.h"
#include "rk_i2c.h"
#include "pm.h"

#ifdef CONFIG_RK_PL330
#include "pl330.h"
#endif

#ifdef CONFIG_RK_DMAC
#include "dma.h"
#endif

int rk_get_chiptype(void);
void rk_module_deinit(void);

#endif /* __RKPLAT_H */

