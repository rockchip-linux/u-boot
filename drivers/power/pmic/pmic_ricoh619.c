/*
 *  Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * yxj <yxj@rock-chips.com>
 * 
 */

/*#define DEBUG*/
#include <common.h>
#include <fdtdec.h>
#include <power/battery.h>
#include <power/rockchip_power.h>
#include <power/ricoh619_pmic.h>
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


int ricoh619_poll_pwr_key_sta(void)
{
	i2c_set_bus_num(ricoh619.pmic->bus);
	i2c_init(CONFIG_SYS_I2C_SPEED, ricoh619.pmic->hw.i2c.addr);
	i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
	return i2c_reg_read(ricoh619.pmic->hw.i2c.addr, 0x14) & 0x01;	
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
	int node, nd;
	u32 bus, addr;
	int ret;

	node = fdt_node_offset_by_compatible(blob, g_i2c_node,
					COMPAT_RICOH_RICOH619);
	if (node < 0) {
		debug("can't find dts node for ricoh619\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, node)) {
		debug("device ricoh619 is disabled\n");
		return -1;
	}
	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("pmic ricoh619 get fdt i2c failed\n");
		return ret;
	}

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
		if (!gd->fdt_blob)
			return -1;

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
	i2c_reg_write(ricoh619.pmic->hw.i2c.addr,0x30,0x03);
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

