/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <fdtdec.h>
#include <power/battery.h>
#include <errno.h>
#include <asm/arch/rkplat.h>
#include <power/rockchip_power.h>
#include <i2c.h>

DECLARE_GLOBAL_DATA_PTR;

#define COMPAT_ROCKCHIP_CW201X "cw201x"
static int state_of_chrg = 0;
#define CW201X_I2C_ADDR		0x62
#define CW201X_I2C_CH		0
#define CW201X_I2C_SPEED	200000

#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6
#define REG_CONFIG              0x8
#define REG_MODE                0xA
#define REG_BATINFO             0x10


#define MODE_SLEEP_MASK         (0x3<<6)
#define MODE_SLEEP              (0x3<<6)
#define MODE_NORMAL             (0x0<<6)
#define MODE_QUICK_START        (0x3<<4)
#define MODE_RESTART            (0xf<<0)

#define CONFIG_UPDATE_FLG       (0x1<<1)
#define ATHD                    (0x0<<3)        //ATHD = 0%

static int volt_tab[6] = {3466, 3586, 3670, 3804, 4014, 4316};

struct cw201x {
	struct pmic *p;
	int node;
	struct fdt_gpio_state dc_det;
	int i2c_ch;
};

struct cw201x cw;

static int cw201x_i2c_probe(u32 bus, u32 addr)
{
	char val;
	int ret;
	i2c_set_bus_num(bus);
	i2c_init(CW201X_I2C_SPEED, 0);
	ret = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;
	val = i2c_reg_read(addr, REG_VERSION);
	if (val == 0xff)
		return -ENODEV;
	else
		return 0;
}
static int cw201x_parse_dt(const void* blob)
{
	int err;
	int node;
	u32 bus, addr;
	int ret;

	node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_ROCKCHIP_CW201X);
	if (node < 0) {
		printf("Can't find dts node for fuel guage cw201x\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		debug("device cw201x is disabled\n");
		return -1;
	}
	
	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		debug("fg cw201x get fdt i2c failed\n");
		return ret;
	}

	ret = cw201x_i2c_probe(bus, addr);
	if (ret < 0) {
		debug("fg cw201x i2c probe failed\n");
		return ret;
	}
	cw.p = pmic_alloc();
	cw.node = node;
	cw.p->hw.i2c.addr = addr;
	cw.p->bus = bus;
	err = fdtdec_decode_gpio(blob, cw.node, "dc_det_gpio", &cw.dc_det);
	if (err) {
		printf("decode dc_det_gpio err\n");
		return err;
	}
	cw.dc_det.flags = !(cw.dc_det.flags  & OF_GPIO_ACTIVE_LOW);
	return 0;
}


static int cw_read_word(int reg)
{
	u8 vall, valh;
	u16 val;
	valh = i2c_reg_read(CW201X_I2C_ADDR, reg);
	vall = i2c_reg_read(CW201X_I2C_ADDR, reg + 1);
	val = ((u16)valh << 8) | vall;
	return val;
}

static int cw_get_vol(void)
{
        u16 value16, value16_1, value16_2, value16_3;
        int voltage;

	value16 = cw_read_word(REG_VCELL);
	if (value16 < 0)
		return -1;
        
	value16_1 = cw_read_word(REG_VCELL);
	if (value16_1 < 0)
		return -1;

	value16_2 = cw_read_word(REG_VCELL);
        if (value16_2 < 0)
                return -1;
		
		
        if (value16 > value16_1) {	 
		value16_3 = value16;
		value16 = value16_1;
		value16_1 = value16_3;
        }
		
        if (value16_1 > value16_2) {
		value16_3 =value16_1;
		value16_1 =value16_2;
		value16_2 =value16_3;
	}
			
        if (value16 >value16_1) {	 
		value16_3 =value16;
		value16 =value16_1;
		value16_1 =value16_3;
        }			

        voltage = value16_1 * 305;
        return voltage/1000;
}

/*
for chack charger status in boot
return 0, no charger
return 1, charging
*/
static int cw201x_check_charge(void)
{
	int ret = 0;
	if (gpio_get_value(cw.dc_det.gpio) == cw.dc_det.flags)
		state_of_chrg = 2;
	else
		state_of_chrg = 0;
	ret = !!state_of_chrg;
	return ret;
}

static int get_capcity(int volt)
{
	int i = 0;
	int level0, level1;
	int cap;
	int step = 100 / (ARRAY_SIZE(volt_tab) -1);


	for (i = 0 ; i < ARRAY_SIZE(volt_tab); i++) {
		if (volt <= (volt_tab[i]))
			break;
	}

	if (i == 0) 
		return 0;
	
	level0 = volt_tab[i -1]; 
	level1 = volt_tab[i];

	cap = step * (i-1) + step *(volt - level0)/(level1 - level0);
	/*printf("cap%d step:%d level0 %d level1 %d  diff %d\n",
			cap, step, level0 ,level1, diff);*/
	return cap;
}

static int cw201x_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	if (gpio_get_value(cw.dc_det.gpio) == cw.dc_det.flags)
		state_of_chrg = 2;
	else
		state_of_chrg = 0;
	
	i2c_set_bus_num(cw.p->bus);
	i2c_init (CW201X_I2C_SPEED, 0);
	battery->voltage_uV = cw_get_vol();
	battery->capacity = get_capcity(battery->voltage_uV);
	battery->state_of_chrg = state_of_chrg;
	printf("%s capacity = %d, voltage_uV = %d,state_of_chrg=%d\n",
		bat->name,battery->capacity,battery->voltage_uV,
		battery->state_of_chrg);
	return 0;
}

static int cw201x_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = cw201x_check_charge();
	return 0;
}

static struct power_fg cw201x_fg_ops = {
	.fg_battery_check = cw201x_check_battery,
	.fg_battery_update = cw201x_update_battery,
};


static int fg_cw201x_cfg(void)
{
	u8 val = MODE_SLEEP;
	u8 addr = cw.p->hw.i2c.addr;

	i2c_set_bus_num(cw.p->bus);
	i2c_init (CW201X_I2C_SPEED, 0);
        if ((val & MODE_SLEEP_MASK) == MODE_SLEEP) {
                val = MODE_NORMAL;
                i2c_reg_write(addr, REG_MODE, val);
        }

        return 0;

}
int fg_cw201x_init(unsigned char bus)
{
	static const char name[] = "CW201X_FG";
	int ret;
	if (!cw.p) {
		if (!gd->fdt_blob)
			return -1;

		ret = cw201x_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}

	cw.p->name = name;
	cw.p->interface = PMIC_I2C;
	cw.p->fg = &cw201x_fg_ops;
	cw.p->pbat = calloc(sizeof(struct  power_battery), 1);
	fg_cw201x_cfg();
	return 0;
}

