// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Rockchip Electronics Co., Ltd.
 *
 * Rockchip PMIC Type-C Port Controller Driver
 */

#include <asm/gpio.h>
#include <dm.h>
#include <dm/of_access.h>
#include <i2c.h>
#include <irq-generic.h>
#include <linux/bitfield.h>
#include <linux/compat.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <power/power_delivery/tcpm.h>
#include <power/power_delivery/pd.h>
#include <power/power_delivery/power_delivery.h>

#define RK_TCPC_DEVICE_ID			0x00
#define RK_TCPC_DEVICE_VER_ID			GENMASK(7, 4)
#define RK_TCPC_DEVICE_REV_ID			GENMASK(3, 0)

#define RK_TCPC_INT_STS				0x01
#define RK_TCPC_INT_STS_VSAFE0V			BIT(7)
#define RK_TCPC_INT_STS_CC_OV			BIT(6)
#define RK_TCPC_INT_STS_TX_SUCCESS		BIT(5)
#define RK_TCPC_INT_STS_TX_FAILED		BIT(4)
#define RK_TCPC_INT_STS_RX_HARD_RST		BIT(3)
#define RK_TCPC_INT_STS_RX_SUCCESS		BIT(2)
#define RK_TCPC_INT_STS_VBUS			BIT(1)
#define RK_TCPC_INT_STS_CC			BIT(0)
#define RK_TCPC_INT_STS_MASK			GENMASK(7, 0)

#define RK_TCPC_INT				0x02
#define RK_TCPC_INT_VSAFE0V			BIT(7)
#define RK_TCPC_INT_CC_OV			BIT(6)
#define RK_TCPC_INT_TX_SUCCESS			BIT(5)
#define RK_TCPC_INT_TX_FAILED			BIT(4)
#define RK_TCPC_INT_RX_HARD_RST			BIT(3)
#define RK_TCPC_INT_RX_SUCCESS			BIT(2)
#define RK_TCPC_INT_VBUS			BIT(1)
#define RK_TCPC_INT_CC				BIT(0)
#define RK_TCPC_INT_MASK			GENMASK(7, 0)

#define RK_TCPC_CTRL				0x03
#define RK_TCPC_CTRL_CC_POLARITY_CC2		BIT(0)

#define RK_TCPC_CTRL1				0x04
#define RK_TCPC_CTRL1_PD_EN			BIT(0)

#define RK_TCPC_CTRL2				0x05
#define RK_TCPC_CTRL2_TOGGLE			BIT(3)
#define RK_TCPC_CTRL2_CC_RD_EN			BIT(2)
#define RK_TCPC_CTRL2_CC_RP_3_0			(0x3)
#define RK_TCPC_CTRL2_CC_RP_1_5			(0x2)
#define RK_TCPC_CTRL2_CC_RP_DEF			(0x1)
#define RK_TCPC_CTRL2_CC_RP_OFF			(0x0)
#define RK_TCPC_CTRL2_CC_RP_MASK		GENMASK(1, 0)

#define RK_TCPC_CTRL3				0x06
#define RK_TCPC_CTRL3_TDRP_90_MS		(3)
#define RK_TCPC_CTRL3_TDRP_80_MS		(2)
#define RK_TCPC_CTRL3_TDRP_70_MS		(1)
#define RK_TCPC_CTRL3_TDRP_60_MS		(0)
#define RK_TCPC_CTRL3_TDRP			GENMASK(7, 6)
#define RK_TCPC_CTRL3_DCSRCDRP_60		(3)
#define RK_TCPC_CTRL3_DCSRCDRP_50		(2)
#define RK_TCPC_CTRL3_DCSRCDRP_40		(1)
#define RK_TCPC_CTRL3_DCSRCDRP_30		(0)
#define RK_TCPC_CTRL3_DCSRCDRP			GENMASK(5, 4)
#define RK_TCPC_CTRL3_TYPEC_EN			BIT(3)
#define RK_TCPC_CTRL3_VSAFE0V_DET_EN		BIT(2)
#define RK_TCPC_CTRL3_LP_MODE_EN		(BIT(1) | BIT(0))

#define RK_TCPC_STS				0x07
#define RK_TCPC_STS_CC_OV			BIT(6)
#define RK_TCPC_STS_TOGSS_RUNNING		(0x2)
#define RK_TCPC_STS_TOGSS_RP			(0x0)
#define RK_TCPC_STS_TOGSS_RD			(0x1)
#define RK_TCPC_STS_TOGSS			GENMASK(5, 4)
#define RK_TCPC_STS_CC2				GENMASK(3, 2)
#define RK_TCPC_STS_CC1				GENMASK(1, 0)

#define RK_TCPC_STS1				0x08
#define RK_TCPC_STS_ATTACHED_DB_SRC		0x0d
#define RK_TCPC_STS_DETACH_DB_SNK		0x07
#define RK_TCPC_STS_ATTACHED_DB_SNK		0x06
#define RK_TCPC_STS_TYPEC_STATE			GENMASK(7, 3)
#define RK_TCPC_STS_VBUS			BIT(1)
#define RK_TCPC_STS_VSAFE0V			BIT(0)

#define RK_TCPC_RX_DET				0x09
#define RK_TCPC_RX_DET_CABLE_PLUG		BIT(4)
#define RK_TCPC_RX_DET_DATA_ROLE_DFP		BIT(3)
#define RK_TCPC_RX_DET_PD_SPEC_REV		GENMASK(2, 1)
#define RK_TCPC_RX_DET_PWR_ROLE_SRC		BIT(0)

