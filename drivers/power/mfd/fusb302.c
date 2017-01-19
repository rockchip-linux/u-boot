/*
 * (C) Copyright 2017 Fuzhou Rockchip Electronics Co., Ltd
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
#include "fusb302.h"
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

#define FUSB30X_I2C_DEVICETREE_NAME     "fairchild,fusb302"

#define FUSB302_I2C_SPEED		100000
#define CHARGE_INPUT_DEFAULT_CUR	2000
#define CHARGE_INPUT_DEFAULT_VOL	5000

#define FUSB_MODE_DRP		0
#define FUSB_MODE_UFP		1
#define FUSB_MODE_DFP		2
#define FUSB_MODE_ASS		3

#define TYPEC_CC_VOLT_OPEN	0
#define TYPEC_CC_VOLT_RA	1
#define TYPEC_CC_VOLT_RD	2
#define TYPEC_CC_VOLT_RP	3

#define EVENT_CC		0x1
#define EVENT_RX		0x2
#define EVENT_TX		0x4
#define EVENT_REC_RESET		0x8

static struct fusb30x_chip chip;

static int fusb302_i2c_probe(u32 bus, u32 addr)
{
	int ret;

	i2c_set_bus_num(bus);
	i2c_init(FUSB302_I2C_SPEED, 0);
	ret = i2c_probe(addr);
	if (ret < 0)
		return -ENODEV;

	return 0;
}

int fusb302_i2c_read(u8 reg, u8 *val)
{
	int ret;

	ret = i2c_read(chip.addr, reg, 1, val, 1);
	if (ret != 0)
		printf("fusb302 i2c read error!!!\n");

	return ret;
}

int fusb302_i2c_write(u8 reg, u8 val)
{
	int ret;

	ret = i2c_write(chip.addr, reg, 1, (u8 *)&val, 1);
	if (ret != 0)
		printf("fusb302 i2c write error!!!\n");

	return ret;
}

int fusb302_i2c_raw_write(u8 reg, const void *val, size_t val_len)
{
	int ret;

	ret = i2c_write(chip.addr, reg, 1, (u8 *)val, val_len);
	if (ret != 0)
		printf("fusb302 i2c write error!!!\n");

	return ret;
}

int fusb302_i2c_raw_read(u8 reg, void *val, size_t val_len)
{
	int ret;

	ret = i2c_read(chip.addr, reg, 1, val, val_len);
	if (ret != 0)
		printf("fusb302 i2c read error!!!\n");

	return ret;
}

int fusb302_i2c_update_bits(u8 reg, u8 mask, uchar val)
{
	int ret;
	u8 tmp, orig;

	ret = fusb302_i2c_read(reg, &orig);
	if (ret != 0)
		return ret;

	tmp = orig & ~mask;
	tmp |= val & mask;

	if (tmp != orig)
		ret = fusb302_i2c_write(reg, tmp);

	return ret;
}

static void cc_interrupt_handler(void);
static void enable_irq(void)
{
	irq_install_handler(chip.gpio_irq, (interrupt_handler_t *)cc_interrupt_handler, NULL);
	irq_set_irq_type(chip.gpio_irq, IRQ_TYPE_LEVEL_LOW);
	irq_handler_enable(chip.gpio_irq);
}

static void disable_irq(void)
{
	irq_uninstall_handler(chip.gpio_irq);
	irq_handler_disable(chip.gpio_irq);
}

static void set_state(struct fusb30x_chip *chip, enum connection_state state)
{
	if (!state)
		debug("PD disabled\n");
	chip->conn_state = state;
	chip->sub_state = 0;
	chip->val_tmp = 0;
}

static int tcpm_get_message(struct fusb30x_chip *chip)
{
	u8 buf[32];
	int len;

	fusb302_i2c_raw_read(FUSB_REG_FIFO, buf, 3);
	chip->rec_head = (buf[1] & 0xff) | ((buf[2] << 8) & 0xff00);

	len = PD_HEADER_CNT(chip->rec_head) << 2;
	fusb302_i2c_raw_read(FUSB_REG_FIFO, buf, len + 4);

	memcpy(chip->rec_load, buf, len);

	return 0;
}

static void fusb302_flush_rx_fifo(struct fusb30x_chip *chip)
{
	tcpm_get_message(chip);
}

static int tcpm_get_cc(struct fusb30x_chip *chip, int *CC1, int *CC2)
{
	int *CC_MEASURE;
	u8 store, val;

	*CC1 = TYPEC_CC_VOLT_OPEN;
	*CC2 = TYPEC_CC_VOLT_OPEN;

	if (chip->cc_state & 0x01)
		CC_MEASURE = CC1;
	else
		CC_MEASURE = CC2;

	if (chip->cc_state & 0x04) {
		fusb302_i2c_read(FUSB_REG_SWITCHES0, &store);
		/* measure cc1 first */
		fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
				   SWITCHES0_MEAS_CC1 | SWITCHES0_MEAS_CC2 |
				   SWITCHES0_PU_EN1 | SWITCHES0_PU_EN2 |
				   SWITCHES0_PDWN1 | SWITCHES0_PDWN2,
				   SWITCHES0_PDWN1 | SWITCHES0_PDWN2 |
				   SWITCHES0_MEAS_CC1);
		udelay(300);

		fusb302_i2c_read(FUSB_REG_STATUS0, &val);
		val &= STATUS0_BC_LVL;
		if (val)
			*CC1 = val;

		fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
				   SWITCHES0_MEAS_CC1 | SWITCHES0_MEAS_CC2 |
				   SWITCHES0_PU_EN1 | SWITCHES0_PU_EN2 |
				   SWITCHES0_PDWN1 | SWITCHES0_PDWN2,
				   SWITCHES0_PDWN1 | SWITCHES0_PDWN2 |
				   SWITCHES0_MEAS_CC2);
		udelay(300);

		fusb302_i2c_read(FUSB_REG_STATUS0, &val);
		val &= STATUS0_BC_LVL;
		if (val)
			*CC2 = val;
		fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
				   SWITCHES0_MEAS_CC1 | SWITCHES0_MEAS_CC2,
				   store);
	} else {
		fusb302_i2c_read(FUSB_REG_SWITCHES0, &store);
		val = store;
		val &= ~(SWITCHES0_MEAS_CC1 | SWITCHES0_MEAS_CC2 |
				SWITCHES0_PU_EN1 | SWITCHES0_PU_EN2);
		if (chip->cc_state & 0x01)
			val |= SWITCHES0_MEAS_CC1 | SWITCHES0_PU_EN1;
		else
			val |= SWITCHES0_MEAS_CC2 | SWITCHES0_PU_EN2;

		fusb302_i2c_write(FUSB_REG_SWITCHES0, val);

		fusb302_i2c_write(FUSB_REG_MEASURE, chip->cc_meas_high);
		udelay(300);

		fusb302_i2c_read(FUSB_REG_STATUS0, &val);
		if (val & STATUS0_COMP) {
			int retry = 3;
			int comp_times = 0;

			while (retry--) {
				fusb302_i2c_write(FUSB_REG_MEASURE, chip->cc_meas_high);
				udelay(300);
				fusb302_i2c_read(FUSB_REG_STATUS0, &val);
				if (val & STATUS0_COMP) {
					comp_times++;
					if (comp_times == 3) {
						*CC_MEASURE = TYPEC_CC_VOLT_OPEN;
						fusb302_i2c_write(FUSB_REG_SWITCHES0, store);
					}
				}
			}
		} else {
			fusb302_i2c_write(FUSB_REG_MEASURE, chip->cc_meas_low);
			fusb302_i2c_read(FUSB_REG_MEASURE, &val);
			udelay(300);

			fusb302_i2c_read(FUSB_REG_STATUS0, &val);

			if (val & STATUS0_COMP)
				*CC_MEASURE = TYPEC_CC_VOLT_RD;
			else
				*CC_MEASURE = TYPEC_CC_VOLT_RA;
			fusb302_i2c_write(FUSB_REG_SWITCHES0, store);
		}
	}

	return 0;
}

