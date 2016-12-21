/*
 * (C) Copyright 2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/rkplat.h>
#include <common.h>
#include <errno.h>
#include <fdtdec.h>
#include <i2c.h>
#include <malloc.h>
#include <power/rockchip_power.h>

DECLARE_GLOBAL_DATA_PTR;

#define COMPAT_BQ25700	"ti,bq25700"

#define BQ25700_I2C_SPEED			100000
#define BQ25700_CHARGE_CURRENT_1500MA		0x5C0
#define BQ25700_SDP_INPUT_CURRENT_500MA		0xA00
#define BQ25700_DCP_INPUT_CURRENT_1500MA	0x1E00

#define WATCHDOG_ENSABLE	(0x03 << 13)

#define BQ25700_CHARGEOPTION0_REG	0x12
#define BQ25700_CHARGECURREN_REG	0x14
#define BQ25700_CHARGERSTAUS_REG	0x20
#define BQ25700_INPUTCURREN_REG		0x3F

struct bq25700 {
	struct pmic *p;
	int node;
};

struct bq25700 charger;

static int bq25700_charger_status(struct pmic *p)
{
	u16 value;

	i2c_set_bus_num(p->bus);
	i2c_init(BQ25700_I2C_SPEED, 0);
	i2c_read(p->hw.i2c.addr, BQ25700_CHARGERSTAUS_REG, 1,
		 (u8 *)&value, 2);

	p->chrg->state_of_charger = value >> 15;

	return value >> 15;
}

static void bq25700_charger_current_init(struct pmic *p)
{
	u16 charge_current = BQ25700_CHARGE_CURRENT_1500MA;
	u16 sdp_inputcurrent = BQ25700_SDP_INPUT_CURRENT_500MA;
	u16 dcp_inputcurrent = BQ25700_DCP_INPUT_CURRENT_1500MA;
	u16 temp;

	i2c_read(p->hw.i2c.addr, BQ25700_CHARGEOPTION0_REG, 1,
		 (u8 *)&temp, 2);
	temp &= (~WATCHDOG_ENSABLE);
	i2c_write(p->hw.i2c.addr, BQ25700_CHARGEOPTION0_REG, 1,
		  (u8 *)&temp, 2);

	if (dwc_otg_check_dpdm() > 1)
		i2c_write(p->hw.i2c.addr, BQ25700_INPUTCURREN_REG, 1,
			  (u8 *)&dcp_inputcurrent, 2);
	else
		i2c_write(p->hw.i2c.addr, BQ25700_INPUTCURREN_REG, 1,
			  (u8 *)&sdp_inputcurrent, 2);

	if (bq25700_charger_status(p))
		i2c_write(p->hw.i2c.addr, BQ25700_CHARGECURREN_REG, 1,
			  (u8 *)&charge_current, 2);
}

static struct power_chrg bq25700_charger_ops = {
	.state_of_charger = 0,
	.chrg_type =  bq25700_charger_status,
};

static int bq25700_i2c_probe(u32 bus, u32 addr)
{
	int ret;

	i2c_set_bus_num(bus);
	i2c_init(BQ25700_I2C_SPEED, 0);
	ret = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;

	return 0;
}

static int bq25700_parse_dt(const void *blob)
{
	int node;
	u32 bus, addr;
	int ret;

	node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_BQ25700);
	if (node < 0) {
		printf("Can't find dts node for charger bq25700\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, node)) {
		printf("device bq25700 is disabled\n");
		return -1;
	}

	ret = fdt_get_i2c_info(blob, node, &bus, &addr);
	if (ret < 0) {
		printf("fg bq25700 get fdt i2c failed\n");
		return ret;
	}

	ret = bq25700_i2c_probe(bus, addr);
	if (ret < 0) {
		printf("fg bq25700 i2c probe failed\n");
		return ret;
	}

	charger.p = pmic_alloc();
	charger.node = node;
	charger.p->hw.i2c.addr = addr;
	charger.p->bus = bus;

	return 0;
}

int charger_bq25700_init(void)
{
	static const char name[] = "BQ25700_CHARGER";
	int ret;

	if (!charger.p) {
		if (!gd->fdt_blob)
			return -1;

		ret = bq25700_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}

	charger.p->name = name;
	charger.p->interface = PMIC_I2C;
	charger.p->chrg = &bq25700_charger_ops;
	charger.p->pbat = calloc(sizeof(struct  power_battery), 1);

	bq25700_charger_current_init(charger.p);

	return 0;
}
