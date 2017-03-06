/*
 * Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * zhangqing < zhangqing@rock-chips.com >
 * andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/rockchip_power.h>
#include <errno.h>
#include <pwm.h>
#include <asm/arch/rkplat.h>
#include <fdtdec.h>

DECLARE_GLOBAL_DATA_PTR;

#define COMPAT_ROCKCHIP_PWM	"rockchip_pwm_regulator"
#define COMPAT_PWM_REGULATOR	"pwm-regulator"
#define PWM_NUM_REGULATORS	2
int pwm_max_volt[2];
int pwm_coefficient[2];

struct regulator_init_reg_name regulator_init_pwm_matches[MAX_PWM_NUM];

struct pwm_regulator {
	int period;
	int polarity;
	unsigned int pwm_vol_map_count;
	unsigned int coefficient;
	unsigned int init_voltage;
	unsigned int max_voltage;
	unsigned int pwm_voltage;
	int *volt_table;
};

struct pwm_regulator pwm;

struct fdt_regulator_match *pmic_regulator_reg_matches;

static struct fdt_regulator_match pwm_reg_matches[] = {
	{ .prop = "pwm_dcdc1",},
	{ .prop = "pwm_dcdc2",},
};

int regulator_register_check(int num_matches)
{
	int i, ret = 0;

	for (i = 0; i < MAX_DCDC_NUM; i++) {
		if (strcmp(regulator_init_pwm_matches[num_matches].name, regulator_init_pmic_matches[i].name) == 0) {
			printf("DCDC %s has been used, pwm regulator not init it.\n",
					regulator_init_pwm_matches[num_matches].name);
			ret = -ENODEV;
			return ret;
		}
	}
	return ret;
}

static int pwm_regulator_set_rate(int rate, int pwm_id)
{
	int duty_cycle;

	duty_cycle = (rate * (pwm.period) / 100) ;
	pwm_config(pwm_id, duty_cycle, pwm.period);

	return 0;
}

static int pwm_regulator_set_voltage(int pwm_id, int num_matches,
		int min_uV, int max_uV)
{
	u32 size = pwm.pwm_vol_map_count;
	u32 i, vol, pwm_value;
	int ret = 0;

	ret = regulator_register_check(num_matches);
	if (ret < 0) {
		return ret;
	}

	if (!pwm.volt_table) {
		pwm_value = (min_uV - pwm_reg_matches[num_matches].min_uV) /
			     ((pwm_reg_matches[num_matches].max_uV -
			     pwm_reg_matches[num_matches].min_uV) / 100);
	} else {
		if (min_uV < pwm.volt_table[0] ||
		    max_uV > pwm.volt_table[size-1]) {
			printf("voltage_map voltage is out of table\n");
			return -EINVAL;
		}

		for (i = 0; i < size; i++) {
			if (pwm.volt_table[i] >= min_uV)
				break;
		}
		vol =  pwm.volt_table[i];

		/* VDD12 = 1.40 - 0.455*D , pwm duty*/
		pwm_value = (pwm_reg_matches[i].max_uV - vol) /
			    ((pwm_reg_matches[num_matches].max_uV -
			     pwm_reg_matches[num_matches].min_uV) / 100);
		/*pwm_value %, coefficient *1000*/
	}
	if (pwm_regulator_set_rate(pwm_value, pwm_id) != 0) {
		printf("fail to set pwm rate,pwm_value=%d\n", pwm_value);
		return -1;
	}

	printf("set pwm voltage ok,pwm_id =%d vol=%d,pwm_value=%d\n",
	       pwm_id, min_uV, pwm_value);

	return 0;
}

