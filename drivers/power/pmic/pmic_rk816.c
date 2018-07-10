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

static struct pmic_rk816 *rk8xx;

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

/***************************** RK816 ******************************************/
static struct fdt_regulator_match rk816_reg_matches[] = {
	{ .prop = "RK816_DCDC1",},
	{ .prop = "RK816_DCDC2",},
	{ .prop = "RK816_DCDC3",},
	{ .prop = "RK816_DCDC4",},
	{ .prop = "RK816_LDO1", },
	{ .prop = "RK816_LDO2", },
	{ .prop = "RK816_LDO3", },
	{ .prop = "RK816_LDO4", },
	{ .prop = "RK816_LDO5", },
	{ .prop = "RK816_LDO6", },
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

/***************************** RK805 ******************************************/
enum rk805_regulator {
	RK805_DCDC1 = 0,
	RK805_DCDC2,
	RK805_DCDC3,
	RK805_DCDC4,
	RK805_LDO1,
	RK805_LDO2,
	RK805_LDO3,
	RK805_end
};

static struct fdt_regulator_match rk805_reg_matches[] = {
	{ .prop = "RK805_DCDC1",},
	{ .prop = "RK805_DCDC2",},
	{ .prop = "RK805_DCDC3",},
	{ .prop = "RK805_DCDC4",},
	{ .prop = "RK805_LDO1", },
	{ .prop = "RK805_LDO2", },
	{ .prop = "RK805_LDO3", },
};

static struct fdt_regulator_match rk805_reg1_matches[] = {
	{ .prop = "DCDC_REG1",},
	{ .prop = "DCDC_REG2",},
	{ .prop = "DCDC_REG3",},
	{ .prop = "DCDC_REG4",},
	{ .prop = "LDO_REG1", },
	{ .prop = "LDO_REG2", },
	{ .prop = "LDO_REG3", },
};

/***************************** RK805 END **************************************/

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
	if ((val & 0x8160) == 0x8160 || (val & 0x8050) == 0x8050)
		return 0;
	else
		return -ENODEV;
}

static int rk805_regulator_disable(int id)
{
	u8 val;

	if (id < 4) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_DCDC_EN_REG);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK805_DCDC_EN_REG,
			      (val | (1 << (id + 4))) & (~(1 << id)));

	} else if (id < 7) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_LDO_EN_REG);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK805_LDO_EN_REG,
			      (val | (1 << id)) & (~(1 << (id - 4))));
	}

	debug("disable %s %d dcdc_en = %08x ldo_en1 =%08x\n",
	      __func__, id,
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_DCDC_EN_REG),
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_LDO_EN_REG));

	return 0;
}

static int rk805_regulator_enable(int id)
{
	u8 val;

	if (id < 4) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_DCDC_EN_REG);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK805_DCDC_EN_REG,
			      (val | (1 << (id + 4))) | (1 << id));
	} else if (id < 7) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_LDO_EN_REG);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK805_LDO_EN_REG,
			      (val | (1 << id)) | (1 << (id - 4)));
	}

	debug("enable %s %d dcdc_en = %08x ldo_en1 =%08x\n",
	      __func__, id,
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_DCDC_EN_REG),
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK805_LDO_EN_REG));

	return 0;
}

int rk816_regulator_enable(int id)
{
	if (rk8xx->enable_regulator)
		return rk8xx->enable_regulator(id);

	return 0;
}

static int rk816_regulator_disable(int id)
{
	if (rk8xx->disable_regulator)
		return rk8xx->disable_regulator(id);

	return 0;
}

static int _rk816_regulator_disable(int id)
{
	u8 val;

	if (id < 4) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_DCDC_EN_REG1);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_DCDC_EN_REG1,
			      (val | (1 << (id + 4))) & (~(1 << id)));

	} else if (id < 8) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG1);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG1,
			      (val | (1 << id)) & (~(1 << (id - 4))));

	} else {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG2);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG2,
			      (val | (1 << (id - 4))) & (~(1 << (id - 8))));

	}

	debug("disable %s %d dcdc_en = %08x ldo_en1 =%08x ldo_en2 =%08x\n",
	      __func__, id,
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_DCDC_EN_REG1),
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG1),
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG2));

	return 0;
}

