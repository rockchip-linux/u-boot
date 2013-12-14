/*
 *  Copyright (C) 2012 rockchips
 *  zyw < zyw@rock-chips.com >
 *  for sample
 */

#include <common.h>
#include <power/pmic.h>
#include <i2c.h>
#include <asm/arch/rk_i2c.h>
#include <errno.h>
#include "pmic_act8846.h"

const static int buck_set_vol_base_addr[] = {
	act8846_BUCK1_SET_VOL_BASE,
	act8846_BUCK2_SET_VOL_BASE,
	act8846_BUCK3_SET_VOL_BASE,
	act8846_BUCK4_SET_VOL_BASE,
};
const static int buck_contr_base_addr[] = {
	act8846_BUCK1_CONTR_BASE,
 	act8846_BUCK2_CONTR_BASE,
 	act8846_BUCK3_CONTR_BASE,
 	act8846_BUCK4_CONTR_BASE,
};
#define act8846_BUCK_SET_VOL_REG(x) (buck_set_vol_base_addr[x])
#define act8846_BUCK_CONTR_REG(x) (buck_contr_base_addr[x])


const static int ldo_set_vol_base_addr[] = {
	act8846_LDO1_SET_VOL_BASE,
	act8846_LDO2_SET_VOL_BASE,
	act8846_LDO3_SET_VOL_BASE,
	act8846_LDO4_SET_VOL_BASE, 
	act8846_LDO5_SET_VOL_BASE, 
	act8846_LDO6_SET_VOL_BASE, 
	act8846_LDO7_SET_VOL_BASE, 
	act8846_LDO8_SET_VOL_BASE, 
//	act8846_LDO9_SET_VOL_BASE, 
};
const static int ldo_contr_base_addr[] = {
	act8846_LDO1_CONTR_BASE,
	act8846_LDO2_CONTR_BASE,
	act8846_LDO3_CONTR_BASE,
	act8846_LDO4_CONTR_BASE,
	act8846_LDO5_CONTR_BASE,
	act8846_LDO6_CONTR_BASE,
	act8846_LDO7_CONTR_BASE,
	act8846_LDO8_CONTR_BASE,
//	act8846_LDO9_CONTR_BASE,
};
#define act8846_LDO_SET_VOL_REG(x) (ldo_set_vol_base_addr[x])
#define act8846_LDO_CONTR_REG(x) (ldo_contr_base_addr[x])

const static int buck_voltage_map[] = {
	 600, 625, 650, 675, 700, 725, 750, 775,
	 800, 825, 850, 875, 900, 925, 950, 975,
	 1000, 1025, 1050, 1075, 1100, 1125, 1150,
	 1175, 1200, 1250, 1300, 1350, 1400, 1450,
	 1500, 1550, 1600, 1650, 1700, 1750, 1800, 
	 1850, 1900, 1950, 2000, 2050, 2100, 2150, 
	 2200, 2250, 2300, 2350, 2400, 2500, 2600, 
	 2700, 2800, 2850, 2900, 3000, 3100, 3200,
	 3300, 3400, 3500, 3600, 3700, 3800, 3900,
};

const static int ldo_voltage_map[] = {
	 600, 625, 650, 675, 700, 725, 750, 775,
	 800, 825, 850, 875, 900, 925, 950, 975,
	 1000, 1025, 1050, 1075, 1100, 1125, 1150,
	 1175, 1200, 1250, 1300, 1350, 1400, 1450,
	 1500, 1550, 1600, 1650, 1700, 1750, 1800, 
	 1850, 1900, 1950, 2000, 2050, 2100, 2150, 
	 2200, 2250, 2300, 2350, 2400, 2500, 2600, 
	 2700, 2800, 2850, 2900, 3000, 3100, 3200,
	 3300, 3400, 3500, 3600, 3700, 3800, 3900,
};

static struct act8846_reg_stable act8846_dcdc[] = {
	{
		.name		= "act_dcdc1",   //ddr
		.reg_ctl	= act8846_BUCK1_CONTR_BASE,
		.reg_vol	= act8846_BUCK1_SET_VOL_BASE,
	},
	{
		.name		= "vdd_core",    //logic
		.reg_ctl	= act8846_BUCK2_CONTR_BASE,
		.reg_vol	= act8846_BUCK2_SET_VOL_BASE,
	},
	{
		.name		= "vdd_cpu",   //arm
		.reg_ctl	= act8846_BUCK3_CONTR_BASE,
		.reg_vol	= act8846_BUCK3_SET_VOL_BASE,
	},
	{
		.name		= "act_dcdc4",   //vccio
		.reg_ctl	= act8846_BUCK4_CONTR_BASE,
		.reg_vol	= act8846_BUCK4_SET_VOL_BASE,
	},
}; 

static struct act8846_reg_stable act8846_ldo[] = {
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


static int act8846_set_bits(int reg_addr,int mask,int val)
{
	int tmp = 0,ret =0;

	ret = I2C_READ(reg_addr,&tmp);
	if (ret == 0){
		tmp = (tmp & ~mask) | val;
		ret = I2C_WRITE(reg_addr,&tmp);
	}
	return 0;	
}


/*
for chack charger status in boot
return 0, no charger
return 1, charging
*/
int check_charge(void)
{
    int reg=0;
    int ret = 0;
/*
    if(IReadLoaderFlag() == 0) {
		if(GetVbus()) { 	  //reboot charge
			printf("In charging! \n");
			ret = 1;
		}
    }
*/ 
    return ret;
}



/*
set charge current
0. disable charging  
1. usb charging, 500mA
2. ac adapter charging, 1.5A
*/
int pmic_charger_setting(int current)
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


static int check_vol_stable(int *vol_stable,int vol)
{
	int i=0;

	for(i=VOL_MIN_IDX;i<VOL_MAX_IDX;i++){
		if(vol <= (vol_stable[i]*1000))
			return i;
	}
	return -1;
}


static int act8846_set_init()
{
	int i=0,vol=0,reg_val=0;

	for(i = 0; i < ARRAY_SIZE(act8846_dcdc); i++){
		vol = pmic_get_vol(act8846_dcdc[i].name);
		if(vol == 0)
			continue;

		reg_val = check_vol_stable(buck_voltage_map,vol);
		if(reg_val == -1)
			continue;

		printf("pmu: name : %s, vol: %d ,val: %d\r\n",act8846_dcdc[i].name,vol,reg_val);

		//vol
		act8846_set_bits(act8846_dcdc[i].reg_vol,BUCK_VOL_MASK,reg_val);

		//en
		act8846_set_bits(act8846_dcdc[i].reg_ctl,BUCK_EN_MASK,BUCK_EN_MASK);
	}
	
	for(i = 0; i < ARRAY_SIZE(act8846_ldo); i++){
		vol = pmic_get_vol(act8846_ldo[i].name);
		if(vol == 0)
			continue;

		reg_val = check_vol_stable(ldo_voltage_map,vol);
		if(reg_val == -1)
			continue;

		printf("pmu2: name : %s, vol: %d ,val: %d\r\n",act8846_dcdc[i].name,vol,reg_val);
		//vol
		act8846_set_bits(act8846_ldo[i].reg_vol,LDO_VOL_MASK,reg_val);

		//en
		act8846_set_bits(act8846_ldo[i].reg_ctl,LDO_EN_MASK,LDO_EN_MASK);	
	}

	return 0;
}


int pmic_init(unsigned char bus)
{
	i2c_set_bus_num(I2C_CH);
	act8846_set_init();

	return 0;
}

void shut_down(void)
{

}

