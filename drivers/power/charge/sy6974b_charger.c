/*
 * (C) Copyright 2022 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <dm.h>
#include <i2c.h>
#include <irq-generic.h>
#include <power/fuel_gauge.h>
#include <linux/delay.h>
#include <linux/usb/phy-rockchip-usb2.h>
#ifdef CONFIG_DM_POWER_DELIVERY
#include <power/power_delivery/power_delivery.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static int dbg_enable;

#define CHDBG(fmt, args...) \
	do { \
		if (dbg_enable) { \
			printf("CH-SY6974: " fmt, ##args); \
		} \
	} while (0)

/* define register */
#define REG_0 0x00
#define REG_1 0x01
#define REG_2 0x02
#define REG_3 0x03
#define REG_4 0x04
#define REG_5 0x05
#define REG_6 0x06
#define REG_7 0x07
#define REG_8 0x08
#define REG_9 0x09
#define REG_A 0x0a
#define REG_B 0x0b

/* charge contrl */
#define CHRG_EN             BIT(4)
#define HIZ_EN_MASK         BIT(7)
#define TERM_EN_MASK        BIT(7)
#define VAC_OVP_MASK        GENMASK(7, 6)
#define VBUS_GOOD_MASK      BIT(7)
#define BOOSTV_MASK         GENMASK(5, 4)
#define BOOST_LIM_MIN       BIT(7)
#define OTG_EN_MASK         BIT(5)
#define OTG_EN              BIT(5)
/* Part ID */
#define PN_MASK             GENMASK(6, 3)
/* WDT TIMER SET */
#define WDT_TIMER_MASK      GENMASK(5, 4)
#define WDT_TIMER_DISABLE   0
#define WDT_TIMER_40S       BIT(4)
#define WDT_TIMER_80S       BIT(5)
#define WDT_TIMER_160S      (BIT(4) | BIT(5))
#define WDT_RST_MASK        BIT(6)
#define WDT_RST             BIT(6)
/* recharge voltage */
#define VRECHARGE_MASK      BIT(0)
#define VRECHRG_STEP        100  // mv
#define VRECHRG_OFFSET      100  // mv
#define VRECHRG_DEF         200  // mv
/* charge status */
#define VSYS_STAT_MASK      BIT(0)
#define THERM_STAT_MASK     BIT(1)
#define PG_STAT_MASK        BIT(2)
#define CHG_STAT_MASK       GENMASK(4, 3)
#define PRECHRG_STAT        BIT(3)
#define FAST_CHRG_STAT      BIT(4)
#define TERM_CHRG_STAT      (BIT(3) | BIT(4))
#define NOT_CHRGING_STAT    0
#define CHG_FAULT_MASK      GENMASK(5, 4)
/* charge type */
#define VBUS_STAT_MASK      GENMASK(7, 5)
#define USB_SDP             BIT(5)
#define USB_CDP             BIT(6)
#define USB_DCP             (BIT(5) | BIT(6))
#define UNKNOWN             (BIT(7) | BIT(5))
#define NON_STANDARD        (BIT(7) | BIT(6))
#define OTG_MODE            (BIT(7) | BIT(6) | BIT(5))
/* TEMP Status */
#define TEMP_STAT_MASK      GENMASK(2, 0)
#define TEMP_NORMAL         BIT(0)
#define TEMP_WARM           BIT(1)
#define TEMP_COOL           (BIT(0) | BIT(1))
#define TEMP_COLD           (BIT(0) | BIT(2))
#define TEMP_HOT            (BIT(1) | BIT(2))
/* precharge current */
#define PRECHRG_I_LIM_MASK  GENMASK(7, 4)
#define PRECHRG_I_LIM_STEP  60000   // uA
#define PRECHRG_I_LIM_MIN   60000   // uA
#define PRECHRG_I_LIM_MAX   780000  // uA
#define PRECHRG_I_LIM_DEF   180000  // uA
/* termination current */
#define TERMCHRG_I_LIM_MASK GENMASK(3, 0)
#define TERMCHRG_I_LIM_STEP 60000   // uA
#define TERMCHRG_I_LIM_MIN  60000   // uA
#define TERMCHRG_I_LIM_MAX  960000  // uA
#define TERMCHRG_I_LIM_DEF  240000  // uA
/* charge current */
#define ICHRG_I_MASK        GENMASK(5, 0)
#define ICHRG_I_STEP        60000    // uA
#define ICHRG_I_MIN         0        // uA
#define ICHRG_I_MAX         2000000  // uA
#define ICHRG_I_DEF         1000000  // uA
#define ICHRG_I_JEITA_MASK  BIT(0)
/* charge voltage */
#define VREG_V_MASK         GENMASK(7, 3)
#define VREG_V_MAX          4500000  // uV
#define VREG_V_MIN          3856000  // uV
#define VREG_V_DEF          4500000  // uV
#define VREG_V_STEP         32000    // uV
/* iindpm current */
#define IINDPM_I_MASK       GENMASK(4, 0)
#define IINDPM_I_MIN        100000   // uA
#define IINDPM_I_MAX        2000000  // uA
#define IINDPM_STEP         100000   // uA
#define IINDPM_DEF          500000   // uA
#define VINDPM_INT_MASK     BIT(1)
#define VINDPM_INT_DIS      BIT(1)
#define IINDPM_INT_MASK     BIT(0)
#define IINDPM_INT_DIS      BIT(0)
/* vindpm voltage */
#define VINDPM_V_MASK       GENMASK(3, 0)
#define VINDPM_V_MIN        3900000  // uV
#define VINDPM_OFFSET       3900000  // uV
#define VINDPM_V_MAX        5400000  // uV
#define VINDPM_STEP         100000   // uV
#define VINDPM_DEF          4700000  // uV

