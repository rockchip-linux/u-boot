/*
 * Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * zyw < zyw@rock-chips.com >
 * andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*#define DEBUG*/
#include <common.h>
#include <errno.h>
#include <power/rk808_pmic.h>
#include <power/rockchip_power.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_rk808 rk808;


enum rk808_regulator {
	RK808_DCDC1=0,
	RK808_DCDC2,
	RK808_DCDC3,
	RK808_DCDC4,
	RK808_LDO1,
	RK808_LDO2,
	RK808_LDO3,
	RK808_LDO4,
	RK808_LDO5,
	RK808_LDO6,
	RK808_LDO7,
	RK808_LDO8,
	RK808_LDO9,
	RK808_LDO10,
	RK808_end
};

const static int buck_set_vol_base_addr[] = {
	RK808_BUCK1_ON_REG,
	RK808_BUCK2_ON_REG,
	RK808_BUCK3_CONFIG_REG,
	RK808_BUCK4_ON_REG,
};
#define rk808_BUCK_SET_VOL_REG(x) (buck_set_vol_base_addr[x])

const static int ldo_set_vol_base_addr[] = {
	RK808_LDO1_ON_VSEL_REG,
	RK808_LDO2_ON_VSEL_REG,
	RK808_LDO3_ON_VSEL_REG,
	RK808_LDO4_ON_VSEL_REG,
	RK808_LDO5_ON_VSEL_REG,
	RK808_LDO6_ON_VSEL_REG,
	RK808_LDO7_ON_VSEL_REG,
	RK808_LDO8_ON_VSEL_REG,
};

#define rk808_LDO_SET_VOL_REG(x) (ldo_set_vol_base_addr[x - 4])

const static int ldo_voltage_map[] = {
	  1800, 1900, 2000, 2100, 2200,  2300,  2400, 2500, 2600,
	  2700, 2800, 2900, 3000, 3100, 3200,3300, 3400,
};
const static int ldo3_voltage_map[] = {
	 800, 900, 1000, 1100, 1200,  1300, 1400, 1500, 1600,
	 1700, 1800, 1900,  2000,2100,  2200,  2500,
};
const static int ldo6_voltage_map[] = {
	 800, 900, 1000, 1100, 1200,  1300, 1400, 1500, 1600,
	 1700, 1800, 1900,  2000,2100,  2200,  2300,2400,2500,
};

static struct fdt_regulator_match rk808_reg_matches[] = {
	{ .prop = "rk_dcdc1",},
	{ .prop = "rk_dcdc2",},
	{ .prop = "rk_dcdc3",},
	{ .prop = "rk_dcdc4",},
	{ .prop = "rk_ldo1", },
	{ .prop = "rk_ldo2", },
	{ .prop = "rk_ldo3", },
	{ .prop = "rk_ldo4", },
	{ .prop = "rk_ldo5", },
	{ .prop = "rk_ldo6", },
	{ .prop = "rk_ldo7", },
	{ .prop = "rk_ldo8", },
	{ .prop = "rk_ldo9", },
	{ .prop = "rk_ldo10",},
};

static struct fdt_regulator_match rk808_reg1_matches[] = {
	{ .prop = "DCDC_REG1",},
	{ .prop = "DCDC_REG2",},
	{ .prop = "DCDC_REG3",},
	{ .prop = "DCDC_REG4",},
	{ .prop = "LDO_REG1", },
	{ .prop = "LDO_REG2", },
	{ .prop = "LDO_REG3", },
	{ .prop = "LDO_REG4", },
	{ .prop = "LDO_REG5", },
	{ .prop = "LDO_REG6", },
	{ .prop = "LDO_REG7", },
	{ .prop = "LDO_REG8", },
	{ .prop = "SWITCH_REG1",},
	{ .prop = "SWITCH_REG2",},
};

#if 0
/*
for chack charger status in boot
return 0, no charger
return 1, charging
*/
int check_charge(void)
{
    int reg=0;
    int ret = 0;
	if(GetVbus()) { 	  //reboot charge
		debug("In charging! \n");
		ret = 1;
	}
    return ret;
}

#endif

/*
set charge current
0. disable charging  
1. usb charging, 500mA
2. usb adapter charging, 2A
3. ac adapter charging , 3A
*/
int pmic_rk808_charger_setting(int current)
{
    debug("%s charger_type = %d\n",__func__,current);
    i2c_set_bus_num(rk808.pmic->bus);
    i2c_init (RK808_I2C_SPEED, 0x6b);
    debug("%s charge ic id = 0x%x\n",__func__,i2c_reg_read(0x6b,0x0a));
    switch (current){
    case 0:
        //disable charging
        break;
    case 1:
        //set charger current to 500ma
        break;
    case 2:
         //set charger current to 1.5A
        i2c_reg_write(0x6b,0,(i2c_reg_read(0x6b,0)&0xf8)|0x6);/* Input Current Limit  2A */
        break;
    case 3:
        //set charger current to 1.5A
        i2c_reg_write(0x6b,0,(i2c_reg_read(0x6b,0)&0xf8)|0x7);/* Input Current Limit 3A */
        break;
    default:
        break;
    }
    return 0;
}

