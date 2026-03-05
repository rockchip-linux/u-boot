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

#define SGM_DBG(args...) \
	do { \
		if (dbg_enable) { \
			printf(args); \
		} \
	} while (0)

/* define register addresses */
#define SGM4154x_CHRG_CTRL_0			0x00
#define SGM4154x_CHRG_CTRL_1			0x01
#define SGM4154x_CHRG_CTRL_2			0x02
#define SGM4154x_CHRG_CTRL_3			0x03
#define SGM4154x_CHRG_CTRL_4			0x04
#define SGM4154x_CHRG_CTRL_5			0x05
#define SGM4154x_CHRG_CTRL_6			0x06
#define SGM4154x_CHRG_CTRL_7			0x07
#define SGM4154x_CHRG_STAT			0x08
#define SGM4154x_CHRG_FAULT			0x09
#define SGM4154x_CHRG_CTRL_a			0x0a
#define SGM4154x_CHRG_CTRL_b			0x0b
#define SGM4154x_CHRG_CTRL_c			0x0c
#define SGM4154x_CHRG_CTRL_d			0x0d
#define SGM4154x_INPUT_DET			0x0e
#define SGM4154x_CHRG_CTRL_f			0x0f

/* charge status flags */
#define SGM4154x_CHRG_EN			BIT(4)
#define SGM4154x_HIZ_EN				BIT(7)
#define SGM4154x_TERM_EN			BIT(7)
#define SGM4154x_VAC_OVP_MASK			GENMASK(7, 6)
#define SGM4154x_DPDM_ONGOING			BIT(7)
#define SGM4154x_VBUS_GOOD			BIT(7)

#define SGM4154x_BOOSTV				GENMASK(5, 4)
#define SGM4154x_BOOST_LIM			BIT(7)
#define SGM4154x_OTG_EN				BIT(5)

/* Part ID */
#define SGM4154x_PN_MASK			GENMASK(6, 3)

/* WDT TIMER SET */
#define SGM4154x_WDT_TIMER_MASK			GENMASK(5, 4)
#define SGM4154x_WDT_TIMER_DISABLE		0
#define SGM4154x_WDT_TIMER_40S			BIT(4)
#define SGM4154x_WDT_TIMER_80S			BIT(5)
#define SGM4154x_WDT_TIMER_160S			(BIT(4) | BIT(5))

#define SGM4154x_WDT_RST_MASK			BIT(6)

/* SAFETY TIMER SET */
#define SGM4154x_SAFETY_TIMER_MASK		GENMASK(3, 3)
#define SGM4154x_SAFETY_TIMER_DISABLE		0
#define SGM4154x_SAFETY_TIMER_EN		BIT(3)
#define SGM4154x_SAFETY_TIMER_5H		0
#define SGM4154x_SAFETY_TIMER_10H		BIT(2)

/* recharge voltage */
#define SGM4154x_VRECHARGE			BIT(0)
#define SGM4154x_VRECHRG_STEP_mV		100
#define SGM4154x_VRECHRG_OFFSET_mV		100

/* charge status */
#define SGM4154x_VSYS_STAT			BIT(0)
#define SGM4154x_THERM_STAT			BIT(1)
#define SGM4154x_PG_STAT			BIT(2)
#define SGM4154x_CHG_STAT_MASK			GENMASK(4, 3)
#define SGM4154x_PRECHRG			BIT(3)
#define SGM4154x_FAST_CHRG			BIT(4)
#define SGM4154x_TERM_CHRG			(BIT(3) | BIT(4))

/* charge type */
#define SGM4154x_VBUS_STAT_MASK			GENMASK(7, 5)
#define SGM4154x_NOT_CHRGING			0
#define SGM4154x_USB_SDP			BIT(5)
#define SGM4154x_USB_CDP			BIT(6)
#define SGM4154x_USB_DCP			(BIT(5) | BIT(6))
#define SGM4154x_UNKNOWN			(BIT(7) | BIT(5))
#define SGM4154x_NON_STANDARD			(BIT(7) | BIT(6))
#define SGM4154x_OTG_MODE			(BIT(7) | BIT(6) | BIT(5))

