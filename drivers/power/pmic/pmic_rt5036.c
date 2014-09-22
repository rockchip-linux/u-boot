/*
 *  Copyright (C) 2012 rockchips
 *  zhangqing < zhangqing@rock-chips.com >
 *  for sample
 */

#define DEBUG
#include <common.h>
#include <power/rt5036_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_rt5036 rt5036;

/*#define CONFIG_RT_SUPPORT_ACUSB_DUALIN*/
/*#define CONFIG_RT_SUPPORT_4V35_BAT*/


static struct fdt_regulator_match rt5036_reg_matches[] = {
	{ .prop = "rt,rt5036-dcdc1",},
	{ .prop = "rt,rt5036-dcdc2",},
	{ .prop = "rt,rt5036-dcdc3",},
	{ .prop = "rt,rt5036-dcdc4",},
	{ .prop = "rt,rt5036-ldo1", },
	{ .prop = "rt,rt5036-ldo2", },
	{ .prop = "rt,rt5036-ldo3", },
	{ .prop = "rt,rt5036-ldo4", },
	{ .prop = "rt,rt5036-lsw1", },
	{ .prop = "rt,rt5036-lsw2", },
};

static int rt5036_i2c_probe(u32 bus, u32 addr)
{
	char val;
	int ret;

	i2c_set_bus_num(bus);
	i2c_init(RT5036_I2C_SPEED, 0);
	ret  = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, 0x00);
	if (val != 0x36)
		return -ENODEV;
	else
		return 0;
}

static int rt5036_parse_dt(const void* blob)
{
	int node, parent, nd;
	u32 i2c_bus_addr, bus;
	int ret;
	fdt_addr_t addr;
	struct fdt_gpio_state gpios[2];
	node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_ROCKCHIP_RT5036);
	if (node < 0) {
		printf("can't find dts node for rt5036\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		debug("device rt5036 is disabled\n");
		return -1;
	}
	
	addr = fdtdec_get_addr(blob, node, "reg");
	/*fdtdec_decode_gpios(blob, node, "gpios", gpios, 2);*/
	
	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		printf("%s: Cannot find node parent\n", __func__);
		return -1;
	}
	i2c_bus_addr = fdtdec_get_addr(blob, parent, "reg");
	bus = i2c_get_bus_num_fdt(i2c_bus_addr);
	ret = rt5036_i2c_probe(bus, addr);
	if (ret < 0) {
		printf("pmic rt5036 i2c probe failed\n");
		return ret;
	}
	
	fdt_regulator_match(blob, parent, rt5036_reg_matches,
					RT5036_NUM_REGULATORS);
	
	rt5036.pmic = pmic_alloc();
	rt5036.node = node;
	rt5036.pmic->hw.i2c.addr = addr;
	rt5036.pmic->bus = bus;
	printf("rt5036 i2c_bus:%d addr:0x%02x\n", rt5036.pmic->bus,
		rt5036.pmic->hw.i2c.addr);
	return 0;
	 
}

static int rt5036_pre_init(unsigned char bus,uchar addr)
{
	printf("%s,line=%d\n", __func__,__LINE__);
	 
	i2c_set_bus_num(bus);
	i2c_init(RT5036_I2C_SPEED, addr);
	i2c_set_bus_speed(RT5036_I2C_SPEED);

	#if defined (CONFIG_RT_SUPPORT_4V35_BAT)
	i2c_reg_write(addr, RT5025_REG_CHGCTL2,
			(i2c_reg_read(addr, RT5025_REG_CHGCTL2) & 0x03) | 0x70);
	#endif
	
	i2c_reg_write(addr, RT5036_REG_LSWEN,
		i2c_reg_read(addr, RT5036_REG_LSWEN) |RT5036_LSWNEN_MASK1); /*enable lsw1*/

// 	i2c_reg_write(addr, RT5036_REG_BUCKLDONEN,
//		i2c_reg_read(addr, RT5036_REG_BUCKLDONEN) |RT5036_LDOEN_MASK4); /*enable ldo4*/
	return 0;
}


int pmic_rt5036_init(unsigned char bus)
{
	int ret;
	if (!rt5036.pmic) {
		ret = rt5036_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}

	rt5036.pmic->interface = PMIC_I2C;
	//enable lcdc power ldo, and enable other ldo 
	ret = rt5036_pre_init(rt5036.pmic->bus, rt5036.pmic->hw.i2c.addr);
	if (ret < 0)
			return ret;
	fg_rt5036_init(rt5036.pmic->bus, rt5036.pmic->hw.i2c.addr);

	return 0;
}


void pmic_rt5036_shut_down(void)
{
	i2c_set_bus_num(rt5036.pmic->bus);
    	i2c_init (RT5036_I2C_SPEED, rt5036.pmic->hw.i2c.addr);
    	i2c_set_bus_speed(RT5036_I2C_SPEED);
	i2c_reg_write(rt5036.pmic->hw.i2c.addr, RT5036_REG_MISC3, i2c_reg_read(rt5036.pmic->hw.i2c.addr, RT5036_REG_MISC3) | 0x80);
	i2c_reg_write(rt5036.pmic->hw.i2c.addr, RT5036_REG_MISC3, i2c_reg_read(rt5036.pmic->hw.i2c.addr, RT5036_REG_MISC3) & 0x7f);

}