struct sy6974b {
	struct udevice *dev;
	struct udevice *pd;
	bool pd_online;
	u32 init_count;
	u32 ichg;
	u32 vchg;
	int irq;
};

enum power_supply_type {
	POWER_SUPPLY_TYPE_UNKNOWN = 0,
	POWER_SUPPLY_TYPE_USB,          /* Standard Downstream Port */
	POWER_SUPPLY_TYPE_USB_DCP,      /* Dedicated Charging Port */
	POWER_SUPPLY_TYPE_USB_CDP,      /* Charging Downstream Port */
	POWER_SUPPLY_TYPE_USB_FLOATING, /* DCP without shorting D+/D- */
};

static int sy6974b_read(struct sy6974b *charger, uint reg, u8 *buffer)
{
	u8 val;
	int ret;

	ret = dm_i2c_read(charger->dev, reg, &val, 1);
	if (ret) {
		printf("CH-SY6974: read %#x error, ret=%d", reg, ret);
		return ret;
	}

	*buffer = val;
	return 0;
}

static int sy6974b_write(struct sy6974b *charger, uint reg, u8 val)
{
	int ret;

	ret = dm_i2c_write(charger->dev, reg, &val, 1);
	if (ret) {
		printf("CH-SY6974: write %#x error, ret=%d", reg, ret);
	}

	return ret;
}

static int sy6974b_update_bits(struct sy6974b *charger, u8 offset, u8 mask, u8 val)
{
	u8 reg;

	sy6974b_read(charger, offset, &reg);
	reg &= ~mask;

	return sy6974b_write(charger, offset, reg | val);
}

static int sy6974b_set_input_curr_lim(struct sy6974b *charger, int iindpm)
{
	u8 reg_val;
	int ret;

	if (iindpm < IINDPM_I_MIN) {
		iindpm = IINDPM_I_MIN;
	}
	if (iindpm >= IINDPM_I_MAX) {
		iindpm = IINDPM_I_MAX;
	}

	reg_val = (iindpm - IINDPM_I_MIN) / IINDPM_STEP;
	ret = sy6974b_update_bits(charger, REG_0, IINDPM_I_MASK, reg_val);
	if (ret) {
		printf("CH-SY6974: set charge input current error!\n");
	}

	return ret;
}

static int sy6974b_get_usb_type(void)
{
#ifdef CONFIG_PHY_ROCKCHIP_INNO_USB2
	return rockchip_chg_get_type();
#else
	return 0;
#endif
}

static int sy6974b_charger_capability(struct udevice *dev)
{
	return FG_CAP_CHARGER;
}

static int sy6974b_set_ichrg_curr(struct sy6974b *charger, int uA)
{
	u8 reg_val;
	int ret;

	if (uA < ICHRG_I_MIN) {
		uA = ICHRG_I_MIN;
	} else if (uA > charger->ichg) {
		uA = charger->ichg;
	}

	reg_val = uA / ICHRG_I_STEP;
	ret = sy6974b_update_bits(charger, REG_2, ICHRG_I_MASK, reg_val);
	if (ret) {
		printf("CH-SY6974: set icharge current error!\n");
	}

	return ret;
}

static int sy6974b_set_prechrg_curr(struct sy6974b *charger, int uA)
{
	u8 reg_val;
	int ret;

	if (uA < PRECHRG_I_LIM_MIN) {
		uA = PRECHRG_I_LIM_MIN;
	} else if (uA > PRECHRG_I_LIM_MAX) {
		uA = PRECHRG_I_LIM_MAX;
	}

	reg_val = (uA - PRECHRG_I_LIM_MIN) / PRECHRG_I_LIM_STEP;
	reg_val = reg_val << 4;
	ret = sy6974b_update_bits(charger, REG_3, PRECHRG_I_LIM_MASK, reg_val);
	if (ret) {
		printf("CH-SY6974: set pre charge current error!\n");
	}

	return ret;
}