#define RK_TCPC_RX_DET1				0x0a
#define RK_TCPC_RX_DET1_HARD_RST_EN		BIT(6)
#define RK_TCPC_RX_DET1_CABLE_RST_EN		BIT(5)
#define RK_TCPC_RX_DET1_SOP2_EN			BIT(2)
#define RK_TCPC_RX_DET1_SOP1_EN			BIT(1)
#define RK_TCPC_RX_DET1_SOP_EN			BIT(0)

#define RK_TCPC_RX_INFO				0x0b

#define RK_TCPC_RX_CTRL				0x0c
#define RK_TCPC_RX_CTRL_RX_EN			BIT(0)

#define RK_TCPC_TX_CFG				0x0d
#define RK_TCPC_TX_CFG_RETRY_CNT_3		(0x3)
#define RK_TCPC_TX_CFG_RETRY_CNT_2		(0x2)
#define RK_TCPC_TX_CFG_RETRY_CNT_1		(0x1)
#define RK_TCPC_TX_CFG_RETRY_CNT_0		(0x0)
#define RK_TCPC_TX_CFG_RETRY			GENMASK(4, 3)
#define RK_TCPC_TX_CFG_MSG_TYPE			GENMASK(2, 0)

#define RK_TCPC_TX_CFG1				0x0e
#define RK_TCPC_TX_CFG1_TX_LOAD			GENMASK(4, 3)
#define RK_TCPC_TX_CFG1_TX_LOAD_900_OHM		(0x0)
#define RK_TCPC_TX_CFG1_TX_LOAD_800_OHM		(0x1)
#define RK_TCPC_TX_CFG1_TX_LOAD_1000_OHM	(0x2)
#define RK_TCPC_TX_CFG1_TX_LOAD_700_OHM		(0x3)
#define RK_TCPC_TX_CFG1_BIST_TST_MODE		BIT(2)
#define RK_TCPC_TX_CFG1_TX_CARRIER_MODE		BIT(1)
#define RK_TCPC_TX_CFG1_TX_EN			BIT(0)

#define RK_TCPC_TX_CFG2				0x0f
#define RK_TCPC_TX_CFG2_BMC_EN			BIT(0)

#define RK_TCPC_TX_CTRL				0x10
#define RK_TCPC_TX_CTRL_TX_BYTE_CNT		GENMASK(5, 0)

#define RK_TCPC_BMC_STS				0x11

#define RK_TCPC_RESET				0x12
#define RK_TCPC_RESET_TX			BIT(2)
#define RK_TCPC_RESET_RX			BIT(1)
#define RK_TCPC_RESET_PD			BIT(0)

#define RK_TCPC_PD_HEADER			0x15
#define RK_TCPC_PD_DATA				0x17

#define RK_TCPC_DB_CTRL				0x70
#define RK_TCPC_DB_HW_DISABLE			BIT(0)

#define RK_TCPC_REG_OFFSET_MAX			RK_TCPC_DB_CTRL

#define RK_TCPC_DEBUG	0

struct rk_tcpc_chip {
	struct udevice *dev;
	struct tcpm_port *tcpm_port;
	struct tcpc_dev tcpc_dev;
	struct regulator *vbus;
	int irq;
	bool int_present;

	/* lock for sharing chip states */
	struct mutex lock;

	/* pd status */
	bool tx_fun_en;
	bool rx_fun_en;

	/* port status */
	bool vbus_on;
	bool charge_on;
	bool vbus_present;

	enum typec_cc_status cc1;
	enum typec_cc_status cc2;
};