/* TEMP Status */
#define SGM4154x_TEMP_MASK			GENMASK(2, 0)
#define SGM4154x_TEMP_NORMAL			BIT(0)
#define SGM4154x_TEMP_WARM			BIT(1)
#define SGM4154x_TEMP_COOL			(BIT(0) | BIT(1))
#define SGM4154x_TEMP_COLD			(BIT(0) | BIT(3))
#define SGM4154x_TEMP_HOT			(BIT(2) | BIT(3))

/* precharge current */
#define SGM4154x_PRECHRG_CUR_MASK		GENMASK(7, 4)
#define SGM4154x_PRECHRG_CURRENT_STEP_uA	60000
#define SGM4154x_PRECHRG_I_MIN_uA		60000
#define SGM4154x_PRECHRG_I_MAX_uA		780000
#define SGM4154x_PRECHRG_I_DEF_uA		180000

/* termination current */
#define SGM4154x_TERMCHRG_CUR_MASK		GENMASK(3, 0)
#define SGM4154x_TERMCHRG_CURRENT_STEP_uA	60000
#define SGM4154x_TERMCHRG_I_MIN_uA		60000
#define SGM4154x_TERMCHRG_I_MAX_uA		960000
#define SGM4154x_TERMCHRG_I_DEF_uA		180000

/* charge current */
#define SGM4154x_ICHRG_CUR_MASK			GENMASK(5, 0)
#define SGM4154x_ICHRG_I_STEP_uA		60000
#define SGM4154x_ICHRG_I_MIN_uA			0
#define SGM4154x_ICHRG_I_MAX_uA			3780000
#define SGM4154x_ICHRG_I_DEF_uA			2040000

/* charge voltage */
#define SGM4154x_VREG_V_MASK			GENMASK(7, 3)
#define SGM4154x_VREG_V_MAX_uV			4624000
#define SGM4154x_VREG_V_MIN_uV			3856000
#define SGM4154x_VREG_V_DEF_uV			4208000
#define SGM4154x_VREG_V_STEP_uV			32000

/* VREG Fine Tuning */
#define SGM4154x_VREG_FT_MASK			GENMASK(7, 6)
#define SGM4154x_VREG_FT_UP_8mV			BIT(6)
#define SGM4154x_VREG_FT_DN_8mV			BIT(7)
#define SGM4154x_VREG_FT_DN_16mV		(BIT(7) | BIT(6))

/* iindpm current */
#define SGM4154x_IINDPM_I_MASK			GENMASK(4, 0)
#define SGM4154x_IINDPM_I_MIN_uA		100000
#define SGM4154x_IINDPM_I_MAX_uA		2000000
#define SGM4154x_IINDPM_STEP_uA			100000
#define SGM4154x_IINDPM_DEF_uA			2400000

#define SGM4154x_VINDPM_INT_MASK		BIT(1)
#define SGM4154x_VINDPM_INT_DIS			BIT(1)
#define SGM4154x_IINDPM_INT_MASK		BIT(0)
#define SGM4154x_IINDPM_INT_DIS			BIT(0)

/* vindpm voltage */
#define SGM4154x_VINDPM_V_MASK			GENMASK(3, 0)
#define SGM4154x_VINDPM_V_MIN_uV		3900000
#define SGM4154x_VINDPM_V_MAX_uV		12000000
#define SGM4154x_VINDPM_STEP_uV			100000
#define SGM4154x_VINDPM_DEF_uV			4500000
#define SGM4154x_VINDPM_OS_MASK			GENMASK(1, 0)

/* DP DM SEL */
#define SGM4154x_DP_VSEL_MASK			GENMASK(4, 3)
#define SGM4154x_DM_VSEL_MASK			GENMASK(2, 1)

/* PUMPX SET */
#define SGM4154x_EN_PUMPX			BIT(7)
#define SGM4154x_PUMPX_UP			BIT(6)
#define SGM4154x_PUMPX_DN			BIT(5)

/* vendor ID related definitions */
#define SGM415xx_VENDOR_ID_MASK		GENMASK(6, 3)
#define SGM415xx_VENDOR_ID_SHIFT	3

/* chip-specific constants */
#define SGM41513_ICHRG_I_MAX_uA		3000000
#define SGM41513_IINDPM_I_MAX_uA	3200000
#define SGM41543_ICHRG_I_MAX_uA		3500000	/* Assumed value, need to verify with datasheet */
#define SGM41543_IINDPM_I_MAX_uA	3400000	/* Assumed value, need to verify with datasheet */

