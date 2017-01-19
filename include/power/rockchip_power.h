/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __ROCKCHIP_PMIC_H_
#define __ROCKCHIP_PMIC_H_
#include <power/pmic.h>

enum pmic_id {
	PMIC_ID_UNKNOW,
	PMIC_ID_RICOH619,
	PMIC_ID_ACT8846,
	PMIC_ID_RK805,
	PMIC_ID_RK808,
	PMIC_ID_RK816,
	PMIC_ID_RK818,
	PMIC_ID_RT5025,
	PMIC_ID_RT5036,
	PMIC_ID_ACT8931,
};

/* rockchip first i2c node as parent for pmic */
extern int g_i2c_node;

unsigned char get_rockchip_pmic_id(void);
int dwc_otg_check_dpdm(void);
int is_charging(void);

int pmic_act8846_init(unsigned char bus);
int pmic_ricoh619_init(unsigned char bus);
int pmic_rk808_init(unsigned char bus);
int pmic_rk816_init(unsigned char bus);
char *pmic_get_rk8xx_id(unsigned char bus);
int pmic_rk818_init(unsigned char bus);
int pmic_rt5025_init(unsigned char bus);
int pmic_rt5036_init(unsigned char bus);
int pmic_act8931_init(unsigned char bus);

int pmic_ricoh619_charger_setting(int current);
int pmic_act8846_charger_setting(int current);
int pmic_rk808_charger_setting(int current);
int pmic_rk818_charger_setting(int current);
int pmic_act8931_charger_setting(int current);

void pmic_ricoh619_shut_down(void);
void pmic_act8846_shut_down(void);
void pmic_rk808_shut_down(void);
void pmic_rk816_shut_down(void);
void pmic_rk818_shut_down(void);
void pmic_rt5025_shut_down(void);
void pmic_rt5036_shut_down(void);
void pmic_act8931_shut_down(void);

int fg_cw201x_init(unsigned char bus);
int fg_ricoh619_init(unsigned char bus,uchar addr);
int fg_rk818_init(unsigned char bus,uchar addr);
int fg_rk816_init(unsigned char bus, uchar addr);
int fg_rt5025_init(unsigned char bus, uchar addr);
int fg_rt5036_init(unsigned char bus, uchar addr);
int charger_bq25700_init(void);
int fusb302_init(void);

int ricoh619_poll_pwr_key_sta(void);
int pmic_rk816_poll_pwrkey_stat(void);

int ec_battery_init(void);

struct regulator_init_reg_name {
	const char *name;
};

#define MAX_DCDC_NUM			5
#define MAX_REGULATOR_NUM		20
#define MAX_PWM_NUM			3

extern struct regulator_init_reg_name regulator_init_pmic_matches[MAX_REGULATOR_NUM];
extern struct regulator_init_reg_name regulator_init_pwm_matches[MAX_PWM_NUM];

int regulator_register_check(int num_matches);


#endif /* __ROCKCHIP_PMIC_H_ */