/*
 * Logging
 */

 #define rk_tcpc_log(dev, fmt, ...)				\
 ({								\
	if (RK_TCPC_DEBUG)					\
		printf(fmt "\n", ##__VA_ARGS__);		\
 })

 /*
 * Copy from Linux Kernel: include/linux/string_choices.h
 */
static inline const char *str_enable_disable(bool v)
{
	return v ? "enable" : "disable";
}

static inline const char *str_on_off(bool v)
{
	return v ? "on" : "off";
}

static int rk_tcpc_write8(struct rk_tcpc_chip *chip, unsigned int reg, u8 val)
{
	int ret = 0;

	ret = dm_i2c_write(chip->dev, reg, &val, 1);
	if (ret < 0)
		printf("%s: cannot write 0x%02x to 0x%02x, ret=%d", __func__, val, reg, ret);

	return ret;
}

static int rk_tcpc_block_write(struct rk_tcpc_chip *chip, unsigned int reg,
			       u8 *data, u8 len)
{
	int ret = 0;

	if (!len)
		goto out;

	ret = dm_i2c_write(chip->dev, reg, data, len);
	if (ret < 0)
		printf("%s: cannot block write 0x%02x, len=%d, ret=%d", __func__, reg, len, ret);

out:
	return ret;
}

static int rk_tcpc_read8(struct rk_tcpc_chip *chip, unsigned int reg, u8 *val)
{
	int ret = 0, retries;

	for (retries = 0; retries < 3; retries++) {
		ret = dm_i2c_read(chip->dev, reg, val, 1);
		if (ret == 0)
			return ret;

		printf("%s: cannot read 0x%02x, ret=%d", __func__, reg, ret);
	}

	return ret;
}

static int rk_tcpc_block_read(struct rk_tcpc_chip *chip, u8 reg,
			      u8 *data, u8 len)
{
	int ret = 0;

	if (!len)
		goto out;

	ret = dm_i2c_read(chip->dev, reg, data, len);
	if (ret < 0)
		printf("%s: cannot block read 0x%02x, len=%d, ret=%d", __func__, reg, len, ret);

out:
	return ret;
}

static int rk_tcpc_check_id(struct rk_tcpc_chip *chip)
{
	int ret;
	u8 val;

	ret = rk_tcpc_read8(chip, RK_TCPC_DEVICE_ID, &val);
	if (ret < 0) {
		printf("cannot read Device id, ret=%d\n", ret);
		return ret;
	}

	printf("Version ID: 0x%02lx, Revision ID: 0x%02lx,\n",
		 FIELD_GET(RK_TCPC_DEVICE_VER_ID, val), FIELD_GET(RK_TCPC_DEVICE_REV_ID, val));

	return 0;
}

static int rk_tcpc_sw_reset(struct rk_tcpc_chip *chip)
{
	int ret;

	ret = rk_tcpc_write8(chip, RK_TCPC_RESET, RK_TCPC_RESET_PD);
	if (ret < 0)
		rk_tcpc_log(chip, "cannot sw reset the chip, ret=%d", ret);
	else
		rk_tcpc_log(chip, "sw reset");

	return ret;
}

static int rk_tcpc_enable_tx(struct rk_tcpc_chip *chip, bool enable)
{
	int ret = 0;

	/*
	 * TX module is usually disabled to save power consumption,
	 * enabled it at first PD transmit.
	 */
	if (enable) {
		if (!chip->tx_fun_en) {
			ret = rk_tcpc_write8(chip, RK_TCPC_TX_CFG1, RK_TCPC_TX_CFG1_TX_EN);
			if (ret < 0)
				return ret;

			chip->tx_fun_en = true;
			rk_tcpc_log(chip, "PD tx enabled");
		}
	} else {
		if (chip->tx_fun_en) {
			ret = rk_tcpc_write8(chip, RK_TCPC_TX_CFG1, 0);
			if (ret < 0)
				return ret;

			chip->tx_fun_en = false;
			rk_tcpc_log(chip, "PD tx disabled");
		}
	}

	return 0;
}

static int rk_tcpc_enable_rx(struct rk_tcpc_chip *chip, bool enable)
{
	int ret = 0;

	/*
	 * RX module is usually disabled to save power consumption,
	 * enabled it at first prepare for PD receiving.
	 */
	if (enable) {
		if (!chip->rx_fun_en) {
			ret = rk_tcpc_write8(chip, RK_TCPC_RX_CTRL, RK_TCPC_RX_CTRL_RX_EN);
			if (ret < 0)
				return ret;

			chip->rx_fun_en = true;
			rk_tcpc_log(chip, "PD rx enabled");
		}
	} else {
		if (chip->rx_fun_en) {
			ret = rk_tcpc_write8(chip, RK_TCPC_RX_CTRL, 0);
			if (ret < 0)
				return ret;

			chip->rx_fun_en = false;
			rk_tcpc_log(chip, "PD rx disabled");
		}
	}

	return 0;
}

static int rk_tcpc_set_lpmode(struct rk_tcpc_chip *chip)
{
	u8 reg = 0;
	int ret;

	/* Set Rp default as 80uA */
	ret = rk_tcpc_read8(chip, RK_TCPC_CTRL2, &reg);
	if (ret < 0)
		return ret;

	reg &= ~RK_TCPC_CTRL2_CC_RP_MASK;
	reg |= RK_TCPC_CTRL2_CC_RP_DEF;

	ret = rk_tcpc_write8(chip, RK_TCPC_CTRL2, reg);
	if (ret < 0)
		return ret;

	/* lp mode enable */
	ret = rk_tcpc_read8(chip, RK_TCPC_CTRL3, &reg);
	if (ret < 0)
		return ret;

	reg |= RK_TCPC_CTRL3_LP_MODE_EN;

	ret = rk_tcpc_write8(chip, RK_TCPC_CTRL3, reg);
	if (ret < 0)
		return ret;

	return 0;
}

static enum typec_cc_status rk_tcpc_sts_to_cc(u8 sts, bool sink)
{
	switch (sts) {
	case 0x1:
		return sink ? TYPEC_CC_RP_DEF : TYPEC_CC_RA;
	case 0x2:
		return sink ? TYPEC_CC_RP_1_5 : TYPEC_CC_RD;
	case 0x3:
		if (sink)
			return TYPEC_CC_RP_3_0;
		/* fallthrough; */
	case 0x0:
	default:
		return TYPEC_CC_OPEN;
	}
}

static int rk_tcpc_pd_read_message(struct rk_tcpc_chip *chip,
				   struct pd_message *msg)
{
	u16 header;
	int len;
	int ret;

	ret = rk_tcpc_block_read(chip, RK_TCPC_PD_HEADER, (u8 *)&header, 2);
	if (ret < 0)
		return ret;

	len = pd_header_cnt_le(header) * 4;
	if (len > PD_MAX_PAYLOAD * 4) {
		rk_tcpc_log(chip, "PD message too long %d", len);
		return -EINVAL;
	}

	msg->header = cpu_to_le16(header);

	if (len > 0) {
		ret = rk_tcpc_block_read(chip, RK_TCPC_PD_DATA,
					 (u8 *)&msg->payload, len);
		if (ret < 0)
			return ret;
	}

	rk_tcpc_log(chip, "PD header: 0x%04x, payload len: %d", msg->header, len);

	return 0;
}

static int rk_tcpc_init_interrupt(struct rk_tcpc_chip *chip)
{
	int ret = 0;
	u8 reg;

	/* Clear all events */
	ret = rk_tcpc_write8(chip, RK_TCPC_INT_STS, RK_TCPC_INT_STS_MASK);
	if (ret < 0)
		return ret;

	reg = RK_TCPC_INT_CC | RK_TCPC_INT_VBUS | RK_TCPC_INT_RX_SUCCESS |
	      RK_TCPC_INT_RX_HARD_RST | RK_TCPC_INT_TX_FAILED |
	      RK_TCPC_INT_TX_SUCCESS;

	ret = rk_tcpc_write8(chip, RK_TCPC_INT, reg);
	if (ret < 0)
		return ret;

	return 0;
}

static int tcpm_init(struct tcpc_dev *dev)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg;
	int ret;

	ret = rk_tcpc_sw_reset(chip);
	if (ret < 0)
		return ret;

	/* Clear all events */
	ret = rk_tcpc_write8(chip, RK_TCPC_INT_STS, RK_TCPC_INT_STS_MASK);
	if (ret < 0)
		return ret;

	/* Disable all interrupts before requesting irq */
	ret = rk_tcpc_write8(chip, RK_TCPC_INT, 0);
	if (ret < 0)
		return ret;

	/* Disable HW debounce */
	ret = rk_tcpc_write8(chip, RK_TCPC_DB_CTRL, RK_TCPC_DB_HW_DISABLE);
	if (ret < 0)
		return ret;

	/* tDRP : 80 ms */
	reg = FIELD_PREP(RK_TCPC_CTRL3_TDRP, RK_TCPC_CTRL3_TDRP_80_MS);

	/* dcSRC.DRP : 30%src + 70%snk */
	reg |= FIELD_PREP(RK_TCPC_CTRL3_DCSRCDRP, RK_TCPC_CTRL3_DCSRCDRP_30);

	/* Enable lp_mode_en/cc_ov_det_en/lg_typec_en */
	reg |= RK_TCPC_CTRL3_TYPEC_EN | RK_TCPC_CTRL3_LP_MODE_EN;

	ret = rk_tcpc_write8(chip, RK_TCPC_CTRL3, reg);
	if (ret < 0)
		return ret;

	rk_tcpc_log(chip, "ctrl3(06h): %02x", reg);

	return 0;
}