/* Chip IDs */
enum sgm415xx_vendor_id {
	SGM41513_CHIP_VENDOR_ID = 0x0,
	SGM41513X_CHIP_VENDOR_ID = 0x1,
	SGM41542S_CHIP_VENDOR_ID = 0xA,
	SGM41542_CHIP_VENDOR_ID = 0xD,
	SGM41512SX_CHIP_VENDOR_ID = 0xE,	/* Assumed value, need to verify with datasheet */
	SGM41512S_CHIP_VENDOR_ID = 0xF,		/* Assumed value, need to verify with datasheet */
};

/* VINDPM offset enumeration */
enum SGM4154x_VINDPM_OS {
	VINDPM_OS_3900mV,
	VINDPM_OS_5900mV,
	VINDPM_OS_7500mV,
	VINDPM_OS_10500mV,
};

/* Enhanced private data structure */
struct sgm41542 {
	struct udevice *dev;
	struct udevice *pd;
	bool pd_online;
	u32 init_count;
	u32 ichg;
	u32 vchg;
	int irq;

	/* Enhanced fields */
	int device_id;           /* Chip identification ID */
	int max_ichg;            /* Maximum charging current (chip-specific) */
	int max_ilim;            /* Maximum input current limit (chip-specific) */
	const char *chip_name;   /* Chip name string */
};

/* Power supply type enumeration */
enum power_supply_type {
	POWER_SUPPLY_TYPE_UNKNOWN = 0,
	POWER_SUPPLY_TYPE_USB,          /* Standard Downstream Port */
	POWER_SUPPLY_TYPE_USB_DCP,      /* Dedicated Charging Port */
	POWER_SUPPLY_TYPE_USB_CDP,      /* Charging Downstream Port */
	POWER_SUPPLY_TYPE_USB_FLOATING, /* DCP without shorting D+/D- */
};

/* Precharge current table for SGM41513 */
static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

/* Basic I2C read function */
static int sgm41542_read(struct sgm41542 *charger, uint reg, u8 *buffer)
{
	u8 val;
	int ret;

	ret = dm_i2c_read(charger->dev, reg, &val, 1);
	if (ret) {
		printf("sgm41542: read 0x%02x error, ret=%d\n", reg, ret);
		return ret;
	}

	*buffer = val;
	return 0;
}

/* Basic I2C write function */
static int sgm41542_write(struct sgm41542 *charger, uint reg, u8 val)
{
	int ret;

	ret = dm_i2c_write(charger->dev, reg, &val, 1);
	if (ret)
		printf("sgm41542: write 0x%02x error, ret=%d\n", reg, ret);

	return ret;
}

/* Update bits function */
static int sgm41542_update_bits(struct sgm41542 *charger,
				u8 offset,
				u8 mask,
				u8 val)
{
	u8 reg;
	int ret;

	ret = sgm41542_read(charger, offset, &reg);
	if (ret)
		return ret;

	reg &= ~mask;
	reg |= val;

	return sgm41542_write(charger, offset, reg);
}