static int sy6974b_set_chrg_volt(struct sy6974b *charger, int chrg_volt)
{
	u8 reg_val;
	int ret;

	if (chrg_volt < VREG_V_MIN) {
		chrg_volt = VREG_V_MIN;
	} else if (chrg_volt > VREG_V_MAX) {
		chrg_volt = VREG_V_MAX;
	}

	reg_val = (chrg_volt - VREG_V_MIN) / VREG_V_STEP;
	reg_val = reg_val << 3;
	ret = sy6974b_update_bits(charger, REG_4, VREG_V_MASK, reg_val);
	if (ret) {
		printf("CH-SY6974: set charge volt error!\n");
	}

	return ret;
}

static int sy6974b_set_charger_voltage(struct udevice *dev, int uV)
{
	struct sy6974b *charger = dev_get_priv(dev);

	CHDBG("set charge voltage: %duV\n", uV);
	return sy6974b_set_chrg_volt(charger, uV);
}

static int sy6974b_charger_enable(struct udevice *dev)
{
	struct sy6974b *charger = dev_get_priv(dev);

	sy6974b_update_bits(charger, REG_1, CHRG_EN, CHRG_EN);
	return 0;
}

static int sy6974b_charger_disable(struct udevice *dev)
{
	struct sy6974b *charger = dev_get_priv(dev);

	sy6974b_update_bits(charger, REG_1, CHRG_EN, 0);
	return 0;
}

static int sy6974b_iprechg_current(struct udevice *dev, int iprechrg_uA)
{
	struct sy6974b *charger = dev_get_priv(dev);

	CHDBG("set charge pre current: %duA\n", iprechrg_uA);

	return sy6974b_set_prechrg_curr(charger, iprechrg_uA);
}

static int sy6974b_charger_current(struct udevice *dev, int ichrg_uA)
{
	struct sy6974b *charger = dev_get_priv(dev);

	CHDBG("set charge current: %duA\n", ichrg_uA);

	return sy6974b_set_ichrg_curr(charger, ichrg_uA);
}

static int sy6974b_get_pd_output_val(struct sy6974b *charger, int *vol, int *cur)
{
#ifdef CONFIG_DM_POWER_DELIVERY
	struct power_delivery_data pd_data;
	int ret;

	if (!charger->pd) {
		return -EINVAL;
	}

	memset(&pd_data, 0, sizeof(pd_data));
	ret = power_delivery_get_data(charger->pd, &pd_data);
	if (ret) {
		return ret;
	}
	if (!pd_data.online || !pd_data.voltage || !pd_data.current) {
		return -EINVAL;
	}

	*vol = pd_data.voltage;
	*cur = pd_data.current;
	charger->pd_online = pd_data.online;

	return 0;
#else
	return -ENOSYS;
#endif
}

static void sy6974b_charger_input_current_init(struct sy6974b *charger)
{
	// int sdp_inputcurrent = 500 * 1000;   // 500mA
	int dcp_inputcurrent = 1000 * 1000;  // 1A
	int pd_inputvol, pd_inputcurrent;
	int ret;

	if (!charger->pd) {
		ret = uclass_get_device(UCLASS_PD, 0, &charger->pd);
		if (ret) {
			if (ret == -ENODEV) {
				printf("CH-SY6974: Can't find PD!\n");
			} else {
				printf("CH-SY6974: Get UCLASS PD failed: %d\n", ret);
			}
			charger->pd = NULL;
		}
	}

	if (!sy6974b_get_pd_output_val(charger, &pd_inputvol, &pd_inputcurrent)) {
		printf("CH-SY6974: charger set pd input v: %d, c: %d fixed c: %d\n", pd_inputvol, pd_inputcurrent, dcp_inputcurrent);
		sy6974b_set_input_curr_lim(charger, pd_inputcurrent);
	} else {
		CHDBG("charger set usb input v type: %d\n", sy6974b_get_usb_type());
		if (sy6974b_get_usb_type() == POWER_SUPPLY_TYPE_USB_DCP) {
			sy6974b_set_input_curr_lim(charger, dcp_inputcurrent);
		} else if (sy6974b_get_usb_type() == POWER_SUPPLY_TYPE_USB_CDP) {
			sy6974b_set_input_curr_lim(charger, dcp_inputcurrent);
		} else if (sy6974b_get_usb_type() == POWER_SUPPLY_TYPE_USB_FLOATING) {
			sy6974b_set_input_curr_lim(charger, dcp_inputcurrent);
		} else {
			sy6974b_set_input_curr_lim(charger, dcp_inputcurrent);
		}
	}
}