static int tcpm_get_vbus(struct tcpc_dev *dev)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg;
	int ret;

	mutex_lock(&chip->lock);

	ret = rk_tcpc_read8(chip, RK_TCPC_STS1, &reg);
	if (ret < 0) {
		mutex_unlock(&chip->lock);
		return 0;
	}

	chip->vbus_present = !!(reg & RK_TCPC_STS_VBUS);
	ret = chip->vbus_present;
	rk_tcpc_log(chip, "sts1(08h) : %02x, vbus present %d", reg, chip->vbus_present);

	mutex_unlock(&chip->lock);
	return ret;
}

static const char * const cc_status_name[] = {
	[TYPEC_CC_OPEN]		= "Open",
	[TYPEC_CC_RA]		= "Ra",
	[TYPEC_CC_RD]		= "Rd",
	[TYPEC_CC_RP_DEF]	= "Rp-def",
	[TYPEC_CC_RP_1_5]	= "Rp-1.5",
	[TYPEC_CC_RP_3_0]	= "Rp-3.0",
};

static int tcpm_set_cc(struct tcpc_dev *dev, enum typec_cc_status cc)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg = 0;
	int ret;

	switch (cc) {
	case TYPEC_CC_OPEN: /* cc_rd_en = 0 && rp_value = 0x0 */
		break;
	case TYPEC_CC_RD:
		ret = rk_tcpc_read8(chip, RK_TCPC_CTRL2, &reg);
		if (ret)
			return ret;
		reg |= RK_TCPC_CTRL2_CC_RD_EN;
		break;
	case TYPEC_CC_RP_DEF:
		reg |= RK_TCPC_CTRL2_CC_RP_DEF;
		break;
	case TYPEC_CC_RP_1_5:
		reg |= RK_TCPC_CTRL2_CC_RP_1_5;
		break;
	case TYPEC_CC_RP_3_0:
		reg |= RK_TCPC_CTRL2_CC_RP_3_0;
		break;
	default:
		rk_tcpc_log(chip, "unsupported cc value %s", cc_status_name[cc]);
		return -EINVAL;
	}

	ret = rk_tcpc_write8(chip, RK_TCPC_CTRL2, reg);
	if (ret)
		return ret;

	rk_tcpc_log(chip, "cc := %s, ctl2(05h) : %02x", cc_status_name[cc], reg);

	return 0;
}

