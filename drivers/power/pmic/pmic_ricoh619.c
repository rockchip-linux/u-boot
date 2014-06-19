/*
 *  Copyright (C) 2012 rockchips
 * yxj <yxj@rock-chips.com>
 * 
 */

/*#define DEBUG*/
#include <common.h>
#include <fdtdec.h>
#include <power/battery.h>
#include <power/ricoh619_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

#define SYSTEM_ON_VOL_THRESD			3650
struct pmic_ricoh619 ricoh619;

static struct battery batt_status;

static struct fdt_regulator_match ricoh619_regulator_matches[] = {
	{ .prop	= "ricoh619_dc1",},
	{ .prop = "ricoh619_dc2",},
	{ .prop = "ricoh619_dc3",},
	{ .prop = "ricoh619_dc4",},
	{ .prop = "ricoh619_dc5",},
	{ .prop = "ricoh619_ldo1",},
	{ .prop = "ricoh619_ldo2",},
	{ .prop = "ricoh619_ldo3",},
	{ .prop = "ricoh619_ldo4",},
	{ .prop = "ricoh619_ldo5",},
	{ .prop = "ricoh619_ldo6",},
	{ .prop = "ricoh619_ldo7",},
	{ .prop = "ricoh619_ldo8",},
	{ .prop = "ricoh619_ldo9",},
	{ .prop = "ricoh619_ldo10",},
	{ .prop = "ricoh619_ldortc1",},
	{ .prop = "ricoh619_ldortc2",},
};

static int ricoh619_set_voltage(struct ricoh619_regulator *ri,
				int min_uV, int max_uV, unsigned *selector);

#define RICOH619_REG(_id, _en_reg, _en_bit, _disc_reg, _disc_bit, _vout_reg, \
	_vout_mask, _ds_reg, _min_uv, _max_uv, _step_uV, _nsteps,    \
	_set_volt, _delay, _eco_reg, _eco_bit, _eco_slp_reg, _eco_slp_bit)  \
{								\
	.reg_en_reg	= _en_reg,				\
	.en_bit		= _en_bit,				\
	.reg_disc_reg	= _disc_reg,				\
	.disc_bit	= _disc_bit,				\
	.vout_reg	= _vout_reg,				\
	.vout_mask	= _vout_mask,				\
	.sleep_reg	= _ds_reg,				\
	.min_uV		= _min_uv,			\
	.max_uV		= _max_uv ,			\
	.step_uV	= _step_uV,				\
	.nsteps		= _nsteps,				\
	.delay		= _delay,				\
	.id		= RICOH619_ID_##_id,			\
	.eco_reg			=  _eco_reg,				\
	.eco_bit			=  _eco_bit,				\
	.eco_slp_reg		=  _eco_slp_reg,				\
	.eco_slp_bit		=  _eco_slp_bit,				\
	.set_voltage		= _set_volt			\
}