/* Chip ID detection function */
static int sgm4154x_hw_chipid_detect(struct sgm41542 *charger)
{
	u8 val = 0;
	int ret;

	ret = sgm41542_read(charger, SGM4154x_CHRG_CTRL_b, &val);
	if (ret) {
		printf("sgm41542: failed to read chip ID register\n");
		return ret;
	}

	val &= SGM415xx_VENDOR_ID_MASK;
	val >>= SGM415xx_VENDOR_ID_SHIFT;
	charger->device_id = val;

	/* Set chip-specific current limits */
	switch (charger->device_id) {
	case SGM41513_CHIP_VENDOR_ID:
	case SGM41513X_CHIP_VENDOR_ID:
		charger->max_ichg = SGM41513_ICHRG_I_MAX_uA;
		charger->max_ilim = SGM41513_IINDPM_I_MAX_uA;
		charger->chip_name = "SGM41513/SGM41513X";
		break;
	case SGM41542_CHIP_VENDOR_ID:
	case SGM41542S_CHIP_VENDOR_ID:
		charger->max_ichg = SGM4154x_ICHRG_I_MAX_uA;
		charger->max_ilim = SGM4154x_IINDPM_I_MAX_uA;
		charger->chip_name = "SGM41542";
		break;
	case SGM41512SX_CHIP_VENDOR_ID:
		charger->max_ichg = SGM41543_ICHRG_I_MAX_uA;
		charger->max_ilim = SGM41543_IINDPM_I_MAX_uA;
		charger->chip_name = "SGM41512SX";
		break;
	case SGM41512S_CHIP_VENDOR_ID:
		/* Assuming SGM41512S has same parameters as SGM41542 */
		charger->max_ichg = SGM41513_ICHRG_I_MAX_uA;
		charger->max_ilim = SGM41513_IINDPM_I_MAX_uA;
		charger->chip_name = "SGM41512S";
		break;
	default:
		printf("sgm41542: unknown chip ID: 0x%x\n", charger->device_id);
		charger->max_ichg = SGM41513_ICHRG_I_MAX_uA;
		charger->max_ilim = SGM41513_IINDPM_I_MAX_uA;
		charger->chip_name = "Unknown";
		break;
	}

	printf("sgm41542: detected %s (ID: 0x%x), max ichg: %dmA, max ilim: %dmA\n",
	       charger->chip_name, charger->device_id,
	       charger->max_ichg / 1000, charger->max_ilim / 1000);
	return 0;
}

/* Input current limit setting function */
static int sgm4154x_set_input_curr_lim(struct sgm41542 *charger, int iindpm)
{
	u8 reg_val;
	int ret;

	if (iindpm < SGM4154x_IINDPM_I_MIN_uA)
		iindpm = SGM4154x_IINDPM_I_MIN_uA;
	if (iindpm > charger->max_ilim)
		iindpm = charger->max_ilim;

	if (iindpm >= SGM4154x_IINDPM_I_MIN_uA &&
	    iindpm <= charger->max_ilim)
		reg_val = (iindpm - SGM4154x_IINDPM_I_MIN_uA) / SGM4154x_IINDPM_STEP_uA;

	ret = sgm41542_update_bits(charger,
				   SGM4154x_CHRG_CTRL_0,
				   SGM4154x_IINDPM_I_MASK,
				   reg_val);

	return ret;
}

/* USB type detection function */
static int sgm41542_get_usb_type(void)
{
#ifdef CONFIG_PHY_ROCKCHIP_INNO_USB2
	return rockchip_chg_get_type();
#else
	return 0;
#endif
}

/* Charger capability function */
static int sgm41542_charger_capability(struct udevice *dev)
{
	return FG_CAP_CHARGER;
}

/* Charging current setting function (with chip-specific calculation) */
static int sgm4154x_set_ichrg_curr(struct sgm41542 *charger, int uA)
{
	u8 reg_val = 0;
	int ret;

	/* Boundary checking */
	if (uA < SGM4154x_ICHRG_I_MIN_uA)
		uA = SGM4154x_ICHRG_I_MIN_uA;
	if (uA > charger->max_ichg)
		uA = charger->max_ichg;

	/* Chip-specific calculation */
	if (charger->device_id == SGM41512SX_CHIP_VENDOR_ID ||
	    charger->device_id == SGM41512S_CHIP_VENDOR_ID ||
	    charger->device_id == SGM41513_CHIP_VENDOR_ID ||
	    charger->device_id == SGM41513X_CHIP_VENDOR_ID) {
		if (uA <= 40000)
			reg_val = uA / 5000;
		else if (uA <= 110000)
			reg_val = 0x08 + (uA - 40000) / 10000;
		else if (uA <= 270000)
			reg_val = 0x0F + (uA - 110000) / 20000;
		else if (uA <= 540000)
			reg_val = 0x17 + (uA - 270000) / 30000;
		else if (uA <= 1500000)
			reg_val = 0x20 + (uA - 540000) / 60000;
		else if (uA <= 2940000)
			reg_val = 0x30 + (uA - 1500000) / 120000;
		else
			reg_val = 0x3d;
	} else {
		/* SGM41542/43 use linear calculation */
		reg_val = uA / SGM4154x_ICHRG_I_STEP_uA;
	}

	ret = sgm41542_update_bits(charger,
				   SGM4154x_CHRG_CTRL_2,
				   SGM4154x_ICHRG_CUR_MASK,
				   reg_val);
	if (ret)
		printf("sgm41542: set charge current error!\n");

	return ret;
}

