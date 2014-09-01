/*
 *  Copyright (C) 2013 rockchips
 *  zyw < zyw@rock-chips.com >
 *  for battery driver sample
 */

#include <common.h>
#include <malloc.h>
#include <power/battery.h>
#include <power/rk818_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>

/*#define SUPPORT_USB_CONNECT_TO_ADP*/

#define PMU_DEBUG 0

#define VOLTAGE_1                       0xEB
#define VOLTAGE_0                       0xEC


#define RICOH619_TAH_SEL2				5
#define RICOH619_TAL_SEL2				6

#define PMU_I2C_ADDRESS      0x1b

int rk818_state_of_chrg = 0;
int voltage_k = 0; 
int voltage_b = 0; 

struct rk818_fg {
	struct pmic *p;
};

struct rk818_fg rk818_fg;

static int rk818_volt_tab[6] = {3466, 3586, 3670, 3804, 4014, 4316};
/*
int pmu_debug(u8 reg)
{
	u8 reg_val;
		reg_val=i2c_reg_read(PMU_I2C_ADDRESS,reg);
		if(PMU_DEBUG)
		{
			printf("pmu_i2c_init read reg_addr 0x%x : 0x%x\n",reg , reg_val);
		}
	return reg_val;
}

int pmu_i2c_set(u8 addr, u8 reg,u8 reg_val)
{
	u8 val_tmp;
	val_tmp=i2c_reg_read(addr,reg);
	val_tmp=val_tmp|reg_val;
	i2c_reg_write(addr,reg,val_tmp);
	return 0;
}

int read_pmu_reg(struct pmic *pmic,u8 reg,u8 *reg_val)
{
	*reg_val=i2c_reg_read(pmic->hw.i2c.addr, reg);
	return 0;
}

int write_pmu_reg(struct pmic *pmic,u8 reg,u8 reg_val)
{
	pmu_i2c_set(pmic->hw.i2c.addr, reg, reg_val);
	return 0;
}
*/
// the unit of voltage value is mV .
int rk818_get_vbat_voltage(struct pmic *pmic)
{
	int voltage=0,voltage_now=0,temp;
	u8 vol_tmp1,vol_tmp2;
	u8 i;

	u8 pswr = i2c_reg_read(pmic->hw.i2c.addr, GGSTS);
	if (!(pswr & 0x60))
		return 3780;
		
	for(i=1;i<11;i++)
	{
		vol_tmp1 =  i2c_reg_read(pmic->hw.i2c.addr, BAT_VOL_REGL); 
		temp = vol_tmp1;
		vol_tmp2 =  i2c_reg_read(pmic->hw.i2c.addr, BAT_VOL_REGH); 
		temp |= vol_tmp2<<8;
		voltage_now = voltage_k*temp + voltage_b;
		voltage += voltage_now;
	}
	voltage=voltage/10;
	debug("###rk818_get_vbat_voltage### voltage = %d\n",voltage);
	return voltage;
}

int rk818_get_capcity(int volt)
{
	int i = 0;
	int level0, level1;
	int cap;
	int diff = 0;
	u8 chgstate;
	int step = 100 / (ARRAY_SIZE(rk818_volt_tab) -1);

	if (rk818_state_of_chrg == 1)
		diff = 70;
	else if (rk818_state_of_chrg == 2)
		diff = 270;

	for (i = 0 ; i < ARRAY_SIZE(rk818_volt_tab); i++) {
		if (volt <= (rk818_volt_tab[i] + diff))
			break;
	}

	if (i == 0) 
		return 0;
	
	level0 = rk818_volt_tab[i -1] + diff; 
	level1 = rk818_volt_tab[i] + diff;

	cap = step * (i-1) + step *(volt - level0)/(level1 - level0);
	debug("cap%d step:%d level0 %d level1 %d  diff %d\n",
			cap, step, level0 ,level1, diff);
	
	chgstate = i2c_reg_read(PMU_I2C_ADDRESS, 0xa0);
	if ((chgstate & 0x70) == 0x40){
		printf("%s chg complete\n",__func__);
		cap = 100;
	}

	return cap;
}


/*
0. disable charging  
1. usb charging
2. ac adapter charging
*/
extern int support_dc_chg;

