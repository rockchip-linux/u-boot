/*
 *  Copyright (C) 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *  csq <csq@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/battery.h>
#include <power/max8997_pmic.h>
#include <errno.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define EC_GET_VERSION_COMMOND		0x10
#define EC_GET_VERSION_INFO_NUM		(5)
#define EC_GET_BATTERY_INFO_COMMOND	0x07
#define EC_GET_PARAMETER_NUM		(13)
#define EC_GET_BATTERY_OTHER_COMMOND	0x08
#define EC_GET_BATTERYINFO_NUM		(7)

#define EC_GET_BIT(a, b)	(((a) & (1 << (b))) ? 1 : 0)
#define EC_IS_BATTERY_IN(a)	EC_GET_BIT(a, 6)

struct ec_battery_t {
	struct pmic	*pmic;
	int		node;	/*device tree node*/
	unsigned char	bus;
	int		i2c_speed;
	u32		i2c_addr;
	int		virtual_power;
};

struct ec_battery_t ec_bat;

static int ec_battery_check(struct pmic *p, struct pmic *bat)
{
	return 0;
}

static int ec_battery_update(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	u8 buf[13] = {0};
	int rem_cap, full_cap, soc, vol, status;
	static u16 i = 0;

	i2c_set_bus_num(bat->bus);
	i2c_init(200000, bat->hw.i2c.addr);

	battery->voltage_uV = 4200;
	battery->capacity = 66;
	battery->isexistbat = 1;
	/* bat exist and fg init success: report data */
	if (!ec_bat.virtual_power) {
		i2c_read(ec_bat.pmic->hw.i2c.addr, 0x07, 1, buf,
			 EC_GET_PARAMETER_NUM);
		if ((EC_GET_PARAMETER_NUM - 1) == buf[0]) {
			status = buf[2] << 8 | buf[1];
			vol = (buf[8] << 8 | buf[7]) * 1000;
			rem_cap = buf[6] << 8 | buf[5];
			full_cap = buf[10] << 8 | buf[9];
			soc = (rem_cap + full_cap / 101) * 100 / full_cap;
			if (soc > 100)
				soc = 100;
			else if (soc < 0)
				soc = 0;

			battery->voltage_uV = vol;
			battery->capacity = soc;
			battery->isexistbat = EC_IS_BATTERY_IN(status);
		}
	}
	if ((i++) % 2000 == 0)
		printf("ec_battery: capacity = %d\n", battery->capacity);

	return 0;
}

static struct power_fg ec_fg_ops = {
	.fg_battery_check = ec_battery_check,
	.fg_battery_update = ec_battery_update,
};

static int ec_battery_i2c_probe(u32 bus, u32 addr)
{
	int ret;

	i2c_set_bus_num(bus);
	i2c_init(200000, 0);
	ret  = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;

	return 0;
}

int ec_battery_init(void)
{
	int node;
	int ret;
	u32 bus, addr;
	static const char name[] = "EC_FG";
	const void* blob;
	struct fdt_gpio_state ec_notify;

	if (!gd->fdt_blob)
		return -1;

	blob = gd->fdt_blob;
	node = fdt_node_offset_by_compatible(blob, 0, "rockchip,ec-battery");
	if (node < 0) {
		printf("can't find dts node for ec-battery\n");
		return -ENODEV;
	}

	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		printf("ec battery get fdt i2c failed\n");
		return ret;
	}

	ret = ec_battery_i2c_probe(bus, addr);
	if (ret < 0) {
		printf("ec battery i2c probe failed\n");
		return ret;
	}

	fdtdec_decode_gpio(blob, node, "ec-notify-gpios", &ec_notify);
	if (gpio_is_valid(ec_notify.gpio)) {
		gpio_direction_output(ec_notify.gpio, 0);
		gpio_set_value(ec_notify.gpio, 0);
	}

	ec_bat.virtual_power = fdtdec_get_int(blob, node, "virtual_power", 0);

	ec_bat.node = node;
	ec_bat.pmic = pmic_alloc();
	ec_bat.pmic->name = name;
	ec_bat.pmic->bus = bus;
	ec_bat.pmic->hw.i2c.addr = addr;
	ec_bat.pmic->interface = PMIC_I2C;
	ec_bat.pmic->fg = &ec_fg_ops;
	ec_bat.pmic->pbat = calloc(sizeof(struct power_battery), 1);

	return 0;
}