static int tcpm_set_cc(struct fusb30x_chip *chip, int mode)
{
	u8 val = 0, mask;

	val &= ~(SWITCHES0_PU_EN1 | SWITCHES0_PU_EN2 |
		 SWITCHES0_PDWN1 | SWITCHES0_PDWN2);

	mask = ~val;

	switch (mode) {
	case FUSB_MODE_DFP:
		if (chip->togdone_pullup)
			val |= SWITCHES0_PU_EN2;
		else
			val |= SWITCHES0_PU_EN1;
		break;
	case FUSB_MODE_UFP:
		val |= SWITCHES0_PDWN1 | SWITCHES0_PDWN2;
		break;
	case FUSB_MODE_DRP:
		val |= SWITCHES0_PDWN1 | SWITCHES0_PDWN2;
		break;
	case FUSB_MODE_ASS:
		break;
	}

	fusb302_i2c_update_bits(FUSB_REG_SWITCHES0, mask, val);
	return 0;
}

static int tcpm_set_rx_enable(struct fusb30x_chip *chip, int enable)
{
	u8 val = 0;

	if (enable) {
		if (chip->cc_polarity)
			val |= SWITCHES0_MEAS_CC2;
		else
			val |= SWITCHES0_MEAS_CC1;
		fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
				   SWITCHES0_MEAS_CC1 | SWITCHES0_MEAS_CC2,
				   val);
		fusb302_flush_rx_fifo(chip);
		fusb302_i2c_update_bits(FUSB_REG_SWITCHES1,
				   SWITCHES1_AUTO_CRC, SWITCHES1_AUTO_CRC);
	} else {
		/*
		 * bit of a hack here.
		 * when this function is called to disable rx (enable=0)
		 * using it as an indication of detach (gulp!)
		 * to reset our knowledge of where
		 * the toggle state machine landed.
		 */
		chip->togdone_pullup = 0;

#ifdef FUSB_HAVE_DRP
		tcpm_set_cc(chip, FUSB_MODE_DRP);
		fusb302_i2c_update_bits(FUSB_REG_CONTROL2,
				   CONTROL2_TOG_RD_ONLY,
				   CONTROL2_TOG_RD_ONLY);
#endif
		fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
				   SWITCHES0_MEAS_CC1 | SWITCHES0_MEAS_CC2,
				   0);
		fusb302_i2c_update_bits(FUSB_REG_SWITCHES1, SWITCHES1_AUTO_CRC, 0);
	}

	return 0;
}

