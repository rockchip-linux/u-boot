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
#include "../mfd/fusb302.h"

DECLARE_GLOBAL_DATA_PTR;

#define BQ25700_ID	0x25700
#define BQ25703_ID	0x25703

#define COMPAT_BQ25700	"ti,bq25700"
#define COMPAT_BQ25703	"ti,bq25703"

#define BQ25700_I2C_SPEED			100000
#define BQ25700_CHARGE_CURRENT_1500MA		0x5C0
#define BQ25700_SDP_INPUT_CURRENT_500MA		0xA00
#define BQ25700_DCP_INPUT_CURRENT_1500MA	0x1E00
#define BQ25700_DCP_INPUT_CURRENT_2000MA	0x2800
#define BQ25700_DCP_INPUT_CURRENT_3000MA	0x3C00

#define WATCHDOG_ENSABLE	(0x03 << 13)

#define BQ25700_CHARGEOPTION0_REG	0x12
#define BQ25700_CHARGECURREN_REG	0x14
#define BQ25700_CHARGERSTAUS_REG	0x20
#define BQ25700_INPUTVOLTAGE_REG	0x3D
#define BQ25700_INPUTCURREN_REG		0x3F

#define BQ25703_CHARGEOPTION0_REG	0x00
#define BQ25703_CHARGECURREN_REG	0x02
#define BQ25703_CHARGERSTAUS_REG	0x20
#define BQ25703_INPUTVOLTAGE_REG	0x0A
#define BQ25703_INPUTCURREN_REG		0x0E

enum bq25700_table_ids {
	/* range tables */
	TBL_ICHG,
	TBL_CHGMAX,
	TBL_INPUTVOL,
	TBL_INPUTCUR,
	TBL_SYSVMIN,
	TBL_OTGVOL,
	TBL_OTGCUR,
	TBL_EXTCON,
};

struct bq25700 {
	struct pmic *p;
	int node;
	struct fdt_gpio_state typec0_enable_gpio, typec1_enable_gpio;
	u32 ichg;
	u32 chip_id;
};

struct bq25700_range {
	u32 min;
	u32 max;
	u32 step;
};

static const union {
	struct bq25700_range  rt;
} bq25700_tables[] = {
	/* range tables */
	[TBL_ICHG] = { .rt = {0, 8128000, 64000} },
	/* uV */
	[TBL_CHGMAX] = { .rt = {0, 19200000, 16000} },
	/* uV  max charge voltage*/
	[TBL_INPUTVOL] = { .rt = {3200000, 19520000, 64000} },
	/* uV  input charge voltage*/
	[TBL_INPUTCUR] = {.rt = {0, 6350000, 50000} },
	/*uA input current*/
	[TBL_SYSVMIN] = { .rt = {1024000, 16182000, 256000} },
	/* uV min system voltage*/
	[TBL_OTGVOL] = {.rt = {4480000, 20800000, 64000} },
	/*uV OTG volage*/
	[TBL_OTGCUR] = {.rt = {0, 6350000, 50000} },
};

struct bq25700 charger;

static u32 bq25700_find_idx(u32 value, enum bq25700_table_ids id)
{
	u32 idx;
	u32 rtbl_size;
	const struct bq25700_range *rtbl = &bq25700_tables[id].rt;

	rtbl_size = (rtbl->max - rtbl->min) / rtbl->step + 1;

	for (idx = 1;
	     idx < rtbl_size && (idx * rtbl->step + rtbl->min <= value);
	     idx++)
		;

	return idx - 1;
}

static int bq25700_charger_status(struct pmic *p)
{
	u16 value;
#if defined(CONFIG_POWER_FUSB302)
	static u16 charge_flag;
#endif

	i2c_set_bus_num(p->bus);
	i2c_init(BQ25700_I2C_SPEED, 0);
	i2c_read(p->hw.i2c.addr, BQ25700_CHARGERSTAUS_REG, 1,
		 (u8 *)&value, 2);

	p->chrg->state_of_charger = value >> 15;

#if defined(CONFIG_POWER_FUSB302)
	if (p->chrg->state_of_charger)
		charge_flag = 1;
	else if (!p->chrg->state_of_charger && charge_flag == 1)
	{
		typec_discharge();
		charge_flag = 0;
	}
#endif

	return value >> 15;
}

