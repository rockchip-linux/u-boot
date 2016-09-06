/*
 * Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * zhangqing < zhangqing@rock-chips.com >
 * andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <power/rockchip_power.h>
#include <power/rk818_pmic.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_rk818 rk818;

int support_dc_chg;

enum rk818_regulator {
	RK818_DCDC1=0,
	RK818_DCDC2,
	RK818_DCDC3,
	RK818_DCDC4,
	RK818_LDO1,
	RK818_LDO2,
	RK818_LDO3,
	RK818_LDO4,
	RK818_LDO5,
	RK818_LDO6,
	RK818_LDO7,
	RK818_LDO8,
	RK818_LDO9,
	RK818_LDO10,
	RK818_end
};

const static int buck_set_vol_base_addr[] = {
	RK818_BUCK1_ON_REG,
	RK818_BUCK2_ON_REG,
	RK818_BUCK3_CONFIG_REG,
	RK818_BUCK4_ON_REG,
};
#define rk818_BUCK_SET_VOL_REG(x) (buck_set_vol_base_addr[x])

const static int ldo_set_vol_base_addr[] = {
	RK818_LDO1_ON_VSEL_REG,
	RK818_LDO2_ON_VSEL_REG,
	RK818_LDO3_ON_VSEL_REG,
	RK818_LDO4_ON_VSEL_REG,
	RK818_LDO5_ON_VSEL_REG,
	RK818_LDO6_ON_VSEL_REG,
	RK818_LDO7_ON_VSEL_REG,
	RK818_LDO8_ON_VSEL_REG,
	RK818_BOOST_LDO9_ON_VSEL_REG,
};

#define rk818_LDO_SET_VOL_REG(x) (ldo_set_vol_base_addr[x - 4])

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

static struct fdt_regulator_match rk818_reg_matches[] = {
	{ .prop = "rk818_dcdc1",},
	{ .prop = "rk818_dcdc2",},
	{ .prop = "rk818_dcdc3",},
	{ .prop = "rk818_dcdc4",},
	{ .prop = "rk818_ldo1", },
	{ .prop = "rk818_ldo2", },
	{ .prop = "rk818_ldo3", },
	{ .prop = "rk818_ldo4", },
	{ .prop = "rk818_ldo5", },
	{ .prop = "rk818_ldo6", },
	{ .prop = "rk818_ldo7", },
	{ .prop = "rk818_ldo8", },
	{ .prop = "rk818_ldo9", },
	{ .prop = "rk818_ldo10",},
};

static struct fdt_regulator_match rk818_reg1_matches[] = {
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
	{ .prop = "LDO_REG9", },
	{ .prop = "SWITCH_REG",},
};

static int rk818_i2c_probe(u32 bus, u32 addr)
{
	char val;
	int ret;

	i2c_set_bus_num(bus);
	i2c_init(RK818_I2C_SPEED, 0);
	ret  = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, 0x2f);
	if ((val == 0xff) || (val == 0))
		return -ENODEV;
	else
		return 0;
	
	
}

int rk818_regulator_disable(int num_regulator)
{

	if (num_regulator < 4)
		i2c_reg_write(RK818_I2C_ADDR, RK818_DCDC_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR, RK818_DCDC_EN_REG) &(~(1 << num_regulator))); //enable dcdc
	else if (num_regulator == 12)
		i2c_reg_write(RK818_I2C_ADDR, RK818_DCDC_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR,RK818_DCDC_EN_REG) &(~(1 << 5))); //enable ldo9
	else if (num_regulator == 13)
		i2c_reg_write(RK818_I2C_ADDR, RK818_DCDC_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR,RK818_DCDC_EN_REG) &(~(1 << 6))); //enable ldo10
	else
	 	i2c_reg_write(RK818_I2C_ADDR, RK818_LDO_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR,RK818_LDO_EN_REG) &(~(1 << (num_regulator -4)))); //enable ldo

	debug("1 %s %d dcdc_en = %08x ldo_en =%08x\n", __func__, num_regulator, i2c_reg_read(RK818_I2C_ADDR,RK818_DCDC_EN_REG), i2c_reg_read(RK818_I2C_ADDR,RK818_LDO_EN_REG));

	 return 0;
}


int rk818_regulator_enable(int num_regulator)
{

	if (num_regulator < 4)
		i2c_reg_write(RK818_I2C_ADDR, RK818_DCDC_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR, RK818_DCDC_EN_REG) |(1 << num_regulator)); //enable dcdc
	else if (num_regulator == 12)
		i2c_reg_write(RK818_I2C_ADDR, RK818_DCDC_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR,RK818_DCDC_EN_REG) |(1 << 5)); //enable ldo9
	else if (num_regulator == 13)
		i2c_reg_write(RK818_I2C_ADDR, RK818_DCDC_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR,RK818_DCDC_EN_REG) |(1 << 6)); //enable ldo10
	else
	 	i2c_reg_write(RK818_I2C_ADDR, RK818_LDO_EN_REG,
			i2c_reg_read(RK818_I2C_ADDR,RK818_LDO_EN_REG) |(1 << (num_regulator -4))); //enable ldo

	debug("1 %s %d dcdc_en = %08x ldo_en =%08x\n", __func__, num_regulator, i2c_reg_read(RK818_I2C_ADDR,RK818_DCDC_EN_REG), i2c_reg_read(RK818_I2C_ADDR,RK818_LDO_EN_REG));

	 return 0;
}

static int rk818_dcdc_select_min_voltage(int min_uV, int max_uV ,int num_regulator)
{
	u16 vsel =0;
	
	if (num_regulator == 0 || num_regulator ==  1){
		if (min_uV < 700000)
		vsel = 0;
		else if (min_uV <= 1500000)
		vsel = ((min_uV - 700000) / 12500) ;
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

static int rk818_regulator_set_voltage(int num_regulator,
				  int min_uV, int max_uV)
{
	const int *vol_map;
	int min_vol = min_uV / 1000;
	u16 val;
	int num =0;

	if (num_regulator < 4){
		if (num_regulator == 2)
			return 0;
		val = rk818_dcdc_select_min_voltage(min_uV,max_uV,num_regulator);	
		i2c_reg_write(RK818_I2C_ADDR, rk818_BUCK_SET_VOL_REG(num_regulator),
			(i2c_reg_read(RK818_I2C_ADDR, rk818_BUCK_SET_VOL_REG(num_regulator)) & (~0x3f)) | val);
		debug("1 %s %d dcdc_vol = %08x\n", __func__, num_regulator, i2c_reg_read(RK818_I2C_ADDR, rk818_BUCK_SET_VOL_REG(num_regulator)));
		return 0;
	}else if (num_regulator == 6){
		vol_map = ldo3_voltage_map;
		num = 15;
	}
	else if (num_regulator == 9 || num_regulator == 10){
		vol_map = ldo6_voltage_map;
		num = 17;
	}
	else {
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

	if (num_regulator == 12) {
		i2c_reg_write(RK818_I2C_ADDR, rk818_LDO_SET_VOL_REG(num_regulator),
			((i2c_reg_read(RK818_I2C_ADDR, rk818_LDO_SET_VOL_REG(num_regulator)) & (~0x1f)) | val));
	}
	else
		i2c_reg_write(RK818_I2C_ADDR, rk818_LDO_SET_VOL_REG(num_regulator),
			((i2c_reg_read(RK818_I2C_ADDR, rk818_LDO_SET_VOL_REG(num_regulator)) & (~0x3f)) | val));
	
	debug("1 %s %d %d ldo_vol =%08x\n", __func__, num_regulator, val, i2c_reg_read(RK818_I2C_ADDR, rk818_LDO_SET_VOL_REG(num_regulator)));

	return 0;
}

static int rk818_set_regulator_init(struct fdt_regulator_match *matches, int num_matches)
{
	int ret;

	if (matches->init_uV)
		ret = rk818_regulator_set_voltage(num_matches, matches->init_uV,
						  matches->init_uV);
	else if (matches->min_uV == matches->max_uV)
		ret = rk818_regulator_set_voltage(num_matches, matches->min_uV,
						  matches->max_uV);
	if (matches->boot_on)
		ret = rk818_regulator_enable(num_matches);

	return ret;
}

static int rk818_parse_dt(const void* blob)
{
	int node, nd;
	struct fdt_gpio_state gpios[2];
	u32 bus, addr;
	int ret, i;
	struct fdt_regulator_match *reg_match = NULL;
	const char *prop;
	int temp_node;

	node = fdt_node_offset_by_compatible(blob,
					g_i2c_node, COMPAT_ROCKCHIP_RK818);
	if (node < 0) {
		debug("can't find dts node for rk818\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		debug("device rk818 is disabled\n");
		return -1;
	}
	
	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("pmic rk818 get fdt i2c failed\n");
		return ret;
	}

	ret = rk818_i2c_probe(bus, addr);
	if (ret < 0) {
		debug("pmic rk818 i2c probe failed\n");
		return ret;
	}

	nd = fdt_get_regulator_node(blob, node);
	if (nd < 0)
		printf("%s: Cannot find regulators\n", __func__);
	else {
		fdt_for_each_subnode(blob, temp_node, nd) {
			prop = fdt_getprop(blob, temp_node,
					   "regulator-compatible", NULL);
			if (prop)
				break;
		}

		if (prop)
			reg_match = rk818_reg_matches;
		else
			reg_match = rk818_reg1_matches;

		fdt_regulator_match(blob, nd, reg_match,
				    RK818_NUM_REGULATORS);
	}

	for (i = 0; i < RK818_NUM_REGULATORS; i++) {
		regulator_init_pmic_matches[i].name = reg_match[i].name;
		ret = rk818_set_regulator_init(&reg_match[i], i);
	}

	fdtdec_decode_gpios(blob, node, "gpios", gpios, 2);
	support_dc_chg = fdtdec_get_int(blob, node, "rk818,support_dc_chg",0);

	rk818.pmic = pmic_alloc();
	rk818.node = node;
	rk818.pmic->hw.i2c.addr = addr;
	rk818.pmic->bus = bus;
	debug("rk818 i2c_bus:%d addr:0x%02x\n", rk818.pmic->bus,
		rk818.pmic->hw.i2c.addr);

	return 0;
}

static int rk818_pre_init(unsigned char bus,uchar addr)
{
	debug("%s,line=%d\n", __func__,__LINE__);
	 
	i2c_set_bus_num(bus);
	i2c_init(RK818_I2C_SPEED, addr);
	i2c_set_bus_speed(RK818_I2C_SPEED);

	i2c_reg_write(addr, 0xa1,i2c_reg_read(addr,0xa1)|0x70); /*close charger when usb low then 3.4V*/
 	i2c_reg_write(addr, 0x52,i2c_reg_read(addr,0x52)|0x02); /*no action when vref*/
 	i2c_reg_write(addr, RK818_DCDC_EN_REG,
		i2c_reg_read(addr, RK818_DCDC_EN_REG) |0x60); /*enable switch & ldo9*/