static int tcpm_set_msg_header(struct fusb30x_chip *chip)
{
	fusb302_i2c_update_bits(FUSB_REG_SWITCHES1,
			   SWITCHES1_POWERROLE | SWITCHES1_DATAROLE,
			   (chip->power_role << 7) |
			   (chip->data_role << 4));
	fusb302_i2c_update_bits(FUSB_REG_SWITCHES1,
			   SWITCHES1_SPECREV, 2 << 5);
	return 0;
}

static int tcpm_set_polarity(struct fusb30x_chip *chip, bool polarity)
{
	u8 val = 0;

#ifdef FUSB_VCONN_SUPPORT
	if (chip->vconn_enabled) {
		if (polarity)
			val |= SWITCHES0_VCONN_CC1;
		else
			val |= SWITCHES0_VCONN_CC2;
	}
#endif

	if (polarity)
		val |= SWITCHES0_MEAS_CC2;
	else
		val |= SWITCHES0_MEAS_CC1;

	fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
			   SWITCHES0_VCONN_CC1 | SWITCHES0_VCONN_CC2 |
			   SWITCHES0_MEAS_CC1 | SWITCHES0_MEAS_CC2,
			   val);

	val = 0;
	if (polarity)
		val |= SWITCHES1_TXCC2;
	else
		val |= SWITCHES1_TXCC1;
	fusb302_i2c_update_bits(FUSB_REG_SWITCHES1,
			   SWITCHES1_TXCC1 | SWITCHES1_TXCC2,
			   val);

	chip->cc_polarity = polarity;

	return 0;
}

static int tcpm_set_vconn(struct fusb30x_chip *chip, int enable)
{
	u8 val = 0;

	if (enable) {
		tcpm_set_polarity(chip, chip->cc_polarity);
	} else {
		val &= ~(SWITCHES0_VCONN_CC1 | SWITCHES0_VCONN_CC2);
		fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
				   SWITCHES0_VCONN_CC1 | SWITCHES0_VCONN_CC2,
				   val);
	}
	chip->vconn_enabled = enable;
	return 0;
}

static void fusb302_pd_reset(struct fusb30x_chip *chip)
{
	fusb302_i2c_write(FUSB_REG_RESET, RESET_PD_RESET);
}

static void tcpm_select_rp_value(struct fusb30x_chip *chip, u32 rp)
{
	u8 control0_reg;

	fusb302_i2c_read(FUSB_REG_CONTROL0, &control0_reg);

	control0_reg &= ~CONTROL0_HOST_CUR;
	/*
	 * according to the host current, the compare value is different
	*/
	switch (rp) {
	/* host pull up current is 80ua , high voltage is 1.596v, low is 0.21v */
	case TYPEC_RP_USB:
		chip->cc_meas_high = 0x26;
		chip->cc_meas_low = 0x5;
		control0_reg |= CONTROL0_HOST_CUR_USB;
		break;
	/* host pull up current is 180ua , high voltage is 1.596v, low is 0.42v */
	case TYPEC_RP_1A5:
		chip->cc_meas_high = 0x26;
		chip->cc_meas_low = 0xa;
		control0_reg |= CONTROL0_HOST_CUR_1A5;
		break;
	/* host pull up current is 330ua , high voltage is 2.604v, low is 0.798v*/
	case TYPEC_RP_3A0:
		chip->cc_meas_high = 0x26;
		chip->cc_meas_low = 0x13;
		control0_reg |= CONTROL0_HOST_CUR_3A0;
		break;
	default:
		chip->cc_meas_high = 0x26;
		chip->cc_meas_low = 0xa;
		control0_reg |= CONTROL0_HOST_CUR_1A5;
		break;
	}

	fusb302_i2c_write(FUSB_REG_CONTROL0, control0_reg);
}

