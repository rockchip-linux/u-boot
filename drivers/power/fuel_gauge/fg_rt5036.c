/*
 *  Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *  zhangqing< zhangqing@rock-chips.com >
 *  for battery driver sample
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <power/battery.h>
#include <power/rt5036_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>

/*#define CONFIG_RT_SUPPORT_ACUSB_DUALIN*/
/*#define CONFIG_RT_SUPPORT_4V35_BAT*/

#define PMU_DEBUG 0

#define VOLTAGE_1                       0xEB
#define VOLTAGE_0                       0xEC

int rt5036_state_of_chrg = 0;
int rt5036_chrg_done = 0;

struct rt5036_fg {
	struct pmic *p;
};

struct rt5036_fg rt5036_fg;

int rt5036_charger_done_det(struct pmic *pmic)
{
	u8 chrg_done;
	
	i2c_set_bus_num(pmic->bus);
	chrg_done = i2c_reg_read(pmic->hw.i2c.addr, 0x16);
	if ((chrg_done & 0x30) == 0x20)
		return 1;
	else 
		return 0;
}

static int rt5036_charger_setting(struct pmic *pmic,int current)
{
	u8 iset1_val;
	const u8 dc_iset1_cfg = 0xe0;
	const u8 usb_iset1_cfg = 0x40;
	
	i2c_set_bus_num(pmic->bus);
	iset1_val = i2c_reg_read(pmic->hw.i2c.addr, RT5036_REG_CHGCTL1);
	
	if (current == 1) {
		if ((iset1_val & 0xe0) == usb_iset1_cfg)
			return 0;
	}
	
	switch (current){
	case 2:
			i2c_reg_write(pmic->hw.i2c.addr, RT5036_REG_CHGCTL1,
				i2c_reg_read(pmic->hw.i2c.addr, RT5036_REG_CHGCTL1) | dc_iset1_cfg);
			break;
	case 1:
			i2c_reg_write(pmic->hw.i2c.addr, RT5036_REG_CHGCTL1,
				(i2c_reg_read(pmic->hw.i2c.addr, RT5036_REG_CHGCTL1) & 0x1f) | usb_iset1_cfg);
			break;
	default:
			break;
		}

	rt5036_chrg_done = rt5036_charger_done_det(pmic);
	
	debug("use usb chg %s iset1:0x%02x current %d chrg_done = %d\n",__func__,
					i2c_reg_read(pmic->hw.i2c.addr, RT5036_REG_CHGCTL1),  current, rt5036_chrg_done);
					
	

	return 0;
}


static int rt5036_chrg_det(struct pmic *pmic)
{
	u8 val;
	unsigned int chrg_state = 0;
	val = i2c_reg_read(pmic->hw.i2c.addr, 0x17);
	if ((val & 0x80) == 0x80) {
		#if defined (CONFIG_RT_SUPPORT_ACUSB_DUALIN)
			chrg_state = 2;
		else
			chrg_state = dwc_otg_check_dpdm();
		#endif
	} else {
			chrg_state = 0;
	}

	return chrg_state;
}

static int rt5036_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = rt5036_chrg_det(bat);
	return 0;
}


/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/

static int rt5036_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	
	rt5036_state_of_chrg = rt5036_chrg_det(bat);
	i2c_set_bus_num(bat->bus);
	i2c_init(RT5036_I2C_SPEED, bat->hw.i2c.addr);
	rt5036_charger_setting(bat,rt5036_state_of_chrg);
	battery->state_of_chrg = rt5036_state_of_chrg;
	return 0;

}

static struct power_fg fg_ops = {
	.fg_battery_check = rt5036_check_battery,
	.fg_battery_update = rt5036_update_battery,
};

int fg_rt5036_init(unsigned char bus, uchar addr)
{
	static const char name[] = "RT5036_FG";
	if (!rt5036_fg.p)
		rt5036_fg.p = pmic_alloc();
	rt5036_fg.p->name = name;
	rt5036_fg.p->bus = bus;
	rt5036_fg.p->hw.i2c.addr = addr;
	rt5036_fg.p->interface = PMIC_I2C;
	rt5036_fg.p->fg = &fg_ops;
	rt5036_fg.p->pbat = calloc(sizeof(struct  power_battery), 1);
	i2c_set_bus_num(bus);
	i2c_init(RT5036_I2C_SPEED,addr);
	return 0;
}

