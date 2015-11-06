/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __PMIC_RICOH619__
#define __PMIC_RICOH619__

#include <power/pmic.h>

#define COMPAT_RICOH_RICOH619  "ricoh,ricoh619"
#define RICOH619_I2C_ADDR 	0x32
#define RICOH619_I2C_SPEED	200000
#define RICOH619_NUM_REGULATORS	17

#define PSWR_REG                0x07
#define VINDAC_REG              0x03
#define PWRFUNC			0x0d
/* for ADC */
#define INTEN_REG               0x9D
#define EN_ADCIR3_REG           0x8A
#define ADCCNT1_REG		0x64
#define ADCCNT2_REG		0x65
#define ADCCNT3_REG             0x66
#define VBATDATAH_REG           0x6A
#define VBATDATAL_REG           0x6B
#define VSYSDATAH_REG           0x70
#define VSYSDATAL_REG           0x71

#define CHGCTL1_REG             0xB3
#define REGISET1_REG            0xB6
#define REGISET2_REG            0xB7
#define CHGISET_REG             0xB8
#define TIMSET_REG              0xB9
#define BATSET1_REG             0xBA
#define BATSET2_REG             0xBB

#define CHGSTATE_REG            0xBD

#define FG_CTRL_REG             0xE0
#define SOC_REG                 0xE1
#define RE_CAP_H_REG            0xE2
#define RE_CAP_L_REG            0xE3
#define FA_CAP_H_REG            0xE4
#define FA_CAP_L_REG            0xE5
#define TT_EMPTY_H_REG          0xE7
#define TT_EMPTY_L_REG          0xE8
#define TT_FULL_H_REG           0xE9
#define TT_FULL_L_REG           0xEA
#define VOLTAGE_1_REG           0xEB
#define VOLTAGE_2_REG           0xEC
#define TEMP_1_REG              0xED
#define TEMP_2_REG              0xEE

#define CC_CTRL_REG             0xEF
#define CC_SUMREG3_REG          0xF3
#define CC_SUMREG2_REG          0xF4
#define CC_SUMREG1_REG          0xF5
#define CC_SUMREG0_REG          0xF6
#define CC_AVERAGE1_REG         0xFB
#define CC_AVERAGE0_REG         0xFC

/* bank 1 */
/* Top address for battery initial setting */
#define BAT_INIT_TOP_REG        0xBC
#define TEMP_GAIN_H_REG         0xD6
#define TEMP_OFF_H_REG          0xD8
#define BAT_REL_SEL_REG         0xDA
#define BAT_TA_SEL_REG          0xDB


#define ricoh619_rails(_name) "RICOH619_"#_name

/* RICHOH Regulator IDs */
enum regulator_id {
	RICOH619_ID_DC1,
	RICOH619_ID_DC2,	
	RICOH619_ID_DC3,
	RICOH619_ID_DC4,
	RICOH619_ID_DC5,
	RICOH619_ID_LDO1,
	RICOH619_ID_LDO2,
	RICOH619_ID_LDO3,
	RICOH619_ID_LDO4,
	RICOH619_ID_LDO5,
	RICOH619_ID_LDO6,
	RICOH619_ID_LDO7,
	RICOH619_ID_LDO8,
	RICOH619_ID_LDO9,
	RICOH619_ID_LDO10,
	RICOH619_ID_LDORTC1,
	RICOH619_ID_LDORTC2,
};

struct ricoh619_regulator {
	int		id;
	int		sleep_id;
	/* Regulator register address.*/
	u8		reg_en_reg;
	u8		en_bit;
	u8		reg_disc_reg;
	u8		disc_bit;
	u8		vout_reg;
	u8		vout_mask;
	u8		vout_reg_cache;
	u8		sleep_reg;
	u8		eco_reg;
	u8		eco_bit;
	u8		eco_slp_reg;
	u8		eco_slp_bit;

	/* chip constraints on regulator behavior */
	int			min_uV;
	int			max_uV;
	int			step_uV;
	int			nsteps;

	/* regulator specific turn-on delay */
	u16			delay;
	int (*set_voltage) (struct ricoh619_regulator *ri, int min_uV, int max_uV,
			    unsigned *selector);

};

struct pmic_ricoh619 {
	struct pmic *pmic;
	int node;	/*device tree node*/
};

#endif