static void tcpm_init(struct fusb30x_chip *chip)
{
	u8 val;
	u8 tmp = 0;

	fusb302_i2c_read(FUSB_REG_DEVICEID, &tmp);
	i2c_read(chip->addr, FUSB_REG_DEVICEID, 1, (u8 *)&tmp, 1);
	chip->chip_id = (u8)tmp;

	chip->is_cc_connected = 0;
	chip->cc_state = 0;

	/* restore default settings */
	fusb302_i2c_update_bits(FUSB_REG_RESET, RESET_SW_RESET,
			   RESET_SW_RESET);
	fusb302_pd_reset(chip);
	/* set auto_retry and number of retries */
	fusb302_i2c_update_bits(FUSB_REG_CONTROL3,
			   CONTROL3_AUTO_RETRY | CONTROL3_N_RETRIES,
			   CONTROL3_AUTO_RETRY | CONTROL3_N_RETRIES),

	/* set interrupts */
	val = 0xff;
	val &= ~(MASK_M_BC_LVL | MASK_M_COLLISION | MASK_M_ALERT |
		 MASK_M_VBUSOK);
	fusb302_i2c_write(FUSB_REG_MASK, val);

	val = 0xff;
	val &= ~(MASKA_M_TOGDONE | MASKA_M_RETRYFAIL | MASKA_M_HARDSENT |
		 MASKA_M_TXSENT | MASKA_M_HARDRST);
	fusb302_i2c_write(FUSB_REG_MASKA, val);

	val = 0xff;
	val = ~MASKB_M_GCRCSEND;
	fusb302_i2c_write(FUSB_REG_MASKB, val);

#ifdef FUSB_HAVE_DRP
	fusb302_i2c_update_bits(FUSB_REG_CONTROL2,
				   CONTROL2_MODE | CONTROL2_TOGGLE,
				   (1 << 1) | CONTROL2_TOGGLE);

	fusb302_i2c_update_bits(FUSB_REG_CONTROL2,
				   CONTROL2_TOG_RD_ONLY,
				   CONTROL2_TOG_RD_ONLY);
#endif
	tcpm_select_rp_value(chip, TYPEC_RP_1A5);
	/* Interrupts Enable */
	fusb302_i2c_update_bits(FUSB_REG_CONTROL0, CONTROL0_INT_MASK,
			   ~CONTROL0_INT_MASK);

	tcpm_set_polarity(chip, 0);
	tcpm_set_vconn(chip, 0);

	fusb302_i2c_write(FUSB_REG_POWER, 0xf);
}

static int tcpm_check_vbus(struct fusb30x_chip *chip)
{
	u8 val;

	/* Read status register */
	fusb302_i2c_read(FUSB_REG_STATUS0, &val);

	return (val & STATUS0_VBUSOK) ? 1 : 0;
}

static void set_mesg(struct fusb30x_chip *chip, int cmd, int is_DMT)
{
	int i;
	struct PD_CAP_INFO *pd_cap_info = &chip->pd_cap_info;

	chip->send_head = ((chip->msg_id & 0x7) << 9) |
			 ((chip->power_role & 0x1) << 8) |
			 (1 << 6) |
			 ((chip->data_role & 0x1) << 5);

	if (is_DMT) {
		switch (cmd) {
		case DMT_SOURCECAPABILITIES:
			chip->send_head |= ((chip->n_caps_used & 0x3) << 12) | (cmd & 0xf);

			for (i = 0; i < chip->n_caps_used; i++) {
				chip->send_load[i] = (pd_cap_info->supply_type << 30) |
						    (pd_cap_info->dual_role_power << 29) |
						    (pd_cap_info->usb_suspend_support << 28) |
						    (pd_cap_info->externally_powered << 27) |
						    (pd_cap_info->usb_communications_cap << 26) |
						    (pd_cap_info->data_role_swap << 25) |
						    (pd_cap_info->peak_current << 20) |
						    (chip->source_power_supply[i] << 10) |
						    (chip->source_max_current[i]);
			}
			break;
		case DMT_REQUEST:
			chip->send_head |= ((1 << 12) | (cmd & 0xf));
			/* send request with FVRDO */
			chip->send_load[0] = (chip->pos_power << 28) |
					    (0 << 27) |
					    (1 << 26) |
					    (0 << 25) |
					    (0 << 24);

			switch (CAP_POWER_TYPE(chip->rec_load[chip->pos_power - 1])) {
			case 0:
				/* Fixed Supply */
				chip->send_load[0] |= ((CAP_FPDO_VOLTAGE(chip->rec_load[chip->pos_power - 1]) << 10) & 0x3ff);
				chip->send_load[0] |= (CAP_FPDO_CURRENT(chip->rec_load[chip->pos_power - 1]) & 0x3ff);
				break;
			case 1:
				/* Battery */
				chip->send_load[0] |= ((CAP_VPDO_VOLTAGE(chip->rec_load[chip->pos_power - 1]) << 10) & 0x3ff);
				chip->send_load[0] |= (CAP_VPDO_CURRENT(chip->rec_load[chip->pos_power - 1]) & 0x3ff);
				break;
			default:
				/* not meet battery caps */
				break;
			}
			break;
		case DMT_SINKCAPABILITIES:
			break;
		case DMT_VENDERDEFINED:
			break;
		default:
			break;
		}
	} else {
		chip->send_head |= (cmd & 0xf);
	}
}

static enum tx_state policy_send_data(struct fusb30x_chip *chip)
{
	u8 senddata[40];
	int pos = 0;
	u8 len;

	debug("%s: chip->tx_state=%d\n", __func__, chip->tx_state);
	switch (chip->tx_state) {
	case 0:
		senddata[pos++] = FUSB_TKN_SYNC1;
		senddata[pos++] = FUSB_TKN_SYNC1;
		senddata[pos++] = FUSB_TKN_SYNC1;
		senddata[pos++] = FUSB_TKN_SYNC2;

		len = PD_HEADER_CNT(chip->send_head) << 2;
		senddata[pos++] = FUSB_TKN_PACKSYM | ((len + 2) & 0x1f);

		senddata[pos++] = chip->send_head & 0xff;
		senddata[pos++] = (chip->send_head >> 8) & 0xff;

		memcpy(&senddata[pos], chip->send_load, len);
		pos += len;

		senddata[pos++] = FUSB_TKN_JAMCRC;
		senddata[pos++] = FUSB_TKN_EOP;
		senddata[pos++] = FUSB_TKN_TXOFF;
		senddata[pos++] = FUSB_TKN_TXON;

		fusb302_i2c_raw_write(FUSB_REG_FIFO, senddata, pos);
		chip->tx_state = tx_busy;
		break;

	default:
		/* wait Tx result */
		break;
	}