int _rk816_regulator_enable(int id)
{
	u8 val;

	if (id < 4) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_DCDC_EN_REG1);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_DCDC_EN_REG1,
			      (val | (1 << (id + 4))) | (1 << id));
	} else if (id < 8) {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG1);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG1,
			      (val | (1 << id)) | (1 << (id - 4)));
	} else {
		val = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG2);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG2,
			      (val | (1 << (id - 4))) | (1 << (id - 8)));
	}

	debug("enable %s %d dcdc_en = %08x ldo_en1 =%08x ldo_en2 =%08x\n",
	      __func__, id,
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_DCDC_EN_REG1),
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG1),
	      i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_LDO_EN_REG2));

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
		else if (min_uV < 1800000)
			vsel = 0x3b;
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

static int rk816_regulator_set_voltage(int id, int min_uV, int max_uV)
{
	u16 val;
	u8 ret;

	val = rk816_dcdc_select_min_voltage(min_uV, max_uV, id);

	if (id < 4) {
		if (id == 2)
			return 0;
		ret = i2c_reg_read(rk8xx->pmic->hw.i2c.addr,
				   rk816_BUCK_SET_VOL_REG(id));
		ret = ret & (~RK816_VOLT_MASK);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr,
			      rk816_BUCK_SET_VOL_REG(id), ret | val);
		debug("0 %s %d dcdc_vol = %08x\n", __func__, id, ret);
		ret = i2c_reg_read(rk8xx->pmic->hw.i2c.addr,
				   RK816_DCDC_EN_REG2);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr,
			      RK816_DCDC_EN_REG2, ret | 0x80);
	} else {
		ret = i2c_reg_read(rk8xx->pmic->hw.i2c.addr,
				   rk816_LDO_SET_VOL_REG(id));
		ret = ret & (~RK816_LDO_VSEL_MASK);
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr,
			      rk816_LDO_SET_VOL_REG(id), ret | val);
		debug("1 %s %d %d ldo_vol =%08x\n", __func__, id, val, ret);
	}

	return 0;
}

static int rk816_set_regulator_init(struct fdt_regulator_match *matches, int id)
{
	int ret = 0;

	if (matches->init_uV) {
		printf("%s %s init uV:%d\n", __func__, matches->name, matches->init_uV);
		ret = rk816_regulator_set_voltage(id, matches->init_uV,
						  matches->init_uV);
	} else if (matches->min_uV == matches->max_uV) {
		debug("%s: regulagor.%d fix uv\n", __func__, id);
		ret = rk816_regulator_set_voltage(id, matches->min_uV,
						  matches->max_uV);
	}
	if (matches->boot_on) {
		debug("%s: regulagor.%d boot on\n", __func__, id);
		ret = rk816_regulator_enable(id);
	}

	return ret;
}

static int rk816_pre_init_ldo(unsigned char bus, uchar addr)
{
	rk816_regulator_disable(9);

	/* disable boost5v and otg_en */
	i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_DCDC_EN_REG2, 0x60);

	return 0;
}

static int rk816_pre_init(unsigned char bus, uchar addr)
{
	int ret = 0;

	i2c_set_bus_num(bus);
	i2c_init(RK816_I2C_SPEED, addr);
	i2c_set_bus_speed(RK816_I2C_SPEED);

	if (rk8xx->pre_init_regulator)
		ret = rk8xx->pre_init_regulator(bus, addr);

	return ret;
}

char *pmic_get_rk8xx_id(unsigned char bus)
{
	return rk8xx->name;
}

void pmic_rk816_shut_down(void)
{
	u8 reg;

	i2c_set_bus_num(rk8xx->pmic->bus);
	i2c_init(RK816_I2C_SPEED, rk8xx->pmic->hw.i2c.addr);
	i2c_set_bus_speed(RK816_I2C_SPEED);
	reg = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_DEV_CTRL_REG);
	i2c_reg_write(rk8xx->pmic->hw.i2c.addr, RK816_DEV_CTRL_REG,
		      (reg | (0x1 << 0)));
}

static struct pmic_rk816 rk816 = {
	.name = "rk816",
	.reg_nums = RK816_NUM_REGULATORS,
	.reg_match = rk816_reg_matches,
	.reg1_match = rk816_reg1_matches,
	.pre_init_regulator = rk816_pre_init_ldo,
	.enable_regulator = _rk816_regulator_enable,
	.disable_regulator = _rk816_regulator_disable,
	.fg_init = fg_rk816_init,
};

static struct pmic_rk816 rk805 = {
	.name = "rk805",
	.reg_nums = RK805_NUM_REGULATORS,
	.reg_match = rk805_reg_matches,
	.reg1_match = rk805_reg1_matches,
	.enable_regulator = rk805_regulator_enable,
	.disable_regulator = rk805_regulator_disable,
};