//	i2c_reg_write(addr, RK818_LDO_EN_REG,
//		i2c_reg_read(addr, RK818_LDO_EN_REG) |0x28);
 	
	/**********enable clkout2****************/
        i2c_reg_write(addr,RK818_CLK32OUT_REG,0x01);
      
	return 0;
}

int pmic_rk818_init(unsigned char bus)
{
	int ret;
	if (!rk818.pmic) {
		if (!gd->fdt_blob)
			return -1;

		ret = rk818_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}

	rk818.pmic->interface = PMIC_I2C;
	//enable lcdc power ldo, and enable other ldo 
	ret = rk818_pre_init(rk818.pmic->bus, rk818.pmic->hw.i2c.addr);
	if (ret < 0)
			return ret;
	fg_rk818_init(rk818.pmic->bus, rk818.pmic->hw.i2c.addr);

	return 0;
}


void pmic_rk818_shut_down(void)
{
	u8 reg;
	i2c_set_bus_num(rk818.pmic->bus);
    	i2c_init (RK818_I2C_SPEED, rk818.pmic->hw.i2c.addr);
    	i2c_set_bus_speed(RK818_I2C_SPEED);
	reg = i2c_reg_read(rk818.pmic->hw.i2c.addr, RK818_DEVCTRL_REG);
	i2c_reg_write(rk818.pmic->hw.i2c.addr, RK818_DEVCTRL_REG, (reg |(0x1 <<0)));

}

void pmic_rk818_power_init(void){
	rk818_regulator_disable(RK818_LDO1);
	rk818_regulator_disable(RK818_LDO2);
#ifndef CONFIG_RKCHIP_RK3399
	rk818_regulator_disable(RK818_LDO8);
#endif
}

void pmic_rk818_power_on(void){
#ifndef CONFIG_RKCHIP_RK3399
	rk818_regulator_enable(RK818_LDO4);
	rk818_regulator_enable(RK818_LDO6);
#endif
	//rk818_regulator_enable(RK818_LDO9);
	//rk818_regulator_enable(RK818_LDO10);
	
	//gpio_direction_output(LCD_EN_PIN,1);
	// mipi wakeup need 120ms
	//mdelay(120);
}

void pmic_rk818_power_off(void){
	//gpio_direction_output(LCD_EN_PIN,0);
#ifndef CONFIG_RKCHIP_RK3399
	rk818_regulator_disable(RK818_LDO6);
	rk818_regulator_disable(RK818_LDO4);
#endif
	//rk818_regulator_disable(RK818_LDO9);
	//rk818_regulator_disable(RK818_LDO10);
}