	return chip->tx_state;
}

static void fusb_state_unattached(struct fusb30x_chip *chip, int evt)
{
	chip->is_cc_connected = 0;
	if ((evt & EVENT_CC) && chip->cc_state) {
		if (chip->cc_state & 0x04) {
			set_state(chip, attach_wait_sink);
			debug("attach_wait_sink\n");

		} else {
			set_state(chip, attach_wait_source);
			debug("attach_wait_source\n");
		}
		tcpm_get_cc(chip, &chip->cc1, &chip->cc2);
		chip->debounce_cnt = 0;
	}
}

static void set_state_unattached(struct fusb30x_chip *chip)
{
	debug("connection has disconnected\n");
	tcpm_init(chip);
	tcpm_set_rx_enable(chip, 0);
	chip->conn_state = unattached;
	tcpm_set_cc(chip, FUSB_MODE_DRP);

	if (gpio_is_valid(chip->gpio_discharge.gpio)) {
		gpio_set_value(chip->gpio_discharge.gpio, 1);
		udelay(1000 * 1000);
		gpio_set_value(chip->gpio_discharge.gpio, 0);
	}
}

static void fusb_state_attach_wait_sink(struct fusb30x_chip *chip)
{
	int cc1, cc2, count = 10;

	while (count--) {
		tcpm_get_cc(chip, &cc1, &cc2);

		if ((chip->cc1 == cc1) && (chip->cc2 == cc2)) {
			chip->debounce_cnt++;
		} else {
			chip->cc1 = cc1;
			chip->cc2 = cc2;
			chip->debounce_cnt = 0;
		}

		udelay(1000 * 2);
		if (chip->debounce_cnt > N_DEBOUNCE_CNT) {
			if ((chip->cc1 != chip->cc2) &&
			    ((!chip->cc1) || (!chip->cc2))) {
				set_state(chip, attached_sink);
				debug("%s attached_sink\n", __func__);

			} else {
				set_state(chip, disabled);
				debug("%s unattached_sink\n", __func__);
			}
			return;
		}
	}
}

static void fusb_state_attached_sink(struct fusb30x_chip *chip)
{
	chip->is_cc_connected = 1;
	if (chip->cc_state & 0x01)
		chip->cc_polarity = 0;
	else
		chip->cc_polarity = 1;

	chip->power_role = 0;
	chip->data_role = 0;
	chip->hardrst_count = 0;
	set_state(chip, policy_snk_startup);
	printf("CC connected in %d as UFP\n", chip->cc_polarity);
}

static void fusb_state_snk_startup(struct fusb30x_chip *chip)
{
	chip->is_pd_connected = 0;
	chip->msg_id = 0;
	chip->vdm_state = 0;
	chip->vdm_substate = 0;
	chip->vdm_send_state = 0;
	chip->val_tmp = 0;
	chip->pos_power = 0;

	memset(chip->partner_cap, 0, sizeof(chip->partner_cap));

	tcpm_set_msg_header(chip);
	tcpm_set_polarity(chip, chip->cc_polarity);
	tcpm_set_rx_enable(chip, 1);
	set_state(chip, policy_snk_discovery);
}

static void fusb_state_snk_discovery(struct fusb30x_chip *chip)
{
	set_state(chip, policy_snk_wait_caps);
}

static void fusb_state_snk_wait_caps(struct fusb30x_chip *chip, int evt)
{
	if (evt & EVENT_RX) {
		if (PD_HEADER_CNT(chip->rec_head) &&
		    PD_HEADER_TYPE(chip->rec_head) == DMT_SOURCECAPABILITIES) {
			set_state(chip, policy_snk_evaluate_caps);
		}
	}
}

static void fusb_set_pos_power(struct fusb30x_chip *chip, int max_vol,
			       int max_cur)
{
	int i;
	int pos_find;
	int tmp;

	pos_find = 0;
	for (i = PD_HEADER_CNT(chip->rec_head) - 1; i >= 0; i--) {
		switch (CAP_POWER_TYPE(chip->rec_load[i])) {
		case 0:
			/* Fixed Supply */
			if ((CAP_FPDO_VOLTAGE(chip->rec_load[i]) * 50) <=
			    max_vol &&
			    (CAP_FPDO_CURRENT(chip->rec_load[i]) * 10) <=
			    max_cur) {
				chip->pos_power = i + 1;
				tmp = CAP_FPDO_VOLTAGE(chip->rec_load[i]);
				chip->pd_output_vol = tmp * 50;
				tmp = CAP_FPDO_CURRENT(chip->rec_load[i]);
				chip->pd_output_cur = tmp * 10;
				pos_find = 1;
			}
			break;
		case 1:
			/* Battery */
			if ((CAP_VPDO_VOLTAGE(chip->rec_load[i]) * 50) <=
			    max_vol &&
			    (CAP_VPDO_CURRENT(chip->rec_load[i]) * 10) <=
			    max_cur) {
				chip->pos_power = i + 1;
				tmp = CAP_VPDO_VOLTAGE(chip->rec_load[i]);
				chip->pd_output_vol = tmp * 50;
				tmp = CAP_VPDO_CURRENT(chip->rec_load[i]);
				chip->pd_output_cur = tmp * 10;
				pos_find = 1;
			}
			break;
		default:
			/* not meet battery caps */
			break;
		}
		if (pos_find)
			break;
	}
}

