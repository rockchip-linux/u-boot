/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKPLAT_H
#define __RKPLAT_H


#include <config.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
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
#include "i2c.h"
#include "pm.h"

#ifdef CONFIG_RK_PL330
#include "pl330.h"
#endif

#ifdef CONFIG_RK_DMAC
#include "dma.h"
#endif

int rk_get_chiptype(void);
void rk_module_deinit(void);
#ifdef CONFIG_RK_MCU
void rk_mcu_init(void);
#endif


#endif /* __RKPLAT_H */
