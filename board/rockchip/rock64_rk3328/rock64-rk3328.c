/*
 * (C) Copyright 2017 PINE64
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#define ROCK64_USB_POWER_GPIO 2

int rk_board_late_init(void)
{
	gpio_request(ROCK64_USB_POWER_GPIO, "usb_power");
	gpio_direction_output(ROCK64_USB_POWER_GPIO, 0);
	gpio_free(ROCK64_USB_POWER_GPIO);
	return 0;
}