/* Precharge current setting function (with chip-specific calculation) */
static int sgm4154x_set_prechrg_curr(struct sgm41542 *charger, int uA)
{
	u8 reg_val = 0;
	int i, ret;

	/* Chip-specific calculation */
	if (charger->device_id == SGM41512SX_CHIP_VENDOR_ID ||
	    charger->device_id == SGM41512S_CHIP_VENDOR_ID ||
	    charger->device_id == SGM41513_CHIP_VENDOR_ID ||
	    charger->device_id == SGM41513X_CHIP_VENDOR_ID) {
		for (i = 1; i < 16; i++) {
			if (uA >= IPRECHG_CURRENT_STABLE[i])
				break;
		}
		reg_val = i - 1;
	} else {
		/* SGM41542/43/12S use linear calculation */
		if (uA < SGM4154x_PRECHRG_I_MIN_uA)
			uA = SGM4154x_PRECHRG_I_MIN_uA;
		else if (uA > SGM4154x_PRECHRG_I_MAX_uA)
			uA = SGM4154x_PRECHRG_I_MAX_uA;

		reg_val = (uA - SGM4154x_PRECHRG_I_MIN_uA) / SGM4154x_PRECHRG_CURRENT_STEP_uA;
	}

	reg_val = reg_val << 4;
	ret = sgm41542_update_bits(charger,
				   SGM4154x_CHRG_CTRL_3,
				   SGM4154x_PRECHRG_CUR_MASK,
				   reg_val);
	if (ret)
		printf("sgm41542: set precharge current error!\n");

	return ret;
}

/* Charging voltage setting function */
static int sgm4154x_set_chrg_volt(struct sgm41542 *charger, int chrg_volt)
{
	u8 reg_val;
	int ret;

	if (chrg_volt < SGM4154x_VREG_V_MIN_uV)
		chrg_volt = SGM4154x_VREG_V_MIN_uV;
	else if (chrg_volt > SGM4154x_VREG_V_MAX_uV)
		chrg_volt = SGM4154x_VREG_V_MAX_uV;

	reg_val = (chrg_volt - SGM4154x_VREG_V_MIN_uV) / SGM4154x_VREG_V_STEP_uV;
	reg_val = reg_val << 3;
	ret = sgm41542_update_bits(charger,
				   SGM4154x_CHRG_CTRL_4,
				   SGM4154x_VREG_V_MASK,
				   reg_val);

	return ret;
}

/* Driver API: Set charger voltage */
static int sgm41542_set_charger_voltage(struct udevice *dev, int uV)
{
	struct sgm41542 *charger = dev_get_priv(dev);

	SGM_DBG("SGM41542: set charger voltage %d uV\n", uV);
	return sgm4154x_set_chrg_volt(charger, uV);
}

/* Driver API: Enable charger */
static int sgm41542_charger_enable(struct udevice *dev)
{
	struct sgm41542 *charger = dev_get_priv(dev);

	SGM_DBG("SGM41542: enable charger\n");
	sgm41542_update_bits(charger, SGM4154x_CHRG_CTRL_1,
			     SGM4154x_CHRG_EN,
			     SGM4154x_CHRG_EN);
	return 0;
}

/* Driver API: Disable charger */
static int sgm41542_charger_disable(struct udevice *dev)
{
	struct sgm41542 *charger = dev_get_priv(dev);

	SGM_DBG("SGM41542: disable charger\n");
	sgm41542_update_bits(charger, SGM4154x_CHRG_CTRL_1,
			     SGM4154x_CHRG_EN,
			     0);
	return 0;
}

/* Driver API: Set precharge current */
static int sgm41542_iprechg_current(struct udevice *dev, int iprechrg_uA)
{
	struct sgm41542 *charger = dev_get_priv(dev);

	SGM_DBG("SGM41542: set precharge current: %d uA\n", iprechrg_uA);
	return sgm4154x_set_prechrg_curr(charger, iprechrg_uA);
}