static int tcpm_get_cc(struct tcpc_dev *dev, enum typec_cc_status *cc1,
		       enum typec_cc_status *cc2)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 sts, sts1, togdone, state;
	u16 reg;
	int ret;

	ret = rk_tcpc_block_read(chip, RK_TCPC_STS, (u8 *)&reg, 2);
	if (ret < 0)
		return ret;

	sts = reg & 0x00FF;
	sts1 = reg >> 8;

	rk_tcpc_log(chip, "status(07h) : %02x, status1(08h) : %02x", sts, sts1);

	/* Escape CC Open state during PD transfer */
	state = FIELD_GET(RK_TCPC_STS_TYPEC_STATE, sts1);
	if (((sts & 0xf) == 0) &&
	    (state == RK_TCPC_STS_ATTACHED_DB_SNK || state == RK_TCPC_STS_DETACH_DB_SNK)) {
		*cc1 = chip->cc1;
		*cc2 = chip->cc2;

		rk_tcpc_log(chip, "keep the previous cc value");
		return 0;
	}

	togdone = FIELD_GET(RK_TCPC_STS_TOGSS, sts);
	switch (togdone) {
	case RK_TCPC_STS_TOGSS_RD:
		rk_tcpc_log(chip, "presenting Rd");
		*cc1 = rk_tcpc_sts_to_cc(FIELD_GET(RK_TCPC_STS_CC1, sts), true);
		*cc2 = rk_tcpc_sts_to_cc(FIELD_GET(RK_TCPC_STS_CC2, sts), true);

		/* Handle special cable that both CC1 and CC2 pins are simultaneously pulled up. */
		if (*cc1 == TYPEC_CC_RP_DEF && *cc2 == TYPEC_CC_RP_DEF) {
			rk_tcpc_log(chip, "special cable detected, set cc2 to Open");
			*cc2 = TYPEC_CC_OPEN;
		}
		break;

	case RK_TCPC_STS_TOGSS_RP:
		rk_tcpc_log(chip, "presenting Rp");
		*cc1 = rk_tcpc_sts_to_cc(FIELD_GET(RK_TCPC_STS_CC1, sts), false);
		*cc2 = rk_tcpc_sts_to_cc(FIELD_GET(RK_TCPC_STS_CC2, sts), false);
		break;

	case RK_TCPC_STS_TOGSS_RUNNING:
	default:
		rk_tcpc_log(chip, "TOGDONE with an invalid state");
		*cc1 = TYPEC_CC_OPEN;
		*cc2 = TYPEC_CC_OPEN;
		break;
	}

	chip->cc1 = *cc1;
	chip->cc2 = *cc2;

	rk_tcpc_log(chip, "detected: cc1=%s, cc2=%s", cc_status_name[*cc1], cc_status_name[*cc2]);
	return 0;
}

static const char * const cc_polarity_name[] = {
	[TYPEC_POLARITY_CC1]	= "Polarity_CC1",
	[TYPEC_POLARITY_CC2]	= "Polarity_CC2",
};

static int tcpm_set_polarity(struct tcpc_dev *dev,
			     enum typec_cc_polarity polarity)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg = 0;
	int ret;

	reg |= polarity == TYPEC_POLARITY_CC2 ? RK_TCPC_CTRL_CC_POLARITY_CC2 : 0;
	ret = rk_tcpc_write8(chip, RK_TCPC_CTRL, reg);
	if (ret < 0)
		return ret;

	rk_tcpc_log(chip, "set %s", cc_polarity_name[polarity]);

	return 0;
}

static int tcpm_set_vconn(struct tcpc_dev *dev, bool on)
{
	/* unsupported */
	return 0;
}

static int tcpm_set_vbus(struct tcpc_dev *dev, bool on, bool charge)
{
	return 0;
}

static const char * const typec_role_name[] = {
	[TYPEC_SINK]		= "Sink",
	[TYPEC_SOURCE]		= "Source",
};

static const char * const typec_data_role_name[] = {
	[TYPEC_DEVICE]		= "Device",
	[TYPEC_HOST]		= "Host",
};

static int tcpm_set_roles(struct tcpc_dev *dev, bool attached,
			  enum typec_role role, enum typec_data_role data)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg;
	int ret;

	reg = FIELD_PREP(RK_TCPC_RX_DET_PD_SPEC_REV, PD_REV20);
	if (role == TYPEC_SOURCE)
		reg |= RK_TCPC_RX_DET_PWR_ROLE_SRC;
	if (data == TYPEC_HOST)
		reg |= RK_TCPC_RX_DET_DATA_ROLE_DFP;

	ret = rk_tcpc_write8(chip, RK_TCPC_RX_DET, reg);
	if (ret < 0) {
		rk_tcpc_log(chip, "cannot set roles %s, %s, ret=%d",
			    typec_role_name[role], typec_data_role_name[data],
			    ret);
		return ret;
	}

	rk_tcpc_log(chip, "set roles := %s, %s", typec_role_name[role],
		    typec_data_role_name[data]);

	return 0;
}

static int tcpm_start_toggling(struct tcpc_dev *dev,
			       enum typec_port_type port_type,
			       enum typec_cc_status cc)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg = 0;
	int ret;

	if (port_type != TYPEC_PORT_DRP)
		return -EOPNOTSUPP;

	rk_tcpc_log(chip, "cc value %s", cc_status_name[cc]);

	switch (cc) {
	case TYPEC_CC_RD:
		reg |= RK_TCPC_CTRL2_CC_RD_EN;
		/* fallthrough; */
	case TYPEC_CC_RP_DEF:
		reg |= RK_TCPC_CTRL2_CC_RP_DEF;
		break;
	case TYPEC_CC_RP_1_5:
		reg |= RK_TCPC_CTRL2_CC_RP_1_5;
		break;
	case TYPEC_CC_RP_3_0:
		reg |= RK_TCPC_CTRL2_CC_RP_3_0;
		break;
	default:
		rk_tcpc_log(chip, "unsupported cc value %s", cc_status_name[cc]);
		return -EINVAL;
	}

	reg |= RK_TCPC_CTRL2_TOGGLE;
	ret = rk_tcpc_write8(chip, RK_TCPC_CTRL2, reg);
	if (ret)
		return ret;

	rk_tcpc_log(chip, "start drp toggling, ctrl2(05h): %02x", reg);

	return 0;
}

