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
#include <power/ricoh619_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>

/*#define SUPPORT_USB_CONNECT_TO_ADP*/

#define PMU_DEBUG 0

#define VOLTAGE_1                       0xEB
#define VOLTAGE_0                       0xEC


#define RICOH619_TAH_SEL2				5
#define RICOH619_TAL_SEL2				6

#define PMU_I2C_ADDRESS      0x32

int state_of_chrg = 0;

struct ricoh619_fg {
	struct pmic *p;
};

struct ricoh619_fg ricoh_fg;

static int volt_tab[6] = {3466, 3586, 3670, 3804, 4014, 4316};
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

/*int pmu_i2c_init(void)
{
	i2c_set_bus_num(CONFIG_PMIC_I2C_BUS);
	i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);

	pmu_i2c_set(PMU_I2C_ADDRESS, 0x66, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x64, 0x02);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x65, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x88, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x89, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x66, 0x20);

	return 1;
}*/

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

#if 0
static int get_check_fuel_gauge_reg(int Reg_h, int Reg_l, int enable_bit)
{
	uint8_t get_data_h, get_data_l;
	int old_data, current_data;
	int i;
	int ret = 0;

	old_data = 0;

	for (i = 0; i < 5 ; i++) {
		ret = read_pmu_reg(Reg_h, &get_data_h);
		if (ret < 0) {
			printf("Error in reading the control register\n");
			return ret;
		}

		ret = read_pmu_reg(Reg_l, &get_data_l);
		if (ret < 0) {
			printf("Error in reading the control register\n");
			return ret;
		}

		current_data = ((get_data_h & 0xff) << 8) | (get_data_l & 0xff);
		current_data = (current_data & enable_bit);

		if (current_data == old_data)
			return current_data;
		else
			old_data = current_data;
	}

	return current_data;
}

static int get_battery_temp(void)
{
	int ret = 0;

	int sign_bit=0;

	ret = get_check_fuel_gauge_reg(TEMP_1_REG, TEMP_2_REG, 0x0fff);
	if (ret < 0) {
		printf("Error in reading the fuel gauge control register\n");
		return ret;
	}

	/* bit3 of 0xED(TEMP_1) is sign_bit */
	sign_bit = ((ret & 0x0800) >> 11);

	ret = (ret & 0x07ff);

		if (sign_bit == 0)	/* positive value part */
			/* conversion unit */
			/* 1 unit is 0.0625 degree and retun unit
			 * should be 0.1 degree,
			 */
			ret = ret * 625  / 1000;
	else {	/*negative value part */
		ret = (~ret + 1) & 0x7ff;
		ret = -1 * ret * 625 / 1000;
	}

    //printk("%s: %d\n",__func__,ret);
	return ret;
}
#endif

// the unit of voltage value is mV .
int ricoh619_get_voltage(struct pmic *pmic)
{
	u8 voltage_h,voltage_l;
	int vol=0,vol_tmp1,vol_tmp2;
	u8 i,val, val2;

	u8 pswr = i2c_reg_read(pmic->hw.i2c.addr, PSWR_REG);
	if (!(pswr & 0x7f))
		return 3780;
	
	for(i=1;i<11;i++)
	{
		read_pmu_reg(pmic, VBATDATAL_REG,&voltage_l);
		read_pmu_reg(pmic,VBATDATAH_REG,&voltage_h);
		vol_tmp1=voltage_l;
		vol_tmp2=voltage_h;
		vol+=(vol_tmp1&0xF)|((vol_tmp2&0xFF)<<4);
	}
	vol=vol/10;
	vol=vol*5000/4095;
	val=i2c_reg_read(pmic->hw.i2c.addr, BATSET2_REG);
	if(vol > 4100&&(val!=0x44)){
		i2c_reg_write(pmic->hw.i2c.addr, CHGCTL1_REG,
			i2c_reg_read(pmic->hw.i2c.addr, CHGCTL1_REG) | 0x08);
		i2c_reg_write(pmic->hw.i2c.addr, BATSET2_REG, 0x44);  /* VFCHG 4.35v) */
		i2c_reg_write(pmic->hw.i2c.addr, CHGCTL1_REG,
			i2c_reg_read(pmic->hw.i2c.addr, CHGCTL1_REG) & 0xf7);
	}
	
	/***************DISABLE_CHARGER_TIMER************************/
	val = i2c_reg_read(pmic->hw.i2c.addr, CHGSTATE_REG);
	if ((vol > 4000) && (val & 0xc0)) {
		//printf("##########DISABLE_CHARGER_TIMER#############\n");
		val = i2c_reg_read(pmic->hw.i2c.addr, TIMSET_REG);
		val2 = val & 0x03;
		if (val2 == 0x02){
			i2c_reg_write(pmic->hw.i2c.addr, TIMSET_REG, val | 0x03);
		} else {
			i2c_reg_write(pmic->hw.i2c.addr, TIMSET_REG, val & 0xfe);
			i2c_reg_write(pmic->hw.i2c.addr, TIMSET_REG, val | 0x02);
			
		}
	}
	/****************************************************************/

	return vol;
}