/* Driver API: Set charger current */
static int sgm41542_charger_current(struct udevice *dev, int ichrg_uA)
{
	struct sgm41542 *charger = dev_get_priv(dev);

	SGM_DBG("SGM41542: set charge current: %d uA\n", ichrg_uA);
	return sgm4154x_set_ichrg_curr(charger, ichrg_uA);
}

/* PD output value getter */
static int sgm41542_get_pd_output_val(struct sgm41542 *charger,
				      int *vol,
				      int *cur)
{
#ifdef CONFIG_DM_POWER_DELIVERY
	struct power_delivery_data pd_data;
	int ret;

	if (!charger->pd)
		return -EINVAL;

	memset(&pd_data, 0, sizeof(pd_data));
	ret = power_delivery_get_data(charger->pd, &pd_data);
	if (ret)
		return ret;
	if (!pd_data.online || !pd_data.voltage || !pd_data.current)
		return -EINVAL;

	*vol = pd_data.voltage;
	*cur = pd_data.current;
	charger->pd_online = pd_data.online;

	return 0;
#else
	return -ENOSYS;
#endif
}

/* Charger input current initialization */
static void sgm41542_charger_input_current_init(struct sgm41542 *charger)
{
	int sdp_inputcurrent = 500 * 1000;
	int dcp_inputcurrent = 2000 * 1000;
	int pd_inputvol, pd_inputcurrent;
	int ret;

	if (!charger->pd) {
		ret = uclass_get_device(UCLASS_PD, 0, &charger->pd);
		if (ret) {
			if (ret == -ENODEV)
				SGM_DBG("sgm41542: Can't find PD\n");
			else
				printf("sgm41542: Get UCLASS PD failed: %d\n", ret);
			charger->pd = NULL;
		}
	}

	if (!sgm41542_get_pd_output_val(charger, &pd_inputvol, &pd_inputcurrent)) {
		SGM_DBG("PD adapter detected: %d mV, %d mA\n",
			pd_inputvol / 1000, pd_inputcurrent / 1000);

		sgm4154x_set_input_curr_lim(charger, pd_inputcurrent);
	} else {
		SGM_DBG("Normal adapter type: %d\n", sgm41542_get_usb_type());

		if (sgm41542_get_usb_type() == POWER_SUPPLY_TYPE_USB_DCP ||
		    sgm41542_get_usb_type() == POWER_SUPPLY_TYPE_USB_CDP ||
		    sgm41542_get_usb_type() == POWER_SUPPLY_TYPE_USB_FLOATING)
			sgm4154x_set_input_curr_lim(charger, dcp_inputcurrent);
		else
			sgm4154x_set_input_curr_lim(charger, sdp_inputcurrent);
	}
}

/* Charger status function */
static bool sgm41542_charger_status(struct udevice *dev)
{
	struct sgm41542 *charger = dev_get_priv(dev);
	int state_of_charger;
	u8 value;
	int i = 0;

__retry:
	sgm41542_read(charger, SGM4154x_CHRG_STAT, &value);
	state_of_charger = !!(value & SGM4154x_PG_STAT);
	if (!state_of_charger && charger->pd_online) {
		if (i < 3) {
			i++;
			mdelay(20);
			goto __retry;
		}
	}

	if (state_of_charger && charger->init_count < 5) {
		sgm41542_charger_input_current_init(charger);
		sgm41542_update_bits(charger,
				     SGM4154x_CHRG_CTRL_1,
				     SGM4154x_CHRG_EN,
				     SGM4154x_CHRG_EN);
		charger->init_count++;
	}

	if (!state_of_charger)
		sgm4154x_set_prechrg_curr(charger, SGM4154x_PRECHRG_I_DEF_uA);

	return state_of_charger;
}

static void sgm41542_irq_handler(int irq, void *data)
{
}