int charger_init(unsigned char bus)
{
    int usb_charger_type = dwc_otg_check_dpdm();

    debug("%s, charger_type = %d, dc_is_charging= %d\n",__func__,usb_charger_type,is_charging());
    if(1){
        pmic_rk808_charger_setting(3);
    }else if(usb_charger_type){
        pmic_rk808_charger_setting(usb_charger_type);
    }
    return 0;

}

static int rk808_i2c_probe(u32 bus, u32 addr)
{
	char val;
	int ret;
	i2c_set_bus_num(bus);
	i2c_init(RK808_I2C_SPEED, 0);
	ret  = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, 0x2f);
	if (val == 0xff)
		return -ENODEV;
	else
		return 0;
	
	
}

int rk808_regulator_disable(int num_regulator)
{

	if (num_regulator < 4)
		i2c_reg_write(RK808_I2C_ADDR, RK808_DCDC_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR, RK808_DCDC_EN_REG) &(~(1 << num_regulator))); /*enable dcdc*/
	else if (num_regulator == 12)
		i2c_reg_write(RK808_I2C_ADDR, RK808_DCDC_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR,RK808_DCDC_EN_REG) &(~(1 << 5))); /*enable switch1*/
	else if (num_regulator == 13)
		i2c_reg_write(RK808_I2C_ADDR, RK808_DCDC_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR,RK808_DCDC_EN_REG) &(~(1 << 6))); /*enable switch2*/
	else
	 	i2c_reg_write(RK808_I2C_ADDR, RK808_LDO_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR,RK808_LDO_EN_REG) &(~(1 << (num_regulator -4)))); /*enable ldo*/

	debug("1 %s %d dcdc_en = %08x ldo_en =%08x\n", __func__, num_regulator, i2c_reg_read(RK808_I2C_ADDR,RK808_DCDC_EN_REG), i2c_reg_read(RK808_I2C_ADDR,RK808_LDO_EN_REG));

	 return 0;
}


int rk808_regulator_enable(int num_regulator)
{

	if (num_regulator < 4)
		i2c_reg_write(RK808_I2C_ADDR, RK808_DCDC_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR, RK808_DCDC_EN_REG) |(1 << num_regulator)); /*enable dcdc*/
	else if (num_regulator == 12)
		i2c_reg_write(RK808_I2C_ADDR, RK808_DCDC_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR,RK808_DCDC_EN_REG) |(1 << 5)); /*enable switch1*/
	else if (num_regulator == 13)
		i2c_reg_write(RK808_I2C_ADDR, RK808_DCDC_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR,RK808_DCDC_EN_REG) |(1 << 6)); /*enable switch2*/
	else
	 	i2c_reg_write(RK808_I2C_ADDR, RK808_LDO_EN_REG,
			i2c_reg_read(RK808_I2C_ADDR,RK808_LDO_EN_REG) |(1 << (num_regulator -4))); /*enable ldo*/

	debug("1 %s %d dcdc_en = %08x ldo_en =%08x\n", __func__, num_regulator, i2c_reg_read(RK808_I2C_ADDR,RK808_DCDC_EN_REG), i2c_reg_read(RK808_I2C_ADDR,RK808_LDO_EN_REG));

	 return 0;
}

static int rk808_dcdc_select_min_voltage(int min_uV, int max_uV ,int num_regulator)
{
	u16 vsel =0;
	
	if (num_regulator == 0 || num_regulator ==  1){
		if (min_uV < 712500)
		vsel = 0;
		else if (min_uV <= 1500000)
		vsel = ((min_uV - 712500) / 12500) ;
		else
		return -EINVAL;
	}
	else if (num_regulator ==3){
		if (min_uV < 1800000)
		vsel = 0;
		else if (min_uV <= 3300000)
		vsel = ((min_uV - 1800000) / 100000) ;
		else
		return -EINVAL;
	}
	return vsel;
}

