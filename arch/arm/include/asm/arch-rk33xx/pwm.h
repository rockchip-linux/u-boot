/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_PWM_H
#define __RKXX_PWM_H

#include <asm/io.h>


/*
 * Because the id will be also used in iomux.c, so the id order
 * should be same with that in iomux.h.
 */
enum rk_pwm_id {
	RK_PWM0,
	RK_PWM1,
	RK_PWM2,
	RK_PWM3,
	RK_PWM4,
	RK_VOP0_PWM,
	RK_VOP1_PWM,
	RK_PWM_ID_CNT,
};

#if defined(RKIO_VOP0_PWM_PHYS) || defined(RKIO_VOP1_PWM_PHYS)
#define RK_VOP_PWM
#endif

#endif /* __RKXX_PWM_H */