#if 0
static int calc_capacity(void)
{
	uint8_t capacity;
	int temp;
	int ret = 0;
	int nt;
	int temperature=0,vol=0;

	temperature = get_battery_temp() / 10; /* unit 0.1 degree -> 1 degree */

	if (temperature >= 25) {
		nt = 0;
	} else if (temperature >= 5) {
		nt = (25 - temperature) * RICOH619_TAH_SEL2 * 625 / 100;
	} else {
		nt = (625  + (5 - temperature) * RICOH619_TAL_SEL2 * 625 / 100);
	}

	/* get remaining battery capacity from fuel gauge */
	ret = read_pmu_reg(SOC_REG, &capacity);
	if (ret < 0) {
		printf("Error in reading the control register\n");
		return ret;
	}
	
	
	if(capacity == 0)
	{
        vol = pmu_get_voltage();
        if(vol < 3597)
            temp = 9;
        else if(vol < 3625)
            temp = 19;
        else if(vol < 3654)
            temp = 29;    
        else if(vol < 3695)
            temp = 39;
        else if(vol < 3750)
            temp = 49;
        else if(vol < 3818)
            temp = 59;
        else if(vol < 3906)
            temp = 69;
        else if(vol < 3992)
            temp = 79;
        else if(vol < 4089)
            temp = 89;
        else if(vol < 4197)
            temp = 99;
        else temp = 100;
        printf("capacity reg is 0, capacity calc from vol = %d, vol = %d \n",temp,vol);
    }else{
        temp = capacity * 100 * 100 / (10000 - nt);
    }

	if(temp >= 2)temp = (temp-2)*100/98;
    else temp=1;
	return temp;		/* Unit is 1% */
}

#endif