static int tcpm_set_pd_rx(struct tcpc_dev *dev, bool on)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg = 0;
	int ret;

	mutex_lock(&chip->lock);

	if (on)
		reg = RK_TCPC_RX_DET1_SOP_EN | RK_TCPC_RX_DET1_HARD_RST_EN;
	ret = rk_tcpc_write8(chip, RK_TCPC_RX_DET1, reg);
	if (ret < 0) {
		rk_tcpc_log(chip, "cannot set rx detect %s", str_on_off(on));
		goto out;
	} else {
		rk_tcpc_log(chip, "set rx detect %s", str_on_off(on));
	}

	ret = rk_tcpc_enable_rx(chip, on);
	if (ret)
		goto out;

	ret = rk_tcpc_enable_tx(chip, on);
	if (ret)
		goto out;

out:
	mutex_unlock(&chip->lock);
	return ret;
}

static const char * const tx_type_name[] = {
	[TCPC_TX_SOP]			= "SOP",
	[TCPC_TX_SOP_PRIME]		= "SOP'",
	[TCPC_TX_SOP_PRIME_PRIME]	= "SOP''",
	[TCPC_TX_SOP_DEBUG_PRIME]	= "DEBUG'",
	[TCPC_TX_SOP_DEBUG_PRIME_PRIME]	= "DEBUG''",
	[TCPC_TX_HARD_RESET]		= "HARD_RESET",
	[TCPC_TX_CABLE_RESET]		= "CABLE_RESET",
	[TCPC_TX_BIST_MODE_2]		= "BIST_MODE_2",
};

static int tcpm_pd_transmit(struct tcpc_dev *dev, enum tcpm_transmit_type type,
			    const struct pd_message *msg, unsigned int negotiated_rev)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u16 header = 0;
	u8 cnt = 0;
	u8 reg;
	int ret = 0;

	mutex_lock(&chip->lock);

	if (type == TCPC_TX_BIST_MODE_2) {
		ret = rk_tcpc_read8(chip, RK_TCPC_TX_CFG1, &reg);
		if (ret < 0)
			goto out;

		reg |= RK_TCPC_TX_CFG1_BIST_TST_MODE;

		ret = rk_tcpc_write8(chip, RK_TCPC_TX_CFG1, reg);
		if (ret < 0)
			goto out;

		rk_tcpc_log(chip, "TX bist mode, tx_cfg1(0eh): %02x", reg);
		goto out;
	}

	if (msg) {
		header = le16_to_cpu(msg->header);
		cnt = pd_header_cnt(header) * 4;

		ret = rk_tcpc_write8(chip, RK_TCPC_TX_CTRL, cnt + 2);
		if (ret < 0)
			goto out;

		ret = rk_tcpc_block_write(chip, RK_TCPC_PD_HEADER, (u8 *)&header, 2);
		if (ret < 0)
			goto out;
	}

	if (cnt > 0) {
		ret = rk_tcpc_block_write(chip, RK_TCPC_PD_DATA, (u8 *)&msg->payload, cnt);
		if (ret < 0)
			goto out;
	}

	/* nRetryCount is 3 in PD2.0 spec where 2 in PD3.0 spec */
	reg = FIELD_PREP(RK_TCPC_TX_CFG_RETRY,
			 (negotiated_rev > PD_REV20 ?
			  RK_TCPC_TX_CFG_RETRY_CNT_2 :
			  RK_TCPC_TX_CFG_RETRY_CNT_3));
	reg |= FIELD_PREP(RK_TCPC_TX_CFG_MSG_TYPE, type);
	ret = rk_tcpc_write8(chip, RK_TCPC_TX_CFG, reg);
	if (ret < 0) {
		rk_tcpc_log(chip, "cannot send PD message type %s , ret=%d",
			    tx_type_name[type], ret);
		goto out;
	}

	/* Enable TX bmc */
	ret = rk_tcpc_write8(chip, RK_TCPC_TX_CFG2, RK_TCPC_TX_CFG2_BMC_EN);
	if (ret < 0)
		goto out;

	if (msg)
		rk_tcpc_log(chip, "sending PD message header: %#x, len: %d", msg->header, cnt);
	else
		rk_tcpc_log(chip, "sending PD message type: %s", tx_type_name[type]);

out:
	mutex_unlock(&chip->lock);
	return ret;
}

static int tcpm_set_bist_data(struct tcpc_dev *dev, bool on)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	u8 reg;
	int ret = 0;

	/* RX Interrupt */
	ret = rk_tcpc_read8(chip, RK_TCPC_INT, &reg);
	if (ret < 0)
		return ret;

	if (on)
		reg &= ~RK_TCPC_INT_RX_SUCCESS;
	else
		reg |= RK_TCPC_INT_RX_SUCCESS;

	ret = rk_tcpc_write8(chip, RK_TCPC_INT, reg);
	if (ret < 0)
		return ret;

	/* HW debounce */
	ret = rk_tcpc_read8(chip, RK_TCPC_DB_CTRL, &reg);
	if (ret < 0)
		return ret;

	if (on)
		reg &= ~RK_TCPC_DB_HW_DISABLE;
	else
		reg |= RK_TCPC_DB_HW_DISABLE;

	ret = rk_tcpc_write8(chip, RK_TCPC_DB_CTRL, reg);
	if (ret < 0)
		return ret;

	return 0;
}

static irqreturn_t rk_tcpc_irq_work(struct rk_tcpc_chip *chip);

static void tcpm_poll_interrupt_event(struct tcpc_dev *dev)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);

	if (!chip->int_present)
		return;

	rk_tcpc_irq_work(chip);

	chip->int_present = false;
	irq_handler_hw_enable(chip->irq);
}

