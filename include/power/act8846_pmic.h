/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _PMIC_ACT8846_H_
#define _PMIC_ACT8846_H_

#include <power/pmic.h>

#define COMPAT_ACTIVE_ACT8846  "act,act8846"
#define COMPAT_ACTIVE_SEMI_ACT8846  "active-semi,act8846"
#define ACT8846_NUM_REGULATORS 12

#define I2C_CH  I2C_BUS_CH0
#define I2C_ADDRESS  0x5a
#define I2C_REG_LEN  1
#define I2C_VALUE_LEN 1
#define ACT8846_I2C_SPEED	200000
#define I2C_READ(addr,value) i2c_read(I2C_ADDRESS,addr,I2C_REG_LEN,value,I2C_VALUE_LEN);
#define I2C_WRITE(addr,value) i2c_write(I2C_ADDRESS,addr,I2C_REG_LEN,value,I2C_VALUE_LEN);


#define act8846_BUCK1_SET_VOL_BASE 0x10
#define act8846_BUCK2_SET_VOL_BASE 0x20
#define act8846_BUCK3_SET_VOL_BASE 0x30
#define act8846_BUCK4_SET_VOL_BASE 0x40

#define act8846_BUCK2_SLP_VOL_BASE 0x21
#define act8846_BUCK3_SLP_VOL_BASE 0x31
#define act8846_BUCK4_SLP_VOL_BASE 0x41

#define act8846_LDO1_SET_VOL_BASE 0x50
#define act8846_LDO2_SET_VOL_BASE 0x58
#define act8846_LDO3_SET_VOL_BASE 0x60
#define act8846_LDO4_SET_VOL_BASE 0x68
#define act8846_LDO5_SET_VOL_BASE 0x70
#define act8846_LDO6_SET_VOL_BASE 0x80
#define act8846_LDO7_SET_VOL_BASE 0x90
#define act8846_LDO8_SET_VOL_BASE 0xa0
//#define act8846_LDO9_SET_VOL_BASE 0xb1

#define act8846_BUCK1_CONTR_BASE 0x12
#define act8846_BUCK2_CONTR_BASE 0x22
#define act8846_BUCK3_CONTR_BASE 0x32
#define act8846_BUCK4_CONTR_BASE 0x42

#define act8846_LDO1_CONTR_BASE 0x51
#define act8846_LDO2_CONTR_BASE 0x59
#define act8846_LDO3_CONTR_BASE 0x61
#define act8846_LDO4_CONTR_BASE 0x69
#define act8846_LDO5_CONTR_BASE 0x71
#define act8846_LDO6_CONTR_BASE 0x81
#define act8846_LDO7_CONTR_BASE 0x91
#define act8846_LDO8_CONTR_BASE 0xa1

#define BUCK_VOL_MASK 0x3f
#define LDO_VOL_MASK 0x3f


#define BUCK_EN_MASK 0x80
#define LDO_EN_MASK 0x80

#define VOL_MIN_IDX 0x00
#define VOL_MAX_IDX 0x3f

struct  act8846_reg_table {
	char	*name;
	char	reg_ctl;
	char	reg_vol;
};

struct pmic_act8846 {
	struct pmic *pmic;
	int node;	/*device tree node*/
	struct fdt_gpio_state pwr_hold;
};

#endif


