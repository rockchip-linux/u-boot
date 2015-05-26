/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _RK_I2C_H_
#define _RK_I2C_H_


typedef enum rk_i2c_bus_ch {
#if defined(CONFIG_RKCHIP_RK3288)
	I2C_BUS_CH0,
	I2C_BUS_CH1,
	I2C_BUS_CH2,
	I2C_BUS_CH3,
	I2C_BUS_CH4,
	I2C_BUS_CH5,
#elif defined(CONFIG_RKCHIP_RK3036)
	I2C_BUS_CH0,
	I2C_BUS_CH1,
	I2C_BUS_CH2,
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	I2C_BUS_CH0,
	I2C_BUS_CH1,
	I2C_BUS_CH2,
	I2C_BUS_CH3,
#else
	#error "PLS config chiptype for i2c bus!"
#endif
	I2C_BUS_MAX
} rk_i2c_bus_ch_t;

#endif /* _RK_I2C_H_ */