static int tcpm_enter_low_power_mode(struct tcpc_dev *dev, bool attached, bool pd_capable)
{
	struct rk_tcpc_chip *chip = container_of(dev, struct rk_tcpc_chip, tcpc_dev);
	int ret;

	rk_tcpc_log(chip, "enter low power mode, attached=%d, pd_capable=%d", attached, pd_capable);

	/* Disable chip interrupts */
	ret = rk_tcpc_write8(chip, RK_TCPC_INT, 0);
	if (ret < 0)
		return ret;

	/* Clear all events */
	ret = rk_tcpc_write8(chip, RK_TCPC_INT_STS, RK_TCPC_INT_STS_MASK);
	if (ret < 0)
		return ret;

	if (attached && pd_capable)
		return 0;

	return rk_tcpc_set_lpmode(chip);
}

static void rk_tcpc_init_tcpc_dev(struct tcpc_dev *rk_tcpc_dev)
{
	rk_tcpc_dev->init = tcpm_init;
	rk_tcpc_dev->get_vbus = tcpm_get_vbus;
	rk_tcpc_dev->set_cc = tcpm_set_cc;
	rk_tcpc_dev->get_cc = tcpm_get_cc;
	rk_tcpc_dev->set_polarity = tcpm_set_polarity;
	rk_tcpc_dev->set_vconn = tcpm_set_vconn;
	rk_tcpc_dev->set_vbus = tcpm_set_vbus;
//	rk_tcpc_dev->is_vbus_vsafe0v = tcpm_is_vbus_vsafe0v;
	rk_tcpc_dev->set_pd_rx = tcpm_set_pd_rx;
	rk_tcpc_dev->set_roles = tcpm_set_roles;
	rk_tcpc_dev->set_bist_data = tcpm_set_bist_data;
	rk_tcpc_dev->start_toggling = tcpm_start_toggling;
	rk_tcpc_dev->pd_transmit = tcpm_pd_transmit;
	rk_tcpc_dev->poll_event = tcpm_poll_interrupt_event;
	rk_tcpc_dev->enter_low_power_mode = tcpm_enter_low_power_mode;
}

static irqreturn_t rk_tcpc_irq_work(struct rk_tcpc_chip *chip)
{
	u8 int_status, status1;
	bool vbus_present;
	int ret;

	ret = rk_tcpc_read8(chip, RK_TCPC_INT_STS, &int_status);
	if (ret < 0)
		return IRQ_HANDLED;

	if (!(FIELD_GET(RK_TCPC_INT_STS_MASK, int_status)))
		return IRQ_NONE;

	rk_tcpc_log(chip, "IRQ(01h): 0x%02x", int_status);

	/*
	 * Clear alert status for everything except RX_STATUS, which shouldn't
	 * be cleared until we have successfully retrieved message.
	 */
	if (int_status & ~RK_TCPC_INT_STS_RX_SUCCESS)
		rk_tcpc_write8(chip, RK_TCPC_INT_STS,
			       int_status & ~RK_TCPC_INT_STS_RX_SUCCESS);

	if (int_status & RK_TCPC_INT_STS_VBUS) {
		rk_tcpc_read8(chip, RK_TCPC_STS1, &status1);
		vbus_present = !!(status1 & RK_TCPC_STS_VBUS);
		rk_tcpc_log(chip, "IRQ: VBUS %s", str_on_off(vbus_present));

		if (vbus_present != chip->vbus_present) {
			chip->vbus_present = vbus_present;
			tcpm_vbus_change(chip->tcpm_port);
		}
	}

	if (int_status & RK_TCPC_INT_STS_CC) {
		rk_tcpc_log(chip, "IRQ: CC change");
		tcpm_cc_change(chip->tcpm_port);
	}

	if (int_status & RK_TCPC_INT_STS_RX_SUCCESS) {
		struct pd_message msg;

		rk_tcpc_log(chip, "IRQ: PD rx success");
		ret = rk_tcpc_pd_read_message(chip, &msg);
		if (ret < 0) {
			rk_tcpc_log(chip, "cannot read PD message, ret=%d", ret);
			return IRQ_HANDLED;
		}

		/* Read complete, clear RX interrupt status bit */
		rk_tcpc_write8(chip, RK_TCPC_INT_STS, RK_TCPC_INT_STS_RX_SUCCESS);

		tcpm_pd_receive(chip->tcpm_port, &msg);
	}

	if (int_status & RK_TCPC_INT_STS_TX_SUCCESS) {
		rk_tcpc_log(chip, "IRQ: PD tx success");
		tcpm_pd_transmit_complete(chip->tcpm_port, TCPC_TX_SUCCESS);
	} else if (int_status & RK_TCPC_INT_STS_TX_FAILED) {
		rk_tcpc_log(chip, "IRQ: PD tx failed");
		tcpm_pd_transmit_complete(chip->tcpm_port, TCPC_TX_FAILED);
	}

	if (int_status & RK_TCPC_INT_STS_RX_HARD_RST) {
		rk_tcpc_log(chip, "IRQ: PD received hardreset");
		tcpm_pd_hard_reset(chip->tcpm_port);
	}

	if (int_status & RK_TCPC_INT_STS_VSAFE0V) {
		rk_tcpc_read8(chip, RK_TCPC_STS1, &status1);
		rk_tcpc_log(chip, "IRQ: VSAFE0V %d", !!(status1 & RK_TCPC_STS_VSAFE0V));
	}

	if (int_status & RK_TCPC_INT_STS_CC_OV) {
		rk_tcpc_log(chip, "IRQ: CC overvoltage");
		WARN(1, "RK TCPC CC Overvoltage");
	}

	return IRQ_HANDLED;
}

