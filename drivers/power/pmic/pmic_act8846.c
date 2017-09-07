/*
 * Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * zyw < zyw@rock-chips.com >
 * yxj <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*#define DEBUG*/
#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <power/rockchip_power.h>
#include <power/act8846_pmic.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_act8846 act8846;

struct fdt_regulator_match act8846_reg_matches[] = {
	{ .prop = "act_dcdc1" ,.min_uV = 1200000, .max_uV = 1200000, .boot_on =1},
	{ .prop = "act_dcdc2" ,.min_uV = 3300000, .max_uV = 3300000, .boot_on = 1},
	{ .prop = "act_dcdc3", .min_uV = 700000, .max_uV = 1500000, .boot_on = 1},
	{ .prop = "act_dcdc4", .min_uV = 2000000, .max_uV = 2000000, .boot_on = 1},
	{ .prop = "act_ldo1", .min_uV = 3300000, .max_uV = 3300000, .boot_on = 1 },
	{ .prop = "act_ldo2", .min_uV = 1000000, .max_uV = 1000000, .boot_on = 1 },
	{ .prop = "act_ldo3", .min_uV = 3300000, .max_uV = 3300000, .boot_on = 1 },
	{ .prop = "act_ldo4", .min_uV = 3300000, .max_uV = 3300000, .boot_on = 1 },
	{ .prop = "act_ldo5", .min_uV = 3300000, .max_uV = 3300000, .boot_on = 1 },
	{ .prop = "act_ldo6", .min_uV = 1000000, .max_uV = 1000000, .boot_on = 1 },
	{ .prop = "act_ldo7", .min_uV = 1800000, .max_uV = 1800000, .boot_on = 1 },
	{ .prop = "act_ldo8", .min_uV = 1800000, .max_uV = 1800000, .boot_on = 1 },
};


const static int voltage_map[] = {
	600, 625, 650, 675, 700, 725, 750, 775,
	800, 825, 850, 875, 900, 925, 950, 975,
	1000, 1025, 1050, 1075, 1100, 1125, 1150,
	1175, 1200, 1250, 1300, 1350, 1400, 1450,
	1500, 1550, 1600, 1650, 1700, 1750, 1800,
	1850, 1900, 1950, 2000, 2050, 2100, 2150,
	2200, 2250, 2300, 2350, 2400, 2500, 2600,
	2700, 2800, 2900, 3000, 3100, 3200, 3300,
	3400, 3500, 3600, 3700, 3800, 3900,
};


static struct act8846_reg_table act8846_regulators[] = {
	{
		.name		= "act_dcdc1",   //ddr
		.reg_ctl	= act8846_BUCK1_CONTR_BASE,
		.reg_vol	= act8846_BUCK1_SET_VOL_BASE,
	},
	{
		.name		= "act_dcdc2",    //logic
		.reg_ctl	= act8846_BUCK2_CONTR_BASE,
		.reg_vol	= act8846_BUCK2_SET_VOL_BASE,
	},
	{
		.name		= "act_dcdc3",   //arm
		.reg_ctl	= act8846_BUCK3_CONTR_BASE,
		.reg_vol	= act8846_BUCK3_SET_VOL_BASE,
	},
	{
		.name		= "act_dcdc4",   //vccio
		.reg_ctl	= act8846_BUCK4_CONTR_BASE,
		.reg_vol	= act8846_BUCK4_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo1",   //vdd11
		.reg_ctl	= act8846_LDO1_CONTR_BASE,
		.reg_vol	= act8846_LDO1_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo2",  //vdd12 
		.reg_ctl	= act8846_LDO2_CONTR_BASE,
		.reg_vol	= act8846_LDO2_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo3",   //vcc18_cif
		.reg_ctl	= act8846_LDO3_CONTR_BASE,
		.reg_vol	= act8846_LDO3_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo4",   //vcca33
		.reg_ctl	= act8846_LDO4_CONTR_BASE,
		.reg_vol	= act8846_LDO4_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo5",   //vcctp
		.reg_ctl	= act8846_LDO5_CONTR_BASE,
		.reg_vol	= act8846_LDO5_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo6",   //vcc33
		.reg_ctl	= act8846_LDO6_CONTR_BASE,
		.reg_vol	= act8846_LDO6_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo7",   //vccio_wl
		.reg_ctl	= act8846_LDO7_CONTR_BASE,
		.reg_vol	= act8846_LDO7_SET_VOL_BASE,
	},
	{
		.name		= "act_ldo8",   //vcc28_cif
		.reg_ctl	= act8846_LDO8_CONTR_BASE,
		.reg_vol	= act8846_LDO8_SET_VOL_BASE,
	},
 };