static int fusb302_set_pos_power_by_charge_ic(struct fusb30x_chip *chip)
{
	int max_vol = CHARGE_INPUT_DEFAULT_VOL, max_cur = CHARGE_INPUT_DEFAULT_CUR;
	const void *blob;
	int node;

	blob = gd->fdt_blob;
	node = fdt_node_offset_by_compatible(blob, 0, "rockchip,uboot-charge");
	if (node < 0) {
		debug("fusb302: can't find dts node for rockchip,uboot-charge\n");
		return -ENODEV;
	}

	max_vol = fdtdec_get_int(blob, node, "max-input-voltage", 0);
	max_cur = fdtdec_get_int(blob, node, "max-input-current", 0);

	if (max_vol > 0 && max_cur > 0)
		fusb_set_pos_power(chip, max_vol, max_cur);

	debug("charge ic max_vol = %dmv max_cur = %dma\n", max_vol, max_cur);
	return 0;
}

static void fusb_state_snk_evaluate_caps(struct fusb30x_chip *chip, int evt)
{
	u32 tmp;

	chip->hardrst_count = 0;
	chip->pos_power = 0;

	for (tmp = 0; tmp < PD_HEADER_CNT(chip->rec_head); tmp++) {
		switch (CAP_POWER_TYPE(chip->rec_load[tmp])) {
		case 0:
			/* Fixed Supply */
			if (CAP_FPDO_VOLTAGE(chip->rec_load[tmp]) <= 100)
				chip->pos_power = tmp + 1;
			break;
		case 1:
			/* Battery */
			if (CAP_VPDO_VOLTAGE(chip->rec_load[tmp]) <= 100)
				chip->pos_power = tmp + 1;
			break;
		default:
			/* not meet battery caps */
			break;
		}
	}

	fusb302_set_pos_power_by_charge_ic(chip);

	debug("chip->pos_power = %d, chip->pd_output_vol=%d  chip->pd_output_cur=%d\n",
				chip->pos_power, chip->pd_output_vol, chip->pd_output_cur);
	if ((!chip->pos_power) || (chip->pos_power > 7)) {
		chip->pos_power = 0;
		set_state(chip, policy_snk_wait_caps);
	} else {
		set_state(chip, policy_snk_select_cap);
	}
}