int get_capcity(int volt)
{
	int i = 0;
	int level0, level1;
	int cap;
	int diff = 0;
	u8 chgstate;
	int step = 100 / (ARRAY_SIZE(volt_tab) -1);

	if (state_of_chrg == 1)
		diff = 70;
	else if (state_of_chrg == 2)
		diff = 270;

	for (i = 0 ; i < ARRAY_SIZE(volt_tab); i++) {
		if (volt <= (volt_tab[i] + diff))
			break;
	}

	if (i == 0) 
		return 0;
	
	level0 = volt_tab[i -1] + diff; 
	level1 = volt_tab[i] + diff;

	cap = step * (i-1) + step *(volt - level0)/(level1 - level0);
	/*printf("cap%d step:%d level0 %d level1 %d  diff %d\n",
			cap, step, level0 ,level1, diff);*/

	chgstate = i2c_reg_read(PMU_I2C_ADDRESS, CHGSTATE_REG);
	if ((chgstate & 0x3f) == 0x04){
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
static int ricoh619_charger_setting(struct pmic *pmic,int current)
{
	u8 iset1_val,chgiset_val;
	const u8 dc_iset1_cfg = 0x16;
	const u8 usb_iset1_cfg = 0x06;
	const u8 dc_chgiset_cfg = 0xd3;
	const u8 usb_chgiset_cfg = 0xc6;
	
	i2c_set_bus_num(pmic->bus);
	iset1_val = i2c_reg_read(pmic->hw.i2c.addr, REGISET1_REG);
	chgiset_val = i2c_reg_read(pmic->hw.i2c.addr, CHGISET_REG);
	
	if ( current == 1) {
		if ((iset1_val == usb_iset1_cfg) && (chgiset_val == usb_chgiset_cfg))
			return 0;
	} else if (current == 2) {
		if ((iset1_val == dc_iset1_cfg) && (chgiset_val == dc_chgiset_cfg))
			return 0;
	}
	i2c_reg_write(pmic->hw.i2c.addr, PWRFUNC,
			i2c_reg_read(pmic->hw.i2c.addr,0x0d)|0x20);                                            
	printf("%s iset1:0x%02x chgiset:0x%02x current %d\n",__func__,
					iset1_val, chgiset_val,current);
	switch (current){
	case 0:
//		i2c_reg_write(pmic->hw.i2c.addr, CHGCTL1_REG,
//			i2c_reg_read(pmic->hw.i2c.addr, 0xb3)&~0x03);      //disable charging
		break;
	case 1:
		i2c_reg_write(pmic->hw.i2c.addr, REGISET1_REG, usb_iset1_cfg);
		i2c_reg_write(pmic->hw.i2c.addr, REGISET2_REG, usb_iset1_cfg);
		i2c_reg_write(pmic->hw.i2c.addr, CHGISET_REG, usb_chgiset_cfg);	
		break;
	case 2:
		i2c_reg_write(pmic->hw.i2c.addr, REGISET1_REG, dc_iset1_cfg);
		i2c_reg_write(pmic->hw.i2c.addr, REGISET2_REG, dc_iset1_cfg);
		i2c_reg_write(pmic->hw.i2c.addr, CHGISET_REG, dc_chgiset_cfg);	/* ILIM_ADP	0x11= 0x0-0x1D (100mA - 3000mA) */
		break;
	default:
		break;
	}
	i2c_reg_write(pmic->hw.i2c.addr, CHGCTL1_REG,
			i2c_reg_read(pmic->hw.i2c.addr, CHGCTL1_REG) & 0xf7);
	return 0;
}


static int ricoh619_chrg_det(struct pmic *pmic)
{
	u8 val;
	unsigned int chrg_state = 0;
	val = i2c_reg_read(pmic->hw.i2c.addr, CHGSTATE_REG);
	if (val & 0x40) { 
#if defined(SUPPORT_USB_CONNECT_TO_ADP)
		chrg_state = dwc_otg_check_dpdm();
#else
		chrg_state = 2;	
#endif
	} else if (val & 0x80) {
		chrg_state = dwc_otg_check_dpdm();
	} else {
		chrg_state = 0;
	}

	return chrg_state;
}

static int ricoh619_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = ricoh619_chrg_det(bat);
	return 0;
}


/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/

static int ricoh619_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;

	state_of_chrg = ricoh619_chrg_det(bat);
	i2c_set_bus_num(bat->bus);
	i2c_init(100000, bat->hw.i2c.addr);
	ricoh619_charger_setting(bat,state_of_chrg);
	battery->voltage_uV = ricoh619_get_voltage(bat);
	battery->capacity = get_capcity(battery->voltage_uV);
	battery->state_of_chrg = state_of_chrg;
	printf("%s capacity = %d, voltage_uV = %d,state_of_chrg=%d\n",
	bat->name,battery->capacity,battery->voltage_uV,state_of_chrg);
	return 0;

}

static struct power_fg fg_ops = {
	.fg_battery_check = ricoh619_check_battery,
	.fg_battery_update = ricoh619_update_battery,
};

int fg_ricoh619_init(unsigned char bus,uchar addr)
{
	static const char name[] = "RICOH619_FG";
	if (!ricoh_fg.p)
		ricoh_fg.p = pmic_alloc();
	ricoh_fg.p->name = name;
	ricoh_fg.p->bus = bus;
	ricoh_fg.p->hw.i2c.addr = addr;
	ricoh_fg.p->interface = PMIC_I2C;
	ricoh_fg.p->fg = &fg_ops;
	ricoh_fg.p->pbat = calloc(sizeof(struct  power_battery), 1);
	i2c_set_bus_num(bus);
	i2c_init(100000,addr);
	i2c_reg_write(addr, ADCCNT3_REG, 0x28);
	return 0;
}

