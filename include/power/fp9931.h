// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef FP9931_H
#define FP9931_H

#include <asm/gpio.h>
#include <linux/bitops.h>

#define FP9931_TMST_VALUE		0x00
#define FP9931_VCOM_SETTING		0x01
#define FP9931_VPOS_VNEG_SETTING	0x02
#define FP9931_PWRON_DELAY		0x03
#define FP9931_CONTROL_REG1		0x0B
#define FP9931_CONTROL_REG2		0x0C

#define fp9931_REG_MAX			0x0C

#define VPOS_VNEG_SETTING		GENMASK(5, 0)
#define PWRON_DELAY_tDLY1		GENMASK(1, 0)
#define PWRON_DELAY_tDLY2		GENMASK(3, 2)
#define PWRON_DELAY_tDLY3		GENMASK(5, 4)
#define PWRON_DELAY_tDLY4		GENMASK(7, 6)
#define CONTROL_REG1_V3P3_EN		BIT(1)
#define CONTROL_REG1_SS_TIME		GENMASK(7, 6)
#define CONTROL_REG2_VN_CL		GENMASK(1, 0)
#define CONTROL_REG2_VP_CL		GENMASK(3, 2)
#define CONTROL_REG2_FIX_RD_PTR		BIT(7)

#define FP9931_VCOM_DRIVER_NAME 	"fp9931-vcom"
#define FP9931_VPOS_VNEG_DRIVER_NAME	"fp9931-vpos-vneg"
#define FP9931_THERMAL_COMTATIBLE_NAME	"fp9931-thermal"

struct fp9931_plat_data {
	struct gpio_desc power_gpio[4];
	int num_power_gpio;
	struct gpio_desc enable_gpio;
};

#endif /* FP9931_H */