static int bq25703_charger_status(struct pmic *p)
{
	u16 value;
	#if defined(CONFIG_POWER_FUSB302)
	static u16 charge_flag;
	#endif

	i2c_set_bus_num(p->bus);
	i2c_init(BQ25700_I2C_SPEED, 0);
	i2c_read(p->hw.i2c.addr, BQ25703_CHARGERSTAUS_REG, 1,
		 (u8 *)&value, 2);

	p->chrg->state_of_charger = value >> 15;

	#if defined(CONFIG_POWER_FUSB302)
	if (p->chrg->state_of_charger) {
		charge_flag = 1;
	} else {
		if (!p->chrg->state_of_charger && charge_flag == 1) {
			typec_discharge();
			charge_flag = 0;
		}
	}
	#endif

	return value >> 15;
}

static int bq257xx_charger_status(struct pmic *p)
{
	if (charger.chip_id == BQ25700_ID)
		bq25700_charger_status(p);
	else
		bq25703_charger_status(p);
}

static void bq25700_charger_current_init(struct pmic *p)
{
	u16 charge_current = BQ25700_CHARGE_CURRENT_1500MA;
	u16 sdp_inputcurrent = BQ25700_SDP_INPUT_CURRENT_500MA;
	u16 dcp_inputcurrent = BQ25700_DCP_INPUT_CURRENT_1500MA;
	u32 pd_inputcurrent = 0;
	u16 vol_idx, cur_idx, pd_inputvol;

	u16 temp;

	i2c_read(p->hw.i2c.addr, BQ25700_CHARGEOPTION0_REG, 1,
		 (u8 *)&temp, 2);
	temp &= (~WATCHDOG_ENSABLE);
	i2c_write(p->hw.i2c.addr, BQ25700_CHARGEOPTION0_REG, 1,
		  (u8 *)&temp, 2);

#if defined(CONFIG_POWER_FUSB302)
	if (!get_pd_output_val(&pd_inputvol, &pd_inputcurrent))
	{
		printf("%s pd charge input vol:%dmv current:%dma\n",
			__func__, pd_inputvol, pd_inputcurrent);
		vol_idx = bq25700_find_idx((pd_inputvol - 1280) * 1000, TBL_INPUTVOL);
		cur_idx = bq25700_find_idx(pd_inputcurrent * 1000, TBL_INPUTCUR);
		cur_idx  = cur_idx << 8;
		vol_idx = vol_idx << 6;
		if (pd_inputcurrent != 0) {
			i2c_write(p->hw.i2c.addr, BQ25700_INPUTCURREN_REG, 1,
				  (u8 *)&cur_idx, 2);
			i2c_write(p->hw.i2c.addr, BQ25700_INPUTVOLTAGE_REG, 1,
				  (u8 *)&vol_idx, 2);
			charge_current = bq25700_find_idx(charger.ichg, TBL_ICHG);
			charge_current = charge_current << 8;
		}
	}
#endif

	if (pd_inputcurrent == 0) {
		if (dwc_otg_check_dpdm() > 1)
			i2c_write(p->hw.i2c.addr, BQ25700_INPUTCURREN_REG, 1,
				  (u8 *)&dcp_inputcurrent, 2);
		else
			i2c_write(p->hw.i2c.addr, BQ25700_INPUTCURREN_REG, 1,
				  (u8 *)&sdp_inputcurrent, 2);
	}

	if (bq25700_charger_status(p))
		i2c_write(p->hw.i2c.addr, BQ25700_CHARGECURREN_REG, 1,
			  (u8 *)&charge_current, 2);
}