static struct ricoh619_regulator ricoh619_regulator_data[] = {
  	RICOH619_REG(DC1, 0x2C, 0, 0x2C, 1, 0x36, 0xFF, 0x3B,
		600000, 3500000, 12500, 0xE8, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(DC2, 0x2E, 0, 0x2E, 1, 0x37, 0xFF, 0x3C,
			600000, 3500000, 12500, 0xE8, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(DC3, 0x30, 0, 0x30, 1, 0x38, 0xFF, 0x3D,
			600000, 3500000, 12500, 0xE8, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(DC4, 0x32, 0, 0x32, 1, 0x39, 0xFF, 0x3E,
			600000, 3500000, 12500, 0xE8, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(DC5, 0x34, 0, 0x34, 1, 0x3A, 0xFF, 0x3F,
			600000, 3500000, 12500, 0xE8, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),
			
  	RICOH619_REG(LDO1, 0x44, 0, 0x46, 0, 0x4C, 0x7F, 0x58,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x48, 0, 0x4A, 0),

	RICOH619_REG(LDO2, 0x44, 1, 0x46, 1, 0x4D, 0x7F, 0x59,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x48, 1, 0x4A, 1),

  	RICOH619_REG(LDO3, 0x44, 2, 0x46, 2, 0x4E, 0x7F, 0x5A,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x48, 2, 0x4A, 2),

  	RICOH619_REG(LDO4, 0x44, 3, 0x46, 3, 0x4F, 0x7F, 0x5B,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x48, 3, 0x4A, 3),

  	RICOH619_REG(LDO5, 0x44, 4, 0x46, 4, 0x50, 0x7F, 0x5C,
			600000, 3500000, 25000, 0x74, ricoh619_set_voltage, 500,
			0x48, 4, 0x4A, 4),

  	RICOH619_REG(LDO6, 0x44, 5, 0x46, 5, 0x51, 0x7F, 0x5D,
			600000, 3500000, 25000, 0x74, ricoh619_set_voltage, 500,
			0x48, 5, 0x4A, 5),

  	RICOH619_REG(LDO7, 0x44, 6, 0x46, 6, 0x52, 0x7F, 0x5E,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(LDO8, 0x44, 7, 0x46, 7, 0x53, 0x7F, 0x5F,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(LDO9, 0x45, 0, 0x47, 0, 0x54, 0x7F, 0x60,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(LDO10, 0x45, 1, 0x47, 1, 0x55, 0x7F, 0x61,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(LDORTC1, 0x45, 4, 0x00, 0, 0x56, 0x7F, 0x00,
			1700000, 3500000, 25000, 0x48, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),

  	RICOH619_REG(LDORTC2, 0x45, 5, 0x00, 0, 0x57, 0x7F, 0x00,
			900000, 3500000, 25000, 0x68, ricoh619_set_voltage, 500,
			0x00, 0, 0x00, 0),
};

int ricoh619_check_charge(void)
{
	int ret = 0;
	get_power_bat_status(&batt_status);
	ret = batt_status.state_of_chrg ? 1: 0;
	if (batt_status.voltage_uV < SYSTEM_ON_VOL_THRESD) {
		ret = 1;
		printf("low power.....\n");
	}
    
    return ret;
}

static int pmic_charger_state(struct pmic *p, int state, int current)
{
    return 0;
}


int ricoh619_poll_pwr_key_sta(void)
{
	i2c_set_bus_num(ricoh619.pmic->bus);
	i2c_init(CONFIG_SYS_I2C_SPEED, ricoh619.pmic->hw.i2c.addr);
	i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
	return i2c_reg_read(ricoh619.pmic->hw.i2c.addr, 0x14) & 0x01;	
}

static int ricoh619_set_voltage( struct ricoh619_regulator *ri,
				int min_uV, int max_uV, unsigned *selector)
{
	int vsel;
	int ret;
	uint8_t vout_val;

	if ((min_uV < ri->min_uV) || (max_uV > ri->max_uV))
		return -EDOM;

	vsel = (min_uV - ri->min_uV + ri->step_uV - 1)/ri->step_uV;
	if (vsel > ri->nsteps)
		return -EDOM;

	if (selector)
		*selector = vsel;

	vout_val = (ri->vout_reg_cache & ~ri->vout_mask) |
				(vsel & ri->vout_mask);
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr, ri->vout_reg, vout_val);
	ri->vout_reg_cache = vout_val;

	return ret;
}

static int ricoh619_i2c_probe(u32 bus, u32 addr)
{
	uchar val;
	int ret;
	i2c_set_bus_num(bus);
	i2c_init (RICOH619_I2C_SPEED, 0);
	ret = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, 0x36);
	if ((val == 0xff) || (val == 0))
		return -ENODEV;
	else
		return 0;
	
}
int ricoh619_parse_dt(const void* blob)
{
	int node, parent, nd;
	u32 i2c_bus_addr, bus;
	int ret;
	fdt_addr_t addr;
	node = fdt_node_offset_by_compatible(blob, 0,
					COMPAT_RICOH_RICOH619);
	if (node < 0) {
		printf("can't find dts node for ricoh619\n");
		return -ENODEV;
	}

	addr = fdtdec_get_addr(blob, node, "reg");
	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}
	i2c_bus_addr = fdtdec_get_addr(blob, parent, "reg");
	bus = i2c_get_bus_num_fdt(i2c_bus_addr);
	ret = ricoh619_i2c_probe(bus, addr);
	if (ret < 0) {
		debug("pmic ricoh619 i2c probe failed\n");
		return ret;
	}
	nd = fdt_get_regulator_node(blob, node);
	if (nd < 0)
		printf("%s: Cannot find regulators\n", __func__);
	else
		fdt_regulator_match(blob, nd, ricoh619_regulator_matches,
					RICOH619_NUM_REGULATORS);
	ricoh619.pmic = pmic_alloc();
	ricoh619.node = node;
	ricoh619.pmic->hw.i2c.addr = addr;
	ricoh619.pmic->bus = bus;
	debug("ricoh619 i2c_bus:%d addr:0x%02x\n", ricoh619.pmic->bus,
		ricoh619.pmic->hw.i2c.addr);
	return 0;
	 
}

int pmic_ricoh619_init(unsigned char bus)
{
	int ret;
	if (!ricoh619.pmic) {
		ret = ricoh619_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}
	//enable lcdc power ldo
	ricoh619.pmic->interface = PMIC_I2C;
	i2c_set_bus_num(ricoh619.pmic->bus);
	i2c_init (RICOH619_I2C_SPEED, 0);
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr, 0xff, 0x00); /*for i2c protect*/
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr ,0x10,0x4c);// DIS_OFF_PWRON_TIM bit 0; OFF_PRESS_PWRON 6s; OFF_JUDGE_PWRON bit 1; ON_PRESS_PWRON bit 2s
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr,0x36,0xc8);// dcdc1 output 3.1v for vccio
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr,0x4c,0x54);// vout1 output 3.0v for vccio_pmu
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr,0x51,0x30);// ldo6 output 1.8v for VCC18_LCD
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr,0x52,0x04);//
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr,0x44,i2c_reg_read(ricoh619.pmic->hw.i2c.addr,0x44)|(3<<5));//ldo6 enable
	fg_ricoh619_init(ricoh619.pmic->bus, ricoh619.pmic->hw.i2c.addr);
	return 0;
}

void pmic_ricoh619_shut_down(void)
{
    i2c_set_bus_num(ricoh619.pmic->bus);
    i2c_init (CONFIG_SYS_I2C_SPEED, ricoh619.pmic->hw.i2c.addr);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
    i2c_reg_write(ricoh619.pmic->hw.i2c.addr, 0xe0, i2c_reg_read(ricoh619.pmic->hw.i2c.addr,0xe0) & 0xfe);
    i2c_reg_write(ricoh619.pmic->hw.i2c.addr, 0x0f, i2c_reg_read(ricoh619.pmic->hw.i2c.addr,0x0f) & 0xfe);   
    i2c_reg_write(ricoh619.pmic->hw.i2c.addr, 0x0e, i2c_reg_read(ricoh619.pmic->hw.i2c.addr,0x0e) | 0x01);  
}

