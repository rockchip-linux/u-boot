/*
 *  Copyright (C) 2012 rockchips
 *  zyw < zyw@rock-chips.com >
 * andy <yxj@rock-chips.com>
 *  for sample
 */

/*#define DEBUG*/
#include <common.h>
#include <power/rk808_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

struct pmic_rk808 rk808;

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

static int rk808_parse_dt(const void* blob)
{
	int node, nd;
	struct fdt_gpio_state gpios[2];
	u32 bus, addr;
	int ret;

	node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_ROCKCHIP_RK808);
	if (node < 0) {
		printf("can't find dts node for rk808\n");
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
	if (nd < 0)
		printf("%s: Cannot find regulators\n", __func__);
	else
		fdt_regulator_match(blob, nd, rk808_reg_matches,
					RK808_NUM_REGULATORS);

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
	i2c_reg_write(0x1b,0x23,i2c_reg_read(0x1b,0x23)|0x60);
	i2c_reg_write(0x1b,0x45,0x02);
	i2c_reg_write(0x1b,0x24,i2c_reg_read(0x1b,0x24)|0x28);

    return 0;
}


void pmic_rk808_shut_down(void)
{
	u8 reg;
	i2c_set_bus_num(0);
	reg = i2c_reg_read(0x1b, 0x4b);
	i2c_reg_write(0x1b, 0x4b, (reg |(0x1 <<3)));

}

