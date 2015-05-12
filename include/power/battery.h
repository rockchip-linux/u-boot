/*
 *  Copyright (C) 2012 Samsung Electronics
 *  Lukasz Majewski <l.majewski@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __POWER_BATTERY_H_
#define __POWER_BATTERY_H_

struct battery {
	unsigned int version;
	unsigned int state_of_chrg;
	unsigned int time_to_empty;
	unsigned int capacity;
	unsigned int voltage_uV;

	unsigned int state;
	unsigned int isexistbat;
};

int power_bat_init(unsigned char bus);
int get_power_bat_status(struct battery *batt_status);

//return 1, if power low.
int is_power_low(void);
//return 1, if power extreme low.
int is_power_extreme_low(void);

#endif /* __POWER_BATTERY_H_ */