static int rk808_regulator_set_voltage(int num_regulator,
				  int min_uV, int max_uV)
{
	const int *vol_map;
	int min_vol = min_uV / 1000;
	u16 val;
	int num =0;

	if (num_regulator < 4) {
		if (num_regulator == 2)
			return 0;
		val = rk808_dcdc_select_min_voltage(min_uV,max_uV,num_regulator);	
		i2c_reg_write(RK808_I2C_ADDR, rk808_BUCK_SET_VOL_REG(num_regulator),
			(i2c_reg_read(RK808_I2C_ADDR, rk808_BUCK_SET_VOL_REG(num_regulator)) & 0x3f) | val);
		debug("1 %s %d dcdc_vol = %08x\n", __func__, num_regulator, i2c_reg_read(RK808_I2C_ADDR, rk808_BUCK_SET_VOL_REG(num_regulator)));
		return 0;
	} else if (num_regulator == 12 || num_regulator == 13) {
		return 0;
	} else if (num_regulator == 6) {
		vol_map = ldo3_voltage_map;
		num = 15;
	} else if (num_regulator == 9 || num_regulator == 10) {
		vol_map = ldo6_voltage_map;
		num = 17;
	} else {
		vol_map = ldo_voltage_map;
		num = 16;
	}

	if (min_vol < vol_map[0] ||
	    min_vol > vol_map[num])
		return -EINVAL;

	for (val = 0; val <= num; val++){
		if (vol_map[val] >= min_vol)
			break;
        }
	i2c_reg_write(RK808_I2C_ADDR, rk808_LDO_SET_VOL_REG(num_regulator),
		((i2c_reg_read(RK808_I2C_ADDR, rk808_LDO_SET_VOL_REG(num_regulator)) & (~0x3f)) | val));
	
	debug("1 %s %d %d ldo_vol =%08x\n", __func__, num_regulator, val, i2c_reg_read(RK808_I2C_ADDR, rk808_LDO_SET_VOL_REG(num_regulator)));

	return 0;
}

static int rk808_set_regulator_init(struct fdt_regulator_match *matches, int num_matches)
{
	int ret;

	if (matches->init_uV)
		ret = rk808_regulator_set_voltage(num_matches, matches->init_uV,
						  matches->init_uV);
	else if (matches->min_uV == matches->max_uV)
		ret = rk808_regulator_set_voltage(num_matches, matches->min_uV,
						  matches->max_uV);
	if (matches->boot_on)
		ret = rk808_regulator_enable(num_matches);


	return ret;
}

static int rk808_parse_dt(const void* blob)
{
	int node, nd;
	struct fdt_gpio_state gpios[2];
	u32 bus, addr;
	int ret, i;
	struct fdt_regulator_match *reg_match = NULL;
	const char *prop;
	int temp_node;
	
	node = fdt_node_offset_by_compatible(blob,
					g_i2c_node, COMPAT_ROCKCHIP_RK808);
	if (node < 0) {
		debug("can't find dts node for rk808\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		debug("device rk808 is disabled\n");
		return -1;
	}
	
	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("pmic rk808 get fdt i2c failed\n");
		return ret;
	}

	ret = rk808_i2c_probe(bus, addr);
	if (ret < 0) {
		debug("pmic rk808 i2c probe failed\n");
		return ret;
	}
	
	nd = fdt_get_regulator_node(blob, node);
	if (nd < 0) {
		printf("%s: Cannot find regulators\n", __func__);
	} else {
		fdt_for_each_subnode(blob, temp_node, nd) {
			prop = fdt_getprop(blob, temp_node,
					   "regulator-compatible", NULL);
			if (prop)
				break;
		}

		if (prop)
			reg_match = rk808_reg_matches;
		else
			reg_match = rk808_reg1_matches;

		fdt_regulator_match(blob, nd, reg_match,
				    RK808_NUM_REGULATORS);
	}

	for (i = 0; i < RK808_NUM_REGULATORS; i++) {
		regulator_init_pmic_matches[i].name = reg_match[i].name;
		ret = rk808_set_regulator_init(&reg_match[i], i);
	}

	fdtdec_decode_gpios(blob, node, "gpios", gpios, 2);

	rk808.pmic = pmic_alloc();
	rk808.node = node;
	rk808.pmic->hw.i2c.addr = addr;
	rk808.pmic->bus = bus;
	rk808.pwr_hold.gpio = gpios[1].gpio;
	rk808.pwr_hold.flags = !(gpios[1].flags  & OF_GPIO_ACTIVE_LOW);
	debug("rk808 i2c_bus:%d addr:0x%02x\n", rk808.pmic->bus,
		rk808.pmic->hw.i2c.addr);

	return 0;
}

int pmic_rk808_init(unsigned char bus)
{
	int ret;
	if (!rk808.pmic) {
		if (!gd->fdt_blob)
			return -1;

		ret = rk808_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}
	
	rk808.pmic->interface = PMIC_I2C;
	//enable lcdc power ldo, and enable other ldo 
	i2c_set_bus_num(rk808.pmic->bus);
	charger_init(0);
	i2c_init(RK808_I2C_SPEED, rk808.pmic->hw.i2c.addr);
	i2c_set_bus_speed(RK808_I2C_SPEED);

    return 0;
}


void pmic_rk808_shut_down(void)
{
	u8 reg;
	i2c_set_bus_num(0);
	reg = i2c_reg_read(0x1b, 0x4b);
	i2c_reg_write(0x1b, 0x4b, (reg |(0x1 <<3)));

}

