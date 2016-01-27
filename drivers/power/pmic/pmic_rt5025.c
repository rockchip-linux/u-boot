/*
 * Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * zhangqing < zhangqing@rock-chips.com >
 * andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define DEBUG
#include <common.h>
#include <errno.h>
#include <power/rockchip_power.h>
#include <power/rt5025_pmic.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_rt5025 rt5025;

static struct fdt_regulator_match rt5025_reg_matches[] = {
	{ .prop = "rt,rt5025-dcdc1",},
	{ .prop = "rt,rt5025-dcdc2",},
	{ .prop = "rt,rt5025-dcdc3",},
	{ .prop = "rt,rt5025-dcdc4",},
	{ .prop = "rt,rt5025-ldo1", },
	{ .prop = "rt,rt5025-ldo2", },
	{ .prop = "rt,rt5025-ldo3", },
	{ .prop = "rt,rt5025-ldo4", },
	{ .prop = "rt,rt5025-ldo5", },
	{ .prop = "rt,rt5025-ldo6", },
};

static int rt5025_i2c_probe(u32 bus, u32 addr)
{
	char val;
	int ret;

	i2c_set_bus_num(bus);
	i2c_init(RT5025_I2C_SPEED, 0);
	ret  = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, 0x00);
	if (val != 0x81)
		return -ENODEV;
	else
		return 0;
}

static int rt5025_parse_dt(const void* blob)
{
	int node, nd;
	u32 bus, addr;
	int ret;
	node = fdt_node_offset_by_compatible(blob,
					g_i2c_node, COMPAT_ROCKCHIP_RT5025);
	if (node < 0) {
		debug("can't find dts node for rt5025\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		debug("device rt5025 is disabled\n");
		return -1;
	}
	
	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("pmic rt5025 get fdt i2c failed\n");
		return ret;
	}

	nd = rt5025_i2c_probe(bus, addr);
	if (nd < 0) {
		debug("pmic rt5025 i2c probe failed\n");
		return -1;
	}

	fdt_regulator_match(blob, nd, rt5025_reg_matches,
					RT5025_NUM_REGULATORS);
	
	rt5025.pmic = pmic_alloc();
	rt5025.node = node;
	rt5025.pmic->hw.i2c.addr = addr;
	rt5025.pmic->bus = bus;
	debug("rt5025 i2c_bus:%d addr:0x%02x\n", rt5025.pmic->bus,
		rt5025.pmic->hw.i2c.addr);

	return 0;
}

static int rt5025_pre_init(unsigned char bus,uchar addr)
{
	debug("%s,line=%d\n", __func__,__LINE__);
	 
	i2c_set_bus_num(bus);
	i2c_init(RT5025_I2C_SPEED, addr);
	i2c_set_bus_speed(RT5025_I2C_SPEED);

	i2c_reg_write(addr, RT5025_REG_CHGCTL2, i2c_reg_read(addr, RT5025_REG_CHGCTL2) | 0xf0); /*set chg time*/
	i2c_reg_write(addr, 0x17, i2c_reg_read(addr, 0x17) & 0x1f); /*power off 2.8v*/
 	i2c_reg_write(addr, 0x52,i2c_reg_read(addr,0x52)|0x02); /*no action when vref*/
	i2c_reg_write(addr, 0x08, (i2c_reg_read(addr,0x08) & 0x03) |0x40); /*set arm 1.1v*/
	i2c_reg_write(addr, 0x09, (i2c_reg_read(addr,0x09) & 0x01) |0x20); /*set logic 1.1v*/
// 	i2c_reg_write(addr, RT5025_REG_LDOONOFF,
//		i2c_reg_read(addr, RT5025_REG_LDOONOFF) |RT5025_LDOEN_MASK6); /*enable ldo6*/
	return 0;
}

int pmic_rt5025_init(unsigned char bus)
{
	int ret;
	if (!rt5025.pmic) {
		if (!gd->fdt_blob)
			return -1;

		ret = rt5025_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}

	rt5025.pmic->interface = PMIC_I2C;
	//enable lcdc power ldo, and enable other ldo 
	ret = rt5025_pre_init(rt5025.pmic->bus, rt5025.pmic->hw.i2c.addr);
	if (ret < 0)
			return ret;
	fg_rt5025_init(rt5025.pmic->bus, rt5025.pmic->hw.i2c.addr);

	return 0;
}


void pmic_rt5025_shut_down(void)
{
	i2c_set_bus_num(rt5025.pmic->bus);
    	i2c_init (RT5025_I2C_SPEED, rt5025.pmic->hw.i2c.addr);
    	i2c_set_bus_speed(RT5025_I2C_SPEED);
	i2c_reg_write(rt5025.pmic->hw.i2c.addr, RT5025_REG_CHANNELH, 0x00);
	i2c_reg_write(rt5025.pmic->hw.i2c.addr, RT5025_REG_CHANNELH, 0x80);
	i2c_reg_write(rt5025.pmic->hw.i2c.addr, RT5025_REG_MISC3, i2c_reg_read(rt5025.pmic->hw.i2c.addr, RT5025_REG_MISC3) | 0x80);

}

