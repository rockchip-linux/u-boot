/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _PMIC_ACT8931_H_
#define _PMIC_ACT8931_H_

#include <power/pmic.h>

#define COMPAT_ACTIVE_ACT8931  "act,act8931"
#define ACT8931_NUM_REGULATORS 7

#define I2C_CH  I2C_BUS_CH0
#define I2C_ADDRESS  0x5b
#define I2C_REG_LEN  1
#define I2C_VALUE_LEN 1
#define ACT8931_I2C_SPEED	100000
#define I2C_READ(addr,value) i2c_read(I2C_ADDRESS,addr,I2C_REG_LEN,value,I2C_VALUE_LEN);
#define I2C_WRITE(addr,value) i2c_write(I2C_ADDRESS,addr,I2C_REG_LEN,value,I2C_VALUE_LEN);


#define ACT8931_BUCK1_SET_VOL_BASE 0x20
#define ACT8931_BUCK2_SET_VOL_BASE 0x30
#define ACT8931_BUCK3_SET_VOL_BASE 0x40

#define ACT8931_BUCK2_SLP_VOL_BASE 0x21
#define ACT8931_BUCK3_SLP_VOL_BASE 0x31
#define ACT8931_BUCK4_SLP_VOL_BASE 0x41

#define ACT8931_LDO1_SET_VOL_BASE 0x50
#define ACT8931_LDO2_SET_VOL_BASE 0x58
#define ACT8931_LDO3_SET_VOL_BASE 0x60
#define ACT8931_LDO4_SET_VOL_BASE 0x68

#define ACT8931_BUCK1_CONTR_BASE 0x22
#define ACT8931_BUCK2_CONTR_BASE 0x32
#define ACT8931_BUCK3_CONTR_BASE 0x42

#define ACT8931_LDO1_CONTR_BASE 0x51
#define ACT8931_LDO2_CONTR_BASE 0x59
#define ACT8931_LDO3_CONTR_BASE 0x61
#define ACT8931_LDO4_CONTR_BASE 0x69

#define BUCK_VOL_MASK 0x3f
#define LDO_VOL_MASK 0x3f


#define BUCK_EN_MASK 0x80
#define LDO_EN_MASK 0x80

#define VOL_MIN_IDX 0x00
#define VOL_MAX_IDX 0x3f

struct  act8931_reg_table {
	char	*name;
	char	reg_ctl;
	char	reg_vol;
};

struct pmic_act8931 {
	struct pmic *pmic;
	int node;	/*device tree node*/
	struct fdt_gpio_state pwr_hold;
};

#endif


