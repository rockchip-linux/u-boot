/*
 *  Copyright (C) 2012 rockchips
 *  zhangqing < zhangqing@rock-chips.com >
 * andy <yxj@rock-chips.com>
 *  for sample
 */

#define DEBUG
#include <common.h>
#include <power/rk818_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_rk818 rk818;

int support_dc_chg;

static struct fdt_regulator_match rk818_reg_matches[] = {
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

static int rk818_parse_dt(const void* blob)
{
	int node, nd;
	struct fdt_gpio_state gpios[2];
	u32 bus, addr;
	int ret;

	node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_ROCKCHIP_RK818);
	if (node < 0) {
		printf("can't find dts node for rk818\n");
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
	else
		fdt_regulator_match(blob, nd, rk818_reg_matches,
					RK818_NUM_REGULATORS);

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