static void bq25703_charger_current_init(struct pmic *p)
{
	u16 charge_current = BQ25700_CHARGE_CURRENT_1500MA;
	u16 sdp_inputcurrent = BQ25700_SDP_INPUT_CURRENT_500MA;
	u16 dcp_inputcurrent = BQ25700_DCP_INPUT_CURRENT_1500MA;
	u32 pd_inputcurrent = 0;
	u16 vol_idx, cur_idx, pd_inputvol;

	u16 temp;

	i2c_read(p->hw.i2c.addr, BQ25703_CHARGEOPTION0_REG, 1,
		 (u8 *)&temp, 2);
	temp &= (~WATCHDOG_ENSABLE);
	i2c_write(p->hw.i2c.addr, BQ25703_CHARGEOPTION0_REG, 1,
		  (u8 *)&temp, 2);

	#if defined(CONFIG_POWER_FUSB302)
	if (!get_pd_output_val(&pd_inputvol, &pd_inputcurrent)) {
		printf("%s pd charge input vol:%dmv current:%dma\n",
		       __func__, pd_inputvol, pd_inputcurrent);
		vol_idx = bq25700_find_idx((pd_inputvol - 1280) * 1000,
					   TBL_INPUTVOL);
		cur_idx = bq25700_find_idx(pd_inputcurrent * 1000,
					   TBL_INPUTCUR);
		cur_idx  = cur_idx << 8;
		vol_idx = vol_idx << 6;
		if (pd_inputcurrent != 0) {
			i2c_write(p->hw.i2c.addr, BQ25703_INPUTCURREN_REG, 1,
				  (u8 *)&cur_idx, 2);
			i2c_write(p->hw.i2c.addr, BQ25703_INPUTVOLTAGE_REG, 1,
				  (u8 *)&vol_idx, 2);
			charge_current = bq25700_find_idx(charger.ichg,
							  TBL_ICHG);
			charge_current = charge_current << 8;
		}
	}
	#endif

	if (pd_inputcurrent == 0) {
		if (dwc_otg_check_dpdm() > 1)
			i2c_write(p->hw.i2c.addr, BQ25703_INPUTCURREN_REG, 1,
				  (u8 *)&dcp_inputcurrent, 2);
		else
		i2c_write(p->hw.i2c.addr, BQ25703_INPUTCURREN_REG, 1,
			  (u8 *)&sdp_inputcurrent, 2);
	}

	if (bq25703_charger_status(p))
		i2c_write(p->hw.i2c.addr, BQ25703_CHARGECURREN_REG, 1,
			  (u8 *)&charge_current, 2);
}

static struct power_chrg bq25700_charger_ops = {
	.state_of_charger = 0,
	.chrg_type =  bq257xx_charger_status,
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
	int node, node1;
	u32 bus, addr;
	int ret, port_num;

	node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_BQ25700);

	node1 = fdt_node_offset_by_compatible(blob,
					      0, COMPAT_BQ25703);

	if ((node < 0) && (node1 < 0)) {
		printf("Can't find dts node for charger bq25700\n");
		return -ENODEV;
	}

	if (node < 0) {
		node = node1;
		charger.chip_id = BQ25700_ID;
	} else
		charger.chip_id = BQ25703_ID;

	if (!fdt_device_is_available(blob, node)) {
		printf("device bq25700 is disabled\n");
		return -1;
	}

	charger.ichg = fdtdec_get_int(blob, node, "ti,charge-current", 0);

#if defined(CONFIG_POWER_FUSB302)
	fdtdec_decode_gpio(blob, node, "typec0-enable-gpios", &charger.typec0_enable_gpio);
	fdtdec_decode_gpio(blob, node, "typec1-enable-gpios", &charger.typec1_enable_gpio);

	if (gpio_is_valid(charger.typec1_enable_gpio.gpio) &&
		gpio_is_valid(charger.typec0_enable_gpio.gpio)) {
		port_num = get_pd_port_num();
		if (port_num == 0) {
			printf("fusb0 charge typec0:1 typec1:0\n");
			gpio_direction_output(charger.typec0_enable_gpio.gpio, 1);
			gpio_direction_output(charger.typec1_enable_gpio.gpio, 0);
		} else if (port_num == 1) {
			printf("fusb1 charge typec0:0 typec1:1\n");
			gpio_direction_output(charger.typec0_enable_gpio.gpio, 0);
			gpio_direction_output(charger.typec1_enable_gpio.gpio, 1);
		}
		udelay(1000 * 200);
	}
#endif

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

	if (charger.chip_id == BQ25700_ID)
		bq25700_charger_current_init(charger.p);
	else
		bq25703_charger_current_init(charger.p);
	return 0;
}