struct of_device_id rk8xx_of_match[] = {
	{"rk816", "rockchip,rk816", .data = &rk816, },
	{"rk805", "rockchip,rk805", .data = &rk805, },
	{},
};

static struct pmic_rk816 *of_match_device(struct of_device_id *match,
					  const void *blob, int *node)
{
	if (!match)
		return NULL;

	for (; match->name[0] && match->compatible[0] && match->data; match++) {
		*node = fdt_node_offset_by_compatible(blob, g_i2c_node,
						      match->compatible);
		if (*node < 0)
			debug("can't find dts node for %s\n", match->name);
		else
			return (struct pmic_rk816 *)match->data;
	}

	return NULL;
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

	rk8xx = of_match_device(rk8xx_of_match, blob, &node);
	if (!rk8xx)
		return -1;

	if (!fdt_device_is_available(blob, node)) {
		debug("device %s is disabled\n", rk8xx->name);
		return -1;
	}

	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("pmic %s get fdt i2c failed\n", rk8xx->name);
		return ret;
	}

	rk8xx->pmic = pmic_alloc();
	rk8xx->node = node;
	rk8xx->pmic->hw.i2c.addr = addr;
	rk8xx->pmic->bus = bus;
	debug("%s i2c_bus:%d addr:0x%02x\n", rk8xx->name,
	      rk8xx->pmic->bus, rk8xx->pmic->hw.i2c.addr);

	ret = rk816_i2c_probe(bus, addr);
	if (ret < 0) {
		debug("pmic %s i2c probe failed\n", rk8xx->name);
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
			reg_match = rk8xx->reg_match;
		else
			reg_match = rk8xx->reg1_match;

		fdt_regulator_match(blob, nd, reg_match, rk8xx->reg_nums);
	}

	for (i = 0; i < rk8xx->reg_nums; i++) {
		regulator_init_pmic_matches[i].name = reg_match[i].name;
		ret = rk816_set_regulator_init(&reg_match[i], i);
	}

	fdtdec_decode_gpios(blob, node, "gpios", gpios, 2);

	return 0;
}

int pmic_rk816_init(unsigned char bus)
{
	int ret;
	if (!rk8xx || !rk8xx->pmic) {
		if (!gd->fdt_blob)
			return -1;

		ret = rk816_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}

	rk8xx->pmic->interface = PMIC_I2C;
	/*enable lcdc power ldo, and enable other ldo*/
	ret = rk816_pre_init(rk8xx->pmic->bus, rk8xx->pmic->hw.i2c.addr);
	if (ret < 0)
		return ret;

	if (rk8xx->fg_init)
		rk8xx->fg_init(rk8xx->pmic->bus, rk8xx->pmic->hw.i2c.addr);

	return 0;
}

/*
 * Why do we clear fall int when detect rise, and clear rise when detect fall ?
 * To solve the pwrkey long pressed situation.
 */
int pmic_rk816_poll_pwrkey_stat(void)
{
	u8 buf;
	static int fall_ever_detected = 0;

	i2c_set_bus_num(rk8xx->pmic->bus);
	i2c_init(RK816_I2C_SPEED, rk8xx->pmic->hw.i2c.addr);
	i2c_set_bus_speed(RK816_I2C_SPEED);

	buf = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_INT_STS_REG1);
	/* rising, clear falling */
	if (buf & PWRON_RISE_INT) {
		if (fall_ever_detected)
			i2c_reg_write(rk8xx->pmic->hw.i2c.addr,
				      RK816_INT_STS_REG1, PWRON_FALL_INT);
		/*
		 * Normally, there must be a falling before a rising, so if
		 * there is no falling ever detected before, possiblely the
		 * situation: we force shutdown by long pressed pwrkey and
		 * release pwrkey exactly when pmic power on. So this rising is
		 * something wrong, clear it and abandont state poll this time.
		 * We will get a normal state at next time.
		 */
		else
			i2c_reg_write(rk8xx->pmic->hw.i2c.addr,
				      RK816_INT_STS_REG1, PWRON_RISE_INT);
	}
	/* falling, clear rising */
	if (buf & PWRON_FALL_INT) {
		i2c_reg_write(rk8xx->pmic->hw.i2c.addr,
			      RK816_INT_STS_REG1, PWRON_RISE_INT);
		fall_ever_detected = 1;
	}

	buf = i2c_reg_read(rk8xx->pmic->hw.i2c.addr, RK816_INT_STS_REG1);

	return (buf & PWRON_FALL_INT);
}

