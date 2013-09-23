/*
 *  Copyright (C) 2013 rockchips
 *  zyw < zyw@rock-chips.com >
 *  for battery driver sample
 */

#include <common.h>
#include <power/pmic.h>
#include <power/battery.h>

#include <errno.h>

#define PMU_DEBUG 0

#define VOLTAGE_1                       0xEB
#define VOLTAGE_0                       0xEC

/* 619 Register information */
/* bank 0 */
#define PSWR_REG                        0x07
/* for ADC */
#define INTEN_REG               0x9D
#define EN_ADCIR3_REG           0x8A
#define ADCCNT3_REG             0x66
#define VBATDATAH_REG           0x6A
#define VBATDATAL_REG           0x6B

#define CHGCTL1_REG             0xB3
#define REGISET1_REG    0xB6
#define REGISET2_REG    0xB7
#define CHGISET_REG             0xB8
#define BATSET2_REG             0xBB
#define FG_CTRL_REG     0xE0
#define	SOC_REG			0xE1
#define	TEMP_1_REG		0xED
#define	TEMP_2_REG		0xEE
#define RICOH619_TAH_SEL2				5
#define RICOH619_TAL_SEL2				6

#define PMU_I2C_ADDRESS        CONFIG_SYS_I2C_SLAVE

int state_of_chrg = 0;

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

int pmu_i2c_init()
{
	u8 reg_val,i;
    i2c_set_bus_num(1);
    i2c_init (PMU_I2C_ADDRESS, CONFIG_SYS_I2C_SLAVE);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);

	pmu_i2c_set(PMU_I2C_ADDRESS, 0x66, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x64, 0x02);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x65, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x88, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x89, 0x00);
	pmu_i2c_set(PMU_I2C_ADDRESS, 0x66, 0x20);

	return 1;
}

int read_pmu_reg(u8 reg,u8 *reg_val)
{
	*reg_val=i2c_reg_read(PMU_I2C_ADDRESS,reg);
	return 0;
}

int write_pmu_reg(u8 reg,u8 reg_val)
{
	pmu_i2c_set(PMU_I2C_ADDRESS, reg, reg_val);
	return 0;
}

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

static int get_battery_temp()
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

static int calc_capacity()
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



// the unit of voltage value is mV .
int pmu_get_voltage()
{
	u8 voltage_h,voltage_l;
	int vol=0,vol_tmp1,vol_tmp2;
	u8 i;
	int ret=0;

	for(i=1;i<11;i++)
	{
	ret=read_pmu_reg(VBATDATAL_REG,&voltage_l);
	ret=read_pmu_reg(VBATDATAH_REG,&voltage_h);
	vol_tmp1=voltage_l;
	vol_tmp2=voltage_h;
	vol+=(vol_tmp1&0xF)|((vol_tmp2&0xFF)<<4);
	
	if(PMU_DEBUG)
		printf("voltage_l=%x,voltage_h=%x,voltage=%lx \n",voltage_l,voltage_h,vol/i);
	}
	vol=vol/10;
	vol=vol*5000/4095;
	
	if(PMU_DEBUG)
	printf("the voltage of battery is %d mV\n",vol);
	return vol;
}

/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/
int get_power_bat_status(struct battery *batt_status)
{
    u8 reg_val;
    int ret=0;
    
    if(!state_of_chrg)
    {
        state_of_chrg = dwc_otg_check_dpdm();
        pmu_i2c_init();
    }
    batt_status->capacity = calc_capacity();

    batt_status->voltage_uV = pmu_get_voltage();
    
    batt_status->state_of_chrg = state_of_chrg;
    printf("%s capacity = %d, voltage_uV = %d,state_of_chrg=%d\n",__func__,batt_status->capacity,batt_status->voltage_uV,batt_status->state_of_chrg  );
	return 0;
}