static int sy6974b_charger_input_volt_init(struct sy6974b *charger)
{
	int input_volt = VINDPM_DEF;  // 4.7V
	u8 reg_val;
	int ret;

	reg_val = (input_volt - VINDPM_V_MIN) / VINDPM_STEP;
	ret = sy6974b_update_bits(charger, REG_6, VINDPM_V_MASK, reg_val);
	if (ret) {
		printf("CH-SY6974: set charge input volt error!\n");
	}

	return ret;
}

static bool sy6974b_charger_status(struct udevice *dev)
{
	struct sy6974b *charger = dev_get_priv(dev);
	int state_of_charger;
	u8 value;
	int i = 0;

__retry:
	sy6974b_read(charger, REG_8, &value);
	state_of_charger = !!(value & PG_STAT_MASK);
	if (!state_of_charger && charger->pd_online) {
		if (i < 3) {
			i++;
			mdelay(20);
			goto __retry;
		}
	}

	if ((state_of_charger) && (charger->init_count < 5)) {
		sy6974b_charger_input_current_init(charger);
		sy6974b_charger_input_volt_init(charger);
		sy6974b_update_bits(charger, REG_1, CHRG_EN, CHRG_EN);
		charger->init_count++;
	}

	if (!state_of_charger) {
		sy6974b_set_prechrg_curr(charger, PRECHRG_I_LIM_DEF);
	}

	return state_of_charger;
}

static int sy6974b_ofdata_to_platdata(struct udevice *dev)
{
	struct sy6974b *charger = dev_get_priv(dev);
	u32 interrupt, phandle;
	int ret;

	charger->dev = dev;
	charger->ichg = dev_read_u32_default(dev, "vbat-current-limit-microamp", 0);
	if (charger->ichg == 0) {
		charger->ichg = ICHRG_I_DEF;
	}
	charger->vchg = dev_read_u32_default(dev, "vbat-voltage-limit-microvolt", 0);
	if (charger->vchg == 0) {
		charger->vchg = VREG_V_DEF;
	}

	phandle = dev_read_u32_default(dev, "interrupt-parent", -ENODATA);
	if (phandle == -ENODATA) {
		printf("CH-SY6974: read 'interrupt-parent' failed, ret=%d\n", phandle);
		return phandle;
	}

	ret = dev_read_u32_array(dev, "interrupts", &interrupt, 1);
	if (ret) {
		printf("CH-SY6974: read 'interrupts' failed, ret=%d\n", ret);
		return ret;
	}

	charger->irq = phandle_gpio_to_irq(phandle, interrupt);
	if (charger->irq < 0) {
		printf("CH-SY6974: failed to request irq: %d\n", charger->irq);
	}

	return 0;
}

static int sy6974b_probe(struct udevice *dev)
{
	struct sy6974b *charger = dev_get_priv(dev);

	charger->dev = dev;
	/* disable watchdog */
	sy6974b_update_bits(charger, REG_5, WDT_TIMER_MASK, WDT_TIMER_DISABLE);
	/* disable IINDPM VINDPM int */
	sy6974b_update_bits(charger, REG_A, VINDPM_INT_MASK | IINDPM_INT_MASK, VINDPM_INT_DIS | IINDPM_INT_DIS);
	/* set battery charge max volt and current */
	sy6974b_set_ichrg_curr(charger, charger->ichg);
	sy6974b_set_chrg_volt(charger, charger->vchg);
	printf("CH-SY6974: set charge volt=%d current=%d\n", charger->vchg, charger->ichg);

	return 0;
}

static const struct udevice_id charger_ids[] = {
	{.compatible = "sil,sy6974b"},
	{},
};

static struct dm_fuel_gauge_ops charger_ops = {
	.get_chrg_online = sy6974b_charger_status,
	.capability = sy6974b_charger_capability,
	.set_charger_voltage = sy6974b_set_charger_voltage,
	.set_charger_enable = sy6974b_charger_enable,
	.set_charger_disable = sy6974b_charger_disable,
	.set_charger_current = sy6974b_charger_current,
	.set_iprechg_current = sy6974b_iprechg_current,
};

U_BOOT_DRIVER(sy6974b_charger) = {
	.name = "sy6974b_charger",
	.id = UCLASS_FG,
	.probe = sy6974b_probe,
	.of_match = charger_ids,
	.ops = &charger_ops,
	.of_to_plat = sy6974b_ofdata_to_platdata,
	.priv_auto = sizeof(struct sy6974b),
};