static int rk818_charger_setting(struct pmic *pmic,int current)
{
	u8 iset1_val,chgiset_val;
	const u8 dc_iset1_cfg = 0x09;
	const u8 usb_iset1_cfg = 0x0;
	const u8 dc_chgiset_cfg = 0x05;
	const u8 usb_chgiset_cfg = 0x0;
	
	i2c_set_bus_num(pmic->bus);
	iset1_val = i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG);
	chgiset_val = i2c_reg_read(pmic->hw.i2c.addr, CHRG_CTRL_REG1);
	
	if ( support_dc_chg || (current == 2)) {
		if (((iset1_val & 0x0f) == dc_iset1_cfg) && ((chgiset_val & 0x0f) == dc_chgiset_cfg))
			return 0;
	}else if (current == 1) {
		if (((iset1_val & 0x0f) == usb_iset1_cfg) && ((chgiset_val & 0x0f) == usb_chgiset_cfg))
			return 0;
	}
	
	if (support_dc_chg){
		i2c_reg_write(pmic->hw.i2c.addr, USB_CTRL_REG,
				(i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG) & 0xf0) | dc_iset1_cfg);
		i2c_reg_write(pmic->hw.i2c.addr, CHRG_CTRL_REG1,
				(i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG) & 0xf0) | dc_chgiset_cfg);
		printf("use dc chg %s iset1:0x%02x chgiset:0x%02x current %d\n",__func__,
					i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG), i2c_reg_read(pmic->hw.i2c.addr, CHRG_CTRL_REG1),current);


	}else{
		switch (current){
		case 2:
			i2c_reg_write(pmic->hw.i2c.addr, USB_CTRL_REG,
				(i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG) & 0xf0) | dc_iset1_cfg);
			i2c_reg_write(pmic->hw.i2c.addr, CHRG_CTRL_REG1,
				(i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG) & 0xf0) | dc_chgiset_cfg);
			break;
		default:
			i2c_reg_write(pmic->hw.i2c.addr, USB_CTRL_REG,
				i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG) & 0xf0);
			i2c_reg_write(pmic->hw.i2c.addr, CHRG_CTRL_REG1,
				i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG) & 0xf0);
			break;
		}
		printf("use usb chg %s iset1:0x%02x chgiset:0x%02x current %d\n",__func__,
					i2c_reg_read(pmic->hw.i2c.addr, USB_CTRL_REG), i2c_reg_read(pmic->hw.i2c.addr, CHRG_CTRL_REG1),current);
	}
	i2c_reg_write(pmic->hw.i2c.addr, CHRG_CTRL_REG1,
			i2c_reg_read(pmic->hw.i2c.addr, CHRG_CTRL_REG1) | 0x80);

	return 0;
}


static int rk818_chrg_det(struct pmic *pmic)
{
	u8 val;
	unsigned int chrg_state = 0;
	val = i2c_reg_read(pmic->hw.i2c.addr, RK818_VB_MON_REG);
	if (val & 0x40) { 
		chrg_state = dwc_otg_check_dpdm();
	} else {
		chrg_state = 0;
	}

	return chrg_state;
}

static int rk818_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = rk818_chrg_det(bat);
	return 0;
}


/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/

static int rk818_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	
	rk818_state_of_chrg = rk818_chrg_det(bat);
	i2c_set_bus_num(bat->bus);
	i2c_init(RK818_I2C_SPEED, bat->hw.i2c.addr);
	rk818_charger_setting(bat,rk818_state_of_chrg);
	battery->voltage_uV = rk818_get_vbat_voltage(bat);
	battery->capacity = rk818_get_capcity(battery->voltage_uV);
	battery->state_of_chrg = rk818_state_of_chrg;
	return 0;

}

static struct power_fg fg_ops = {
	.fg_battery_check = rk818_check_battery,
	.fg_battery_update = rk818_update_battery,
};

static void get_voltage_offset_value(void)
{
	int vcalib0,vcalib1;
	u8 val;

	val = i2c_reg_read(PMU_I2C_ADDRESS, VCALIB0_REGL);
	vcalib0 = val;
	val = i2c_reg_read(PMU_I2C_ADDRESS, VCALIB0_REGH);
	vcalib0 |= val<<8;

	val = i2c_reg_read(PMU_I2C_ADDRESS, VCALIB1_REGL);
	vcalib1 = val;
	val = i2c_reg_read(PMU_I2C_ADDRESS, VCALIB1_REGH);
	vcalib1 |= val<<8;

	voltage_k = (4200 - 3000)/(vcalib1 - vcalib0);
	voltage_b = 4200 - voltage_k*vcalib1;
}

int fg_rk818_init(unsigned char bus,uchar addr)
{
	static const char name[] = "RK818_FG";
	if (!rk818_fg.p)
		rk818_fg.p = pmic_alloc();
	rk818_fg.p->name = name;
	rk818_fg.p->bus = bus;
	rk818_fg.p->hw.i2c.addr = addr;
	rk818_fg.p->interface = PMIC_I2C;
	rk818_fg.p->fg = &fg_ops;
	rk818_fg.p->pbat = calloc(sizeof(struct  power_battery), 1);
	i2c_set_bus_num(bus);
	i2c_init(RK818_I2C_SPEED,addr);
	get_voltage_offset_value();
	return 0;
}

