/*
 *  Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *  zyw < zyw@rock-chips.com >
 *  for battery driver sample
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <power/battery.h>
#include <power/rt5025_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>

/*#define CONFIG_RT_SUPPORT_ACUSB_DUALIN*/
/*#define CONFIG_RT_SUPPORT_4V35_BAT*/

#define PMU_DEBUG 0

#define VOLTAGE_1                       0xEB
#define VOLTAGE_0                       0xEC

#define PMU_I2C_ADDRESS      0x35

int rt5025_state_of_chrg = 0;

struct rt5025_fg {
	struct pmic *p;
};

struct rt5025_fg rt5025_fg;

static int rt5025_volt_tab[6] = {3466, 3586, 3670, 3804, 4014, 4316};

// the unit of voltage value is mV .
int rt5025_get_vbat_voltage(struct pmic *pmic)
{
	u8 voltage_h,voltage_l;
	int vol=0,vol_tmp1,vol_tmp2;
	u8 i;
/*
	u8 pswr = i2c_reg_read(pmic->hw.i2c.addr, PSWR_REG);
	if (!(pswr & 0x7f))
		return 3780;
*/	
	for(i=1;i<11;i++)
	{
		voltage_l = i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_VBATSL);
		voltage_h = i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_VBATSH);
		vol_tmp1=voltage_l;
		vol_tmp2=voltage_h;
		vol +=(vol_tmp2 * 256 + vol_tmp1) * 610 / 1000;
	}
	vol=vol / 10;

	debug("###rt5025_get_vbat_voltage### vol = %d\n",vol);
	
	return vol;
}

int rt5025_get_capcity(int volt)
{
	int i = 0;
	int level0, level1;
	int cap;
	int diff = 0;
	u8 chgstate;
	int step = 100 / (ARRAY_SIZE(rt5025_volt_tab) -1);

	if (rt5025_state_of_chrg == 1)
		diff = 70;
	else if (rt5025_state_of_chrg == 2)
		diff = 270;

	for (i = 0 ; i < ARRAY_SIZE(rt5025_volt_tab); i++) {
		if (volt <= (rt5025_volt_tab[i] + diff))
			break;
	}

	if (i == 0) 
		return 0;
	
	level0 = rt5025_volt_tab[i -1] + diff;
	level1 = rt5025_volt_tab[i] + diff;

	cap = step * (i-1) + step * (volt - level0)/(level1 - level0);
	
	chgstate = i2c_reg_read(PMU_I2C_ADDRESS, RT5025_REG_CHGCTL1);
	if (((chgstate & 0x03) != 0) &&((chgstate & 0x20) == 0x00)){
		printf("%s chg complete\n",__func__);
		cap = 100;
	}
	debug("cap%d step:%d level0 %d level1 %d  diff %d\n",
			cap, step, level0 ,level1, diff);
	return cap;
}


/*
0. disable charging  
1. usb charging
2. ac adapter charging
*/

static int rt5025_charger_setting(struct pmic *pmic,int current)
{
	u8 iset1_val;
	const u8 dc_iset1_cfg = 0x6e;
	const u8 usb_iset1_cfg = 0x02;
	
	i2c_set_bus_num(pmic->bus);
	iset1_val = i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4);
	
	if (current == 1) {
		if ((iset1_val & 0x81) == usb_iset1_cfg)
			return 0;
	}
	
	switch (current){
	case 2:
			i2c_reg_write(pmic->hw.i2c.addr, RT5025_REG_CHGCTL2,
				i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL2) | 0x04);
			i2c_reg_write(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4,
				(i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4) & 0x81) | dc_iset1_cfg);
			break;
	case 1:
			i2c_reg_write(pmic->hw.i2c.addr, RT5025_REG_CHGCTL2,
				i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL2) | 0x04);
			i2c_reg_write(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4,
				(i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4) & 0x81) | usb_iset1_cfg);
			break;
	default:
			i2c_reg_write(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4,
				(i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4) & 0x81) | usb_iset1_cfg);
			break;
		}
	debug("use usb chg %s iset1:0x%02x chgctl1:0x%02x current %d\n",__func__,
					i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL4), i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL1), current);

	i2c_reg_write(pmic->hw.i2c.addr, RT5025_REG_CHGCTL7,
			i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL7) | 0x10);

	return 0;
}


static int rt5025_chrg_det(struct pmic *pmic)
{
	u8 val;
	unsigned int chrg_state = 0;
	val = i2c_reg_read(pmic->hw.i2c.addr, RT5025_REG_CHGCTL1);
	if (val & 0x01) { 
		chrg_state = dwc_otg_check_dpdm();
	} else if (val & 0x02){
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

static int rt5025_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = rt5025_chrg_det(bat);
	return 0;
}


/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/

static int rt5025_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	
	rt5025_state_of_chrg = rt5025_chrg_det(bat);
	i2c_set_bus_num(bat->bus);
	i2c_init(RT5025_I2C_SPEED, bat->hw.i2c.addr);
	rt5025_charger_setting(bat,rt5025_state_of_chrg);
	battery->voltage_uV = rt5025_get_vbat_voltage(bat);
	battery->capacity = rt5025_get_capcity(battery->voltage_uV);
	battery->state_of_chrg = rt5025_state_of_chrg;
	return 0;

}

static struct power_fg fg_ops = {
	.fg_battery_check = rt5025_check_battery,
	.fg_battery_update = rt5025_update_battery,
};

int fg_rt5025_init(unsigned char bus, uchar addr)
{
	static const char name[] = "RT5025_FG";
	if (!rt5025_fg.p)
		rt5025_fg.p = pmic_alloc();
	rt5025_fg.p->name = name;
	rt5025_fg.p->bus = bus;
	rt5025_fg.p->hw.i2c.addr = addr;
	rt5025_fg.p->interface = PMIC_I2C;
	rt5025_fg.p->fg = &fg_ops;
	rt5025_fg.p->pbat = calloc(sizeof(struct  power_battery), 1);
	i2c_set_bus_num(bus);
	i2c_init(RT5025_I2C_SPEED,addr);
	i2c_reg_write(addr, RT5025_REG_CHANNELL,
			i2c_reg_read(addr, RT5025_REG_CHANNELL) | 0x03);
	#if defined (CONFIG_RT_SUPPORT_4V35_BAT)
	i2c_reg_write(addr, RT5025_REG_CHGCTL3,
			(i2c_reg_read(addr, RT5025_REG_CHGCTL3) & 0x03) | 0xa8);
	#endif

	return 0;
}