static int pwm_regulator_parse_dt(const void *blob)
{
	int pwm_node[2], pwm_nd[2];
	int ret = 0, i, length;
	u32 pwm0_data[4];
	int pwm_count = 0, pwm_id[2];
	int pwm_init_volt[2];

	pwm_node[0] = fdt_node_offset_by_compatible(blob,
			0, COMPAT_ROCKCHIP_PWM);
	if (pwm_node[0] < 0) {
		pwm_node[0] = fdt_node_offset_by_compatible(blob,
			0, COMPAT_PWM_REGULATOR);
		if (pwm_node[0] < 0) {
			printf("can't find dts node for pwm0\n");
			return -ENODEV;
		}
	}
	pwm_count = 1;

	pwm_node[1] = fdt_node_offset_by_compatible(blob,
			pwm_node[0], COMPAT_ROCKCHIP_PWM);
	if (pwm_node[1] < 0) {
		pwm_node[1] = fdt_node_offset_by_compatible(blob,
			pwm_node[0], COMPAT_PWM_REGULATOR);
		if (pwm_node[1] < 0)
			printf("can't find dts node for pwm1\n");
		else
			pwm_count = 2;
	} else {
		pwm_count = 2;
	}

	if (fdtdec_get_int_array(blob, pwm_node[0], "pwms", pwm0_data,
			ARRAY_SIZE(pwm0_data))) {
		debug("Cannot decode PWM%d property pwms\n", pwm0_data[1]);
		return -ENODEV;
	}
	pwm.period = pwm0_data[2];
	pwm.polarity = pwm0_data[3];

	fdt_getprop(blob, pwm_node[0], "rockchip,pwm_voltage_map", &length);
	pwm.pwm_vol_map_count = length / sizeof(u32);
	pwm.volt_table = malloc(length);

	if (fdtdec_get_int_array(blob, pwm_node[0], "rockchip,pwm_voltage_map",
			(u32 *)pwm.volt_table, pwm.pwm_vol_map_count)) {
		debug("Cannot decode PWM property pwms\n");
		goto pwm_regulator;
	}

	for (i = 0 ; i < pwm_count; i++) {
		pwm_id[i] = fdtdec_get_int(blob, pwm_node[i], "rockchip,pwm_id", -1);
		if (pwm_id[i] < 0) {
			printf("Cannot find regulator pwm id\n");
			continue;
		}
		pwm_init(pwm_id[i], 0, pwm.polarity);
		pwm_nd[i] = fdt_get_regulator_node(blob, pwm_node[i]);
		pwm_init_volt[i] = fdtdec_get_int(blob, pwm_node[i], "rockchip,pwm_voltage", 0);
		pwm_coefficient[i] = fdtdec_get_int(blob, pwm_node[i], "rockchip,pwm_coefficient", 0);
		pwm_max_volt[i] = fdtdec_get_int(blob, pwm_node[i], "rockchip,pwm_max_voltage", 0);
		if (pwm_nd[i] < 0)
			printf("Cannot find regulators\n");
		else
			fdt_regulator_match(blob, pwm_nd[i], pwm_reg_matches, PWM_NUM_REGULATORS);

		if (pwm_init_volt[i] == 0)
			pwm_init_volt[i] = pwm_reg_matches[i].init_uV;
		regulator_init_pwm_matches[i].name = pwm_reg_matches[i].name;
		if (pwm_reg_matches[i].boot_on)
			ret = pwm_regulator_set_voltage(pwm_id[i], i, pwm_init_volt[i], pwm_init_volt[i]);
		if (pwm_reg_matches[i].boot_on && (pwm_reg_matches[i].min_uV == pwm_reg_matches[i].max_uV))
			ret = pwm_regulator_set_voltage(pwm_id[i], i, pwm_reg_matches[i].min_uV, pwm_reg_matches[i].max_uV);
	}
	return ret;

pwm_regulator:
	for (i = 0 ; i < pwm_count; i++) {
		pwm_id[i] = fdtdec_get_int(blob, pwm_node[i],
					   "rockchip,pwm_id", -1);
		if (pwm_id[i] < 0) {
			printf("Cannot find regulator pwm id\n");
			continue;
		}
		pwm_init(pwm_id[i], 0, pwm.polarity);
		pwm_init_volt[i] = fdtdec_get_int(blob,
						  pwm_node[i],
						  "rockchip,pwm_voltage",
						  0);
		fdt_get_regulator_init_data(blob,
					    pwm_node[i],
					    &pwm_reg_matches[i]);
		if (pwm_init_volt[i] == 0)
			pwm_init_volt[i] = pwm_reg_matches[i].init_uV;
		regulator_init_pwm_matches[i].name = pwm_reg_matches[i].name;
		if (pwm_reg_matches[i].boot_on)
			ret = pwm_regulator_set_voltage(pwm_id[i], i,
							pwm_init_volt[i],
							pwm_init_volt[i]);
		if (pwm_reg_matches[i].boot_on &&
		    (pwm_reg_matches[i].min_uV == pwm_reg_matches[i].max_uV))
			ret = pwm_regulator_set_voltage(pwm_id[i], i,
						pwm_reg_matches[i].min_uV,
						pwm_reg_matches[i].max_uV);
	}
	return ret;
}


int pwm_regulator_init(void)
{
	int ret;

	if (!gd->fdt_blob)
		return -1;

	ret = pwm_regulator_parse_dt(gd->fdt_blob);
	if (ret < 0)
		return ret;

	return 0;
}