static void rk_tcpc_interrupt(int irq, void *data)
{
	struct rk_tcpc_chip *chip = dev_get_priv(data);
	u8 int_status;
	int ret;

	ret = rk_tcpc_read8(chip, RK_TCPC_INT_STS, &int_status);
	if (ret < 0)
		return;

	if (!(FIELD_GET(RK_TCPC_INT_STS_MASK, int_status)) || chip->int_present)
		return;

	rk_tcpc_log(chip, "interrupt(01h): 0x%02x, int_present=%d", int_status, chip->int_present);

	ret = irq_handler_hw_disable(irq);
	if (ret)
		printf("%s: failed to disable irq: %d\n", chip->dev->name, ret);

	chip->int_present = true;
}

static int rk_tcpc_init_gpio_irq(struct rk_tcpc_chip *chip)
{
	struct udevice *dev = chip->dev;
	u32 interrupt, phandle;
	int ret;

	phandle = dev_read_u32_default(dev, "interrupt-parent", -ENODATA);
	if (phandle == -ENODATA) {
		printf("%s: read interrupt-parent failed, ret=%d\n", dev->name, phandle);
		return phandle;
	}

	ret = dev_read_u32_array(dev, "interrupts", &interrupt, 1);
	if (ret) {
		printf("%s: read interrupts failed, ret=%d\n", dev->name, ret);
		return ret;
	}

	chip->irq = phandle_gpio_to_irq(phandle, interrupt);
	if (chip->irq < 0) {
		printf("%s: failed to request irq: %d\n", dev->name, chip->irq);
		return ret;
	}

	return 0;
}

static int rk_tcpc_probe(struct udevice *dev)
{
	struct rk_tcpc_chip *chip = dev_get_priv(dev);
	int ret;

	chip->dev = dev;
	mutex_init(&chip->lock);

	ret = rk_tcpc_check_id(chip);
	if (ret < 0)
		return ret;

	chip->tcpc_dev.connector_node = dev_read_subnode(dev, "connector");
	if (!ofnode_valid(chip->tcpc_dev.connector_node)) {
		printf("%s: 'connector' node is not found\n", __func__);
		return -ENODEV;
	}

	rk_tcpc_init_tcpc_dev(&chip->tcpc_dev);

	ret = rk_tcpc_init_gpio_irq(chip);
	if (ret)
		return ret;

	chip->tcpm_port = tcpm_port_init(dev, &chip->tcpc_dev);
        if (IS_ERR(chip->tcpm_port)) {
                printf("%s: failed to tcpm port init\n", __func__);
                return PTR_ERR(chip->tcpm_port);
        }

	irq_install_handler_flags(chip->irq, rk_tcpc_interrupt,
				  chip->dev, IRQF_SHARED | IRQF_HW_CTRL);

	ret = rk_tcpc_init_interrupt(chip);
	if (ret) {
		printf("%s: cannot init interrupt, ret=%d\n", __func__, ret);
		return ret;;
	}

	ret = irq_handler_enable(chip->irq);
	if (ret) {
		printf("%s: failed to enable irq: %d\n", dev->name, ret);
		irq_free_handler(chip->irq);
		return ret;
	}

	tcpm_poll_event(chip->tcpm_port);

	return 0;
}

static int rk_tcpc_remove(struct udevice *dev)
{
	struct rk_tcpc_chip *chip = dev_get_priv(dev);
	int ret;

	printf("%s", __func__);

	/* Disable chip interrupts before unregistering port */
	ret = rk_tcpc_write8(chip, RK_TCPC_INT, 0);
	if (ret < 0)
		return ret;

	tcpm_uninit_port(chip->tcpm_port);
//	of_node_put(&chip->tcpc_dev.connector_node);

	return 0;
}

static int rk_tcpc_get_voltage(struct udevice *dev)
{
	struct rk_tcpc_chip *chip = dev_get_priv(dev);

	return tcpm_get_voltage(chip->tcpm_port);
}

static int rk_tcpc_get_current(struct udevice *dev)
{
	struct rk_tcpc_chip *chip = dev_get_priv(dev);

	return tcpm_get_current(chip->tcpm_port);
}

static int rk_tcpc_get_online(struct udevice *dev)
{
	struct rk_tcpc_chip *chip = dev_get_priv(dev);

	return tcpm_get_online(chip->tcpm_port);
}

static struct dm_power_delivery_ops rk_tcpc_pd_ops = {
	.get_voltage	= rk_tcpc_get_voltage,
	.get_current	= rk_tcpc_get_current,
	.get_online	= rk_tcpc_get_online,
};

static const struct udevice_id rk_tcpc_dt_match[] = {
	{.compatible = "rockchip,rk817b2-tcpc"},
	{},
};

U_BOOT_DRIVER(rockchip_pmic_tcpc) = {
	.name = "rockchip,pmic-tcpc",
	.id = UCLASS_PD,
	.of_match = rk_tcpc_dt_match,
	.ops = &rk_tcpc_pd_ops,
	.probe = rk_tcpc_probe,
	.remove = rk_tcpc_remove,
	.priv_auto = sizeof(struct rk_tcpc_chip),
};

MODULE_AUTHOR("Frank Wang <frank.wang@rock-chips.com>");
MODULE_DESCRIPTION("Rockchip PMIC Type-C Port Controller Driver");
MODULE_LICENSE("GPL");