/* Device tree parsing function */
static int sgm41542_of_to_plat(struct udevice *dev)
{
	struct sgm41542 *charger = dev_get_priv(dev);
	u32 interrupt, phandle;
	int ret;

	charger->dev = dev;

	/* Parse existing properties */
	charger->ichg = dev_read_u32_default(dev,
					     "vbat-current-limit-microamp",
					     0);
	if (charger->ichg == 0)
		charger->ichg = 3000 * 1000;

	charger->vchg = dev_read_u32_default(dev,
					     "vbat-voltage-limit-microamp",
					     0);
	if (charger->vchg == 0)
		charger->vchg = 4400 * 1000;

	SGM_DBG("charger->ichg: %d uA\n", charger->ichg);
	SGM_DBG("charger->vchg: %d uV\n", charger->vchg);

	/* Parse interrupt properties */
	phandle = dev_read_u32_default(dev, "interrupt-parent", -ENODATA);
	if (phandle == -ENODATA) {
		/* Interrupt is optional */
		charger->irq = 0;
	} else {
		ret = dev_read_u32_array(dev, "interrupts", &interrupt, 1);
		if (ret) {
			printf("sgm41542: read 'interrupts' failed, ret=%d\n", ret);
			return ret;
		}

		charger->irq = phandle_gpio_to_irq(phandle, interrupt);
		if (charger->irq < 0)
			SGM_DBG("sgm41542: failed to request irq: %d\n", charger->irq);
	}

	return 0;
}

/* Probe function */
static int sgm41542_probe(struct udevice *dev)
{
	struct sgm41542 *charger = dev_get_priv(dev);
	int ret;

	printf("sgm41542: driver initializing\n");
	charger->dev = dev;

	/* Disable watchdog */
	ret = sgm41542_update_bits(charger, SGM4154x_CHRG_CTRL_5,
				   SGM4154x_WDT_TIMER_MASK,
				   SGM4154x_WDT_TIMER_DISABLE);
	if (ret) {
		printf("sgm41542: disable watchdog failed\n");
		return ret;
	}

	/* Disable VINDPM and IINDPM interrupts */
	ret = sgm41542_update_bits(charger, SGM4154x_CHRG_CTRL_a,
				   SGM4154x_VINDPM_INT_MASK | SGM4154x_IINDPM_INT_MASK,
				   SGM4154x_VINDPM_INT_DIS | SGM4154x_IINDPM_INT_DIS);
	if (ret) {
		printf("sgm41542: disable interrupts failed\n");
		return ret;
	}

	/* Chip detection */
	ret = sgm4154x_hw_chipid_detect(charger);
	if (ret) {
		printf("sgm41542: chip detection failed\n");
		return ret;
	}

	/* Set charging parameters */
	ret = sgm4154x_set_ichrg_curr(charger, charger->ichg);
	if (ret) {
		printf("sgm41542: set charge current failed\n");
		return ret;
	}

	ret = sgm4154x_set_chrg_volt(charger, charger->vchg);
	if (ret) {
		printf("sgm41542: set charge voltage failed\n");
		return ret;
	}

	if (0 && charger->irq) {
		SGM_DBG("sgm41542: enable sgm42542 irq\n");
		irq_install_handler(charger->irq, sgm41542_irq_handler, dev);
		irq_handler_enable(charger->irq);
	}

	printf("sgm41542: %s initialized successfully\n", charger->chip_name);
	return 0;
}

/* Compatible device IDs */
static const struct udevice_id charger_ids[] = {
	{ .compatible = "sgm,sgm41512s" },
	{ .compatible = "sgm,sgm41512sx" },
	{ .compatible = "sgm,sgm41513" },
	{ .compatible = "sgm,sgm41513x" },
	{ .compatible = "sgm,sgm41542" },
	{ .compatible = "sgm,sgm41542s" },
	{ },
};

/* Fuel gauge operations */
static struct dm_fuel_gauge_ops charger_ops = {
	.get_chrg_online = sgm41542_charger_status,
	.capability = sgm41542_charger_capability,
	.set_charger_voltage = sgm41542_set_charger_voltage,
	.set_charger_enable = sgm41542_charger_enable,
	.set_charger_disable = sgm41542_charger_disable,
	.set_charger_current = sgm41542_charger_current,
	.set_iprechg_current = sgm41542_iprechg_current,

};

/* U-Boot driver declaration */
U_BOOT_DRIVER(sgm41542_charger) = {
	.name = "sgm41542_charger",
	.id = UCLASS_FG,
	.probe = sgm41542_probe,
	.of_match = charger_ids,
	.ops = &charger_ops,
	.of_to_plat = sgm41542_of_to_plat,
	.priv_auto = sizeof(struct sgm41542),
};
