// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * maxim-max96745.c  --  I2C register interface access for max96745 serdes chip
 *
 * Copyright (c) 2023-2028 Rockchip Electronics Co. Ltd.
 *
 * Author: luowei <lw@rock-chips.com>
 */
#include <serdes-display-core.h>
#include "maxim-max96745.h"

static bool max96745_bridge_link_locked(struct serdes *serdes)
{
	int ret;
	unsigned int value;

	ret = serdes_reg_read(serdes, 0x002a, &value);
	if (ret < 0)
		return false;

	if (!FIELD_GET(LINK_LOCKED, value))
		return false;

	return true;
}

static bool max96745_bridge_detect(struct serdes *serdes)
{
	return max96745_bridge_link_locked(serdes);
}

static int max96745_bridge_enable(struct serdes *serdes)
{
	struct drm_display_mode *mode =
		 &serdes->serdes_bridge->bridge->state->conn_state.mode;
	u8 cxtp, tx_rate;
	int ret;
	unsigned int value;

	ret = serdes_reg_read(serdes, 0x0011, &value);
	if (ret < 0)
		return ret;

	cxtp = FIELD_GET(CXTP_A, value);

	ret = serdes_reg_read(serdes, 0x0028, &value);
	if (ret < 0)
		return ret;

	tx_rate = FIELD_GET(TX_RATE, value);

	if (!cxtp && mode->clock > 95000 && tx_rate == 1) {
		ret = serdes_set_bits(serdes, 0x0028, TX_RATE,
				      FIELD_PREP(TX_RATE, 2));
		if (ret < 0)
			return ret;

		ret = serdes_set_bits(serdes, 0x0029, RESET_ONESHOT,
				      FIELD_PREP(RESET_ONESHOT, 1));
		if (ret < 0)
			return ret;

		if (readx_poll_timeout(max96745_bridge_link_locked, serdes, ret,
				       ret, 200000))
			printf("%s: GMSL link not locked\n", __func__);
	}

	return ret;
}

static int max96745_bridge_disable(struct serdes *serdes)
{
	u8 cxtp, tx_rate;
	int ret;
	unsigned int value;

	ret = serdes_reg_read(serdes, 0x0011, &value);
	if (ret < 0)
		return ret;

	cxtp = FIELD_GET(CXTP_A, value);

	ret = serdes_reg_read(serdes, 0x0028, &value);
	if (ret < 0)
		return ret;

	tx_rate = FIELD_GET(TX_RATE, value);

	if (!cxtp && tx_rate == 2) {
		ret = serdes_set_bits(serdes, 0x0028, TX_RATE,
				      FIELD_PREP(TX_RATE, 1));
		if (ret < 0)
			return ret;

		ret = serdes_set_bits(serdes, 0x0029, RESET_ONESHOT,
				      FIELD_PREP(RESET_ONESHOT, 1));
		if (ret < 0)
			return ret;

		if (readx_poll_timeout(max96745_bridge_link_locked, serdes, ret,
				       ret, 200000))
			printf("%s: GMSL link not locked\n", __func__);
	}

	return ret;
}

struct serdes_chip_bridge_ops max96745_bridge_ops = {
	.detect = max96745_bridge_detect,
	.enable = max96745_bridge_enable,
	.disable = max96745_bridge_disable,
};

struct serdes_chip_data serdes_max96745_data = {
	.name		= "max96745",
	.serdes_type	= TYPE_SER,
	.serdes_id	= MAXIM_ID_MAX96745,
	.bridge_ops	= &max96745_bridge_ops,
};
EXPORT_SYMBOL_GPL(serdes_max96745_data);
