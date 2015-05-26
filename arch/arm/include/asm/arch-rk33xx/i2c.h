/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _RKXX_I2C_H_
#define _RKXX_I2C_H_


typedef enum rk_i2c_bus_ch {
#if defined(CONFIG_RKCHIP_RK3368)
	I2C_BUS_CH0,
	I2C_BUS_CH1,
	I2C_BUS_CH2,
	I2C_BUS_CH3,
	I2C_BUS_CH4,
	I2C_BUS_CH5,
#else
	#error "PLS config chiptype for i2c bus!"
#endif
	I2C_BUS_MAX
} rk_i2c_bus_ch_t;


#endif /* _RKXX_I2C_H_ */
