/*
 * Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * zhangqing < zhangqing@rock-chips.com >
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <power/rockchip_power.h>
#include <power/rk816_pmic.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_rk816 rk816;

enum rk816_regulator {
	RK816_DCDC1 = 0,
	RK816_DCDC2,
	RK816_DCDC3,
	RK816_DCDC4,
	RK816_LDO1,
	RK816_LDO2,
	RK816_LDO3,
	RK816_LDO4,
	RK816_LDO5,
	RK816_LDO6,
	RK816_end
};

static int rk816_buck_set_vol_base_addr[] = {
	RK816_BUCK1_ON_VSEL_REG,
	RK816_BUCK2_ON_VSEL_REG,
	RK816_BUCK3_CONFIG_REG,
	RK816_BUCK4_ON_VSEL_REG,
};
#define rk816_BUCK_SET_VOL_REG(x) (rk816_buck_set_vol_base_addr[x])

static int rk816_ldo_set_vol_base_addr[] = {
	RK816_LDO1_ON_VSEL_REG,
	RK816_LDO2_ON_VSEL_REG,
	RK816_LDO3_ON_VSEL_REG,
	RK816_LDO4_ON_VSEL_REG,
	RK816_LDO5_ON_VSEL_REG,
	RK816_LDO6_ON_VSEL_REG,
};

#define rk816_LDO_SET_VOL_REG(x) (rk816_ldo_set_vol_base_addr[x - 4])

static struct fdt_regulator_match rk816_reg_matches[] = {
	{ .prop = "rk816_dcdc1",},
	{ .prop = "rk816_dcdc2",},
	{ .prop = "rk816_dcdc3",},
	{ .prop = "rk816_dcdc4",},
	{ .prop = "rk816_ldo1", },
	{ .prop = "rk816_ldo2", },
	{ .prop = "rk816_ldo3", },
	{ .prop = "rk816_ldo4", },
	{ .prop = "rk816_ldo5", },
	{ .prop = "rk816_ldo6", },
};

static struct fdt_regulator_match rk816_reg1_matches[] = {
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
};

static int rk816_i2c_probe(u32 bus, u32 addr)
{
	u16 val;
	int ret;

	i2c_set_bus_num(bus);
	i2c_init(RK816_I2C_SPEED, 0);
	ret  = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, RK816_CHIP_NAME_REG) << 8;
	val |= i2c_reg_read(addr, RK816_CHIP_VER_REG);
	if (val & 0x8160)
		return 0;
	else
		return -ENODEV;
}

int rk816_regulator_disable(int id)
{
	u8 val;

	if (id < 4) {
		val = i2c_reg_read(RK816_I2C_ADDR, RK816_DCDC_EN_REG1);
		i2c_reg_write(RK816_I2C_ADDR, RK816_DCDC_EN_REG1,
			      (val | (1 << (id + 4))) & (~(1 << id)));
	} else if (id < 8) {
		val = i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG1);
		i2c_reg_write(RK816_I2C_ADDR, RK816_LDO_EN_REG1,
			      (val | (1 << id)) & (~(1 << (id - 4))));
	} else {
		val = i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG2);
		i2c_reg_write(RK816_I2C_ADDR, RK816_LDO_EN_REG2,
			      (val | (1 << (id - 4))) & (~(1 << (id - 8))));
	}

	debug("disable %s %d dcdc_en = %08x ldo_en1 =%08x ldo_en2 =%08x\n",
	      __func__, id,
	      i2c_reg_read(RK816_I2C_ADDR, RK816_DCDC_EN_REG1),
	      i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG1),
	      i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG2));

	 return 0;
}


int rk816_regulator_enable(int id)
{
	u8 val;

	if (id < 4) {
		val = i2c_reg_read(RK816_I2C_ADDR, RK816_DCDC_EN_REG1);
		i2c_reg_write(RK816_I2C_ADDR, RK816_DCDC_EN_REG1,
			      (val | (1 << (id + 4))) | (1 << id));
	} else if (id < 8) {
		val = i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG1);
		i2c_reg_write(RK816_I2C_ADDR, RK816_LDO_EN_REG1,
			      (val | (1 << id)) | (1 << (id - 4)));
	} else {
		val = i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG2);
		i2c_reg_write(RK816_I2C_ADDR, RK816_LDO_EN_REG2,
			      (val | (1 << (id - 4))) | (1 << (id - 8)));
	}

	debug("enable %s %d dcdc_en = %08x ldo_en1 =%08x ldo_en2 =%08x\n",
	      __func__, id,
	      i2c_reg_read(RK816_I2C_ADDR, RK816_DCDC_EN_REG1),
	      i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG1),
	      i2c_reg_read(RK816_I2C_ADDR, RK816_LDO_EN_REG2));

	 return 0;
}

static int rk816_dcdc_select_min_voltage(int min_uV, int max_uV , int id)
{
	u16 vsel = 0;

	if ((id == 0) || (id == 1)) {
		if (min_uV < 712500)
			vsel = 0;
		else if (min_uV <= 1450000)
			vsel = ((min_uV - 712500) / 12500);
		else if (min_uV <= 2200000)
			vsel = 0x3c + ((min_uV - 1800000) / 200000);
		else if (min_uV == 2300000)
			vsel = 0x3f;
		else
			return -EINVAL;
	} else if (id == 3) {
		if (min_uV < 800000)
			vsel = 0;
		else if (min_uV <= 3500000)
			vsel = ((min_uV - 800000) / 100000);
		else
			return -EINVAL;
	} else {
		if (min_uV < 800000)
			vsel = 0;
		else if (min_uV <= 3400000)
			vsel = ((min_uV - 800000) / 100000);
		else
			return -EINVAL;
	}
	return vsel;
}

static int rk816_regulator_set_voltage(int id,
				  int min_uV, int max_uV)
{
	u16 val;
	u8 ret;

	val = rk816_dcdc_select_min_voltage(min_uV, max_uV, id);

	if (id < 4) {
		if (id == 2)
			return 0;
		ret = i2c_reg_read(RK816_I2C_ADDR,
					    rk816_BUCK_SET_VOL_REG(id));
		ret = ret & RK816_VOLT_MASK;
		i2c_reg_write(RK816_I2C_ADDR,
			      rk816_BUCK_SET_VOL_REG(id),
			      ret | val);
		debug("0 %s %d dcdc_vol = %08x\n", __func__, id, ret);
	} else {
		ret = i2c_reg_read(RK816_I2C_ADDR,
					     rk816_LDO_SET_VOL_REG(id));
		ret = ret & RK816_VOLT_MASK;
		i2c_reg_write(RK816_I2C_ADDR,
			      rk816_LDO_SET_VOL_REG(id),
					     ret | val);
		debug("1 %s %d %d ldo_vol =%08x\n", __func__, id,
		      val, ret);
	}
	return 0;
}

static int rk816_set_regulator_init(struct fdt_regulator_match *matches, int id)
{
	int ret = 0;

	if (matches->min_uV == matches->max_uV)
		ret = rk816_regulator_set_voltage(id, matches->min_uV,
						  matches->max_uV);
	if (matches->boot_on)
		ret = rk816_regulator_enable(id);
	return ret;
}

static int rk816_parse_dt(const void *blob)
{
	int node, nd;
	struct fdt_gpio_state gpios[2];
	u32 bus, addr;
	int ret, i;
	struct fdt_regulator_match *reg_match;
	const char *prop;
	int temp_node;

	node = fdt_node_offset_by_compatible(blob,
					     g_i2c_node,
					     COMPAT_ROCKCHIP_RK816);
	if (node < 0) {
		debug("can't find dts node for rk816\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, node)) {
		debug("device rk816 is disabled\n");
		return -1;
	}

	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("pmic rk816 get fdt i2c failed\n");
		return ret;
	}

	ret = rk816_i2c_probe(bus, addr);
	if (ret < 0) {
		debug("pmic rk816 i2c probe failed\n");
		return ret;
	}

	nd = fdt_get_regulator_node(blob, node);
	if (nd < 0) {
		printf("%s: Cannot find regulators\n", __func__);
		return -ENODEV;
	} else {
		fdt_for_each_subnode(blob, temp_node, nd) {
		prop = fdt_getprop(blob, temp_node,
				   "regulator-compatible",
				   NULL);
		if (prop)
			break;
	}

	if (prop)
		reg_match = rk816_reg_matches;
	else
		reg_match = rk816_reg1_matches;

	fdt_regulator_match(blob, nd, reg_match,
			    RK816_NUM_REGULATORS);
	}

	for (i = 0; i < RK816_NUM_REGULATORS; i++) {
		regulator_init_pmic_matches[i].name = reg_match[i].name;
		if (reg_match[i].boot_on || (reg_match[i].min_uV == reg_match[i].max_uV))
			ret = rk816_set_regulator_init(&reg_match[i], i);
	}

	fdtdec_decode_gpios(blob, node, "gpios", gpios, 2);

	rk816.pmic = pmic_alloc();
	rk816.node = node;
	rk816.pmic->hw.i2c.addr = addr;
	rk816.pmic->bus = bus;
	debug("rk816 i2c_bus:%d addr:0x%02x\n", rk816.pmic->bus,
	      rk816.pmic->hw.i2c.addr);

	return 0;
}

static int rk816_pre_init(unsigned char bus, uchar addr)
{
	debug("%s,line=%d\n", __func__, __LINE__);

	i2c_set_bus_num(bus);
	i2c_init(RK816_I2C_SPEED, addr);
	i2c_set_bus_speed(RK816_I2C_SPEED);

	rk816_regulator_disable(9);
	return 0;
}

int pmic_rk816_init(unsigned char bus)
{
	int ret;
	if (!rk816.pmic) {
		if (!gd->fdt_blob)
			return -1;

		ret = rk816_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}

	rk816.pmic->interface = PMIC_I2C;
	/*enable lcdc power ldo, and enable other ldo*/
	ret = rk816_pre_init(rk816.pmic->bus, rk816.pmic->hw.i2c.addr);
	if (ret < 0)
		return ret;

	fg_rk816_init(rk816.pmic->bus, rk816.pmic->hw.i2c.addr);

	return 0;
}

void pmic_rk816_shut_down(void)
{
	u8 reg;
	i2c_set_bus_num(rk816.pmic->bus);
	i2c_init(RK816_I2C_SPEED, rk816.pmic->hw.i2c.addr);
	i2c_set_bus_speed(RK816_I2C_SPEED);
	reg = i2c_reg_read(rk816.pmic->hw.i2c.addr, RK816_DEV_CTRL_REG);
	i2c_reg_write(rk816.pmic->hw.i2c.addr, RK816_DEV_CTRL_REG,
		      (reg | (0x1 << 0)));
}

