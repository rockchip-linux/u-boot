/*
 *  Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *  zyw < zyw@rock-chips.com >
 *  for battery driver sample
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/battery.h>

#include <errno.h>


/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/
int get_power_bat_status(struct battery *batt_status)
{

    batt_status->voltage_uV = 3800000;
    batt_status->capacity = 50;
    batt_status->state_of_chrg = 2;
    
	return 0;
}
