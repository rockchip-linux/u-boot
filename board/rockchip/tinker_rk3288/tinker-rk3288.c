/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <netdev.h>

int rk_board_late_init(void)
{
	struct udevice *dev;
	int ret;
	u8 mac[6];

	ret = i2c_get_chip_for_busnum(2, 0x50, 1, &dev);
	if (ret) {
		debug("failed to get eeprom\n");
		return 0;
	}

	ret = dm_i2c_read(dev, 0x0, mac, 6);
	if (ret) {
		debug("failed to read mac\n");
		return 0;
	}

	if (is_valid_ethaddr(mac))
		eth_setenv_enetaddr("ethaddr", mac);

	return 0;
}