static void fusb_state_snk_select_cap(struct fusb30x_chip *chip, int evt)
{
	u32 tmp;

	debug("%s chip->sub_state=%d %d\n", __func__, chip->sub_state,
				PD_HEADER_TYPE(chip->rec_head));
	switch (chip->sub_state) {
	case 0:
		set_mesg(chip, DMT_REQUEST, DATAMESSAGE);
		chip->sub_state = 1;
		chip->tx_state = tx_idle;
		/* without break */
	case 1:
		tmp = policy_send_data(chip);
		if (tmp == tx_success) {
			chip->sub_state++;
		} else if (tmp == tx_failed) {
			set_state(chip, policy_snk_discovery);
			break;
		}

	default:
		if (evt & EVENT_RX) {
			if (!PD_HEADER_CNT(chip->rec_head)) {
				switch (PD_HEADER_TYPE(chip->rec_head)) {
				case CMT_ACCEPT:
					set_state(chip,
						  policy_snk_transition_sink);
					break;
				case CMT_WAIT:
				case CMT_REJECT:
					if (chip->is_pd_connected) {
						printf("PD connected as UFP, fetching 5V\n");
						set_state(chip,
							  policy_snk_ready);
					} else {
						set_state(chip,
							policy_snk_wait_caps);
						/*
						 * make sure don't send
						 * hard reset to prevent
						 * infinite loop
						 */
						chip->hardrst_count =
							N_HARDRESET_COUNT + 1;
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	}
}

static void fusb_state_snk_transition_sink(struct fusb30x_chip *chip, int evt)
{
	if (evt & EVENT_RX) {
		if ((!PD_HEADER_CNT(chip->rec_head)) &&
		    (PD_HEADER_TYPE(chip->rec_head) == CMT_PS_RDY)) {
			chip->is_pd_connected = 1;
			debug("PD connected as UFP, fetching 5V\n");
			set_state(chip, policy_snk_ready);
		} else if ((PD_HEADER_CNT(chip->rec_head)) &&
			   (PD_HEADER_TYPE(chip->rec_head) ==
			    DMT_SOURCECAPABILITIES)) {
			set_state(chip, policy_snk_evaluate_caps);
		}
	}
}

static void tcpc_alert(struct fusb30x_chip *chip, int *evt)
{
	u8 interrupt = 0, interrupta = 0, interruptb = 0;
	u8 val;

	fusb302_i2c_read(FUSB_REG_INTERRUPT, &interrupt);
	fusb302_i2c_read(FUSB_REG_INTERRUPTA, &interrupta);
	fusb302_i2c_read(FUSB_REG_INTERRUPTB, &interruptb);
	debug("interrupt=0x%x a=0x%x b=0x%x\n", interrupt , interrupta, interruptb);

	if (interrupt & INTERRUPT_BC_LVL) {
		if (chip->is_cc_connected)
			*evt |= EVENT_CC;
	}

	if (interrupt & INTERRUPT_VBUSOK) {
		if (chip->is_cc_connected)
			*evt |= EVENT_CC;
	}

	if (interrupta & INTERRUPTA_TOGDONE) {
		*evt |= EVENT_CC;
		fusb302_i2c_read(FUSB_REG_STATUS1A, &val);
		chip->cc_state = ((u8)val >> 3) & 0x07;

		fusb302_i2c_update_bits(FUSB_REG_CONTROL2,
				   CONTROL2_TOGGLE,
				   0);

		val &= ~(SWITCHES0_PU_EN1 | SWITCHES0_PU_EN2 |
			 SWITCHES0_PDWN1 | SWITCHES0_PDWN2);

		if (chip->cc_state & 0x01)
			val |= SWITCHES0_PU_EN1;
		else
			val |= SWITCHES0_PU_EN2;

		fusb302_i2c_update_bits(FUSB_REG_SWITCHES0,
				   SWITCHES0_PU_EN1 | SWITCHES0_PU_EN2 |
				   SWITCHES0_PDWN1 | SWITCHES0_PDWN2,
				   val);
	}

	if (interruptb & INTERRUPTB_GCRCSENT)
		*evt |= EVENT_RX;

	if (interrupta & INTERRUPTA_TXSENT) {
		*evt |= EVENT_TX;
		fusb302_flush_rx_fifo(chip);
		chip->tx_state = tx_success;
	}

	if (interrupta & INTERRUPTA_HARDRST) {
		fusb302_pd_reset(chip);
		*evt |= EVENT_REC_RESET;
	}

	if (interrupta & INTERRUPTA_RETRYFAIL) {
		*evt |= EVENT_TX;
		chip->tx_state = tx_failed;
	}

	if (interrupta & INTERRUPTA_HARDSENT) {
		chip->tx_state = tx_success;
		*evt |= EVENT_TX;
	}
}

static void state_machine_typec(struct fusb30x_chip *chip)
{
	int evt = 0;
	int cc1, cc2;

	tcpc_alert(chip, &evt);

	if (chip->is_cc_connected) {
		if (evt & EVENT_CC) {
			if ((chip->cc_state & 0x04) &&
			    (chip->conn_state !=
			     policy_snk_transition_default)) {
				if (!tcpm_check_vbus(chip))
					set_state_unattached(chip);
			} else if (chip->conn_state !=
				   policy_src_transition_default) {
				tcpm_get_cc(chip, &cc1, &cc2);
				if (!(chip->cc_state & 0x01))
					cc1 = cc2;
				if (cc1 == TYPEC_CC_VOLT_OPEN)
					set_state_unattached(chip);
			}
		}
	}

	if (evt & EVENT_RX) {
		tcpm_get_message(chip);
		if ((!PD_HEADER_CNT(chip->rec_head)) &&
		    (PD_HEADER_TYPE(chip->rec_head) == CMT_SOFTRESET)) {
			if (chip->power_role)
				set_state(chip, policy_src_send_softrst);
			else
				set_state(chip, policy_snk_send_softrst);
		}
	}

	if (evt & EVENT_TX) {
		if (chip->tx_state == tx_success)
			chip->msg_id++;
	}

	debug("conn_state=%d evt=%d rec_head=%d\n", chip->conn_state, evt, PD_HEADER_TYPE(chip->rec_head));
	switch (chip->conn_state) {
	case disabled:
		debug("%s:disabled\n", __func__);
		break;
	case error_recovery:
		break;
	case unattached:
		fusb_state_unattached(chip, evt);
		if (chip->conn_state != attach_wait_sink)
			break;
	case attach_wait_sink:
		fusb_state_attach_wait_sink(chip);
		if (chip->conn_state != attached_sink)
			break;
	case attached_sink:
		fusb_state_attached_sink(chip);

	/* POWER DELIVERY */
	/* UFP */
	case policy_snk_startup:
		fusb_state_snk_startup(chip);
	case policy_snk_discovery:
		fusb_state_snk_discovery(chip);
	case policy_snk_wait_caps:
		fusb_state_snk_wait_caps(chip, evt);
		if (policy_snk_evaluate_caps != chip->conn_state)
			break;
	case policy_snk_evaluate_caps:
		fusb_state_snk_evaluate_caps(chip, evt);
	case policy_snk_select_cap:
		fusb_state_snk_select_cap(chip, evt);
		break;
	case policy_snk_transition_sink:
		fusb_state_snk_transition_sink(chip, evt);
		break;
	case policy_snk_transition_default:
		printf("%s policy_snk_transition_default ready\n", __func__);
		break;
	case policy_snk_ready:
		printf("fusb302 sink ready\n");
		break;
	case policy_snk_send_hardrst:
		printf("%s policy_snk_send_hardrst\n", __func__);
		break;
	case policy_snk_send_softrst:
		printf("%s policy_snk_send_softrst\n", __func__);
		break;
	default:
		disable_irq();
	}

	enable_irq();
}

static void cc_interrupt_handler(void)
{
	disable_irq();
	state_machine_typec(&chip);
}

static int fusb302_get_node(const void *blob, int node)
{
	node = fdt_node_offset_by_compatible(blob,
					node, FUSB30X_I2C_DEVICETREE_NAME);
	if (node < 0) {
		printf("Can't find dts node for charger fusb302\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, node)) {
		printf("device fusb302 is disabled\n");
		return -1;
	}

	return node;
}

static int fusb302_parse_dt(const void *blob, int node)
{
	u32 bus;
	int ret = 0;

	chip.uboot_charge = fdtdec_get_int(blob, node, "support-uboot-charge", 0);
	if (chip.uboot_charge != 1)
		return -1;

	ret = fdt_get_i2c_info(blob, node, &bus, &chip.addr);
	if (ret < 0) {
		printf("fg fusb302 get fdt i2c failed\n");
		return ret;
	}

	ret = fusb302_i2c_probe(bus, chip.addr);
	if (ret < 0) {
		printf("fg fusb302 i2c probe failed\n");
		return ret;
	}

	fdtdec_decode_gpio(blob, node, "int-n-gpios", &chip.gpio_cc_int);
	if (gpio_is_valid(chip.gpio_cc_int.gpio)) {
		gpio_pull_updown(chip.gpio_cc_int.gpio, GPIOPullUp);
		chip.gpio_irq = gpio_to_irq(chip.gpio_cc_int.gpio);
		irq_install_handler(chip.gpio_irq, (interrupt_handler_t *)cc_interrupt_handler, NULL);
		irq_set_irq_type(chip.gpio_irq, IRQ_TYPE_LEVEL_LOW);
	}

	fdtdec_decode_gpio(blob, node, "vbus-5v-gpios", &chip.gpio_vbus_5v);
	if (gpio_is_valid(chip.gpio_vbus_5v.gpio)) {
		gpio_direction_output(chip.gpio_vbus_5v.gpio, 0);
		gpio_set_value(chip.gpio_vbus_5v.gpio, 0);
	}

	fdtdec_decode_gpio(blob, node, "discharge-gpios", &chip.gpio_discharge);
	if (gpio_is_valid(chip.gpio_discharge.gpio)) {
		gpio_direction_output(chip.gpio_discharge.gpio, 0);
		gpio_set_value(chip.gpio_discharge.gpio, 0);
	}

	return ret;
}

/*
 * The charge ic get adapter outprt vollage and current.
 */
int get_pd_output_val(int *pd_output_vol, int *pd_output_cur)
{
	if (chip.pd_output_cur && chip.pd_output_vol) {
		*pd_output_vol = chip.pd_output_vol;
		*pd_output_cur = chip.pd_output_cur;
		return 0;
	} else {
		return -1;
	}
}

/*
 * The charge ic to contrl charge path depend on port_num.
 */
int get_pd_port_num(void)
{
	return chip.port_num;
}

static void update_port_num(const void *blob, int node)
{
	chip.port_num = fdtdec_get_int(blob, node, "port-num", -1);
	printf ("fusb302 detect chip.port_num = %d\n", chip.port_num);
}

void typec_discharge(void)
{
	if (gpio_is_valid(chip.gpio_discharge.gpio)) {
		gpio_set_value(chip.gpio_discharge.gpio, 1);
		udelay(1000 * 1000);
		gpio_set_value(chip.gpio_discharge.gpio, 0);
	}
}

int fusb302_init(void)
{
	int node = 0, ret, wait_for_complete;
	struct PD_CAP_INFO *pd_cap_info;

	if (!gd->fdt_blob)
		return -1;

	while (1) {
		wait_for_complete = 1000;
		node = fusb302_get_node(gd->fdt_blob, node);
		if (node < 0)
			return node;
		ret = fusb302_parse_dt(gd->fdt_blob, node);
		if (ret < 0)
			continue;

		tcpm_init(&chip);
		tcpm_set_rx_enable(&chip, 0);
		chip.conn_state = unattached;
		tcpm_set_cc(&chip, FUSB_MODE_DRP);

		pd_cap_info = &chip.pd_cap_info;
		pd_cap_info->dual_role_power = 0;
		pd_cap_info->data_role_swap = 0;
		pd_cap_info->externally_powered = 1;
		pd_cap_info->usb_suspend_support = 0;
		pd_cap_info->usb_communications_cap = 0;
		pd_cap_info->supply_type = 0;
		pd_cap_info->peak_current = 0;

		irq_handler_enable(chip.gpio_irq);
		while (-- wait_for_complete) {
			if (chip.conn_state == policy_snk_ready) {
				disable_irq();
				update_port_num(gd->fdt_blob, node);
				return 0;
			}
			if (chip.conn_state == attach_wait_source)
				break;
			udelay(1000 * 2);
		}
		if (chip.conn_state > attached_sink) {
			update_port_num(gd->fdt_blob, node);
		}
		disable_irq();
	}
	return 0;
}