static int act8846_set_bits(uint reg_addr, uchar  mask, uchar val)
{
	uchar tmp = 0;
	int ret =0;

	ret = I2C_READ(reg_addr,&tmp);
	if (ret == 0){
		tmp = (tmp & ~mask) | val;
		ret = I2C_WRITE(reg_addr,&tmp);
	}
	return 0;	
}




/*
set charge current
0. disable charging  
1. usb charging, 500mA
2. ac adapter charging, 1.5A
*/
int pmic_act8846_charger_setting(int current)
{

    printf("%s %d\n",__func__,current);
    switch (current){
    case 0:
        //disable charging
        break;
    case 1:
        //set charger current to 500ma
        break;
    case 2:
         //set charger current to 1.5A
        break;
    default:
        break;
    }
    return 0;
}


static int check_volt_table(const int *volt_table,int volt)
{
	int i=0;

	for(i=VOL_MIN_IDX;i<VOL_MAX_IDX;i++){
		if(volt <= (volt_table[i]*1000))
			return i;
	}
	return -1;
}

int act8846_regulator_set(void)
{
	int i;
	int volt;
	uchar reg_val;
	struct fdt_regulator_match *match;
	struct act8846_reg_table *regulator;
	for (i = 0; i < ACT8846_NUM_REGULATORS; i++) {
		match = &act8846_reg_matches[i];
		if (match->boot_on && (match->min_uV == match->max_uV)) {
			volt = match->min_uV;
			reg_val = check_volt_table(voltage_map, volt);
			regulator = &act8846_regulators[i];
			if ((volt == 0) || (reg_val == -1)) {
				printf("invalid volt = %d or reg_val = %d\n", volt, reg_val);
				continue;
			}
			act8846_set_bits(regulator->reg_vol,LDO_VOL_MASK,reg_val);
			act8846_set_bits(regulator->reg_ctl,LDO_EN_MASK,LDO_EN_MASK);
			debug("set %s--%s volt:%d\n", match->prop, match->name, volt);
		}
	}

	return 0;
}

static int act8846_i2c_probe(u32 bus ,u32 addr)
{
	char val;
	int ret;
	i2c_set_bus_num(bus);
	i2c_init(ACT8846_I2C_SPEED, 0);
	ret = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, 0x22);
	if (val == 0xff)
		return -ENODEV;
	else
		return 0;
	
	
}

static int act8846_parse_dt(const void* blob)
{
	int node, nd, act8846_node, semi_act8846_node;
	struct fdt_gpio_state gpios[2];
	u32 bus, addr;
	int ret, i;

	act8846_node = fdt_node_offset_by_compatible(blob,
					g_i2c_node, COMPAT_ACTIVE_ACT8846);
	semi_act8846_node = fdt_node_offset_by_compatible(blob,
					g_i2c_node, COMPAT_ACTIVE_SEMI_ACT8846);

	if (act8846_node > 0)
		node = act8846_node;
	else if (semi_act8846_node > 0)
		node = semi_act8846_node;
	else
		node = -1;

	if (node < 0) {
		debug("can't find dts node for act8846\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, node)) {
		debug("device act8846 is disabled\n");
		return -1;
	}

	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("pmic act8846 get fdt i2c failed\n");
		return ret;
	}

	ret = act8846_i2c_probe(bus, addr);
	if (ret < 0) {
		debug("pmic act8846 i2c probe failed\n");
		return ret;
	}
	
	nd = fdt_get_regulator_node(blob, node);
	if (nd < 0)
		printf("%s: Cannot find regulators\n", __func__);
	else
		fdt_regulator_match(blob, nd, act8846_reg_matches,
					ACT8846_NUM_REGULATORS);

	for (i = 0; i < ACT8846_NUM_REGULATORS; i++)
		regulator_init_pmic_matches[i].name = act8846_reg_matches[i].name;

	fdtdec_decode_gpios(blob, node, "gpios", gpios, 2);

	act8846.pmic = pmic_alloc();
	act8846.node = node;
	act8846.pmic->hw.i2c.addr = addr;
	act8846.pmic->bus = bus;
	act8846.pwr_hold.gpio = gpios[1].gpio;
	act8846.pwr_hold.flags = !(gpios[1].flags  & OF_GPIO_ACTIVE_LOW);

	return 0;
}


int pmic_act8846_init(unsigned char bus)
{
	int ret;
	if (!act8846.pmic) {
		if (!gd->fdt_blob)
			return -1;

		ret = act8846_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}
	gpio_direction_output(act8846.pwr_hold.gpio,
			act8846.pwr_hold.flags); /*power hold*/
	i2c_set_bus_num(act8846.pmic->bus);
	i2c_init(ACT8846_I2C_SPEED, 0);
	act8846_regulator_set( );

	return 0;
}

void pmic_act8846_shut_down(void)
{
	gpio_direction_output(act8846.pwr_hold.gpio,
			!(act8846.pwr_hold.flags)); 
	mdelay(100);
}

