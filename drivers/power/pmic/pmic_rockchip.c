/*pmic auto compatible driver on rockchip platform
 * Copyright (C) 2014 RockChip inc
 * Andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <power/rockchip_pmic.h>

int pmic_charger_setting(int current)
{
	enum pmic_id  id = get_rockchip_pmic_id();
	switch (id) {
		case PMIC_ID_ACT8846:
			pmic_act8846_charger_setting(current);
			break;
		case PMIC_ID_RICOH619:
			pmic_ricoh619_charger_setting(current);
			break;
		case PMIC_ID_RK808:
			pmic_rk808_charger_setting(current);
			break;
		default:
			break;
	}
	return 0;
}

int pmic_init(unsigned char  bus)
{
	int ret;
#if defined(CONFIG_POWER_RICOH619)
	ret = pmic_ricoh619_init(bus);
	if (ret >= 0) {
		set_rockchip_pmic_id(PMIC_ID_RICOH619);
		printf("pmic:ricoh619\n");
		return 0;
	}
#endif

#if defined(CONFIG_POWER_ACT8846)
	ret = pmic_act8846_init (bus);
	if (ret >= 0) {
		set_rockchip_pmic_id(PMIC_ID_ACT8846);
		printf("pmic:act8846\n");
		return 0;
	}
#endif

#if defined(CONFIG_POWER_RK808)
	ret = pmic_rk808_init (bus);
	if (ret >= 0) {
		set_rockchip_pmic_id(PMIC_ID_RK808);
		printf("pmic:rk808\n");
		return 0;
	}
#endif
	return ret;
}

void shut_down(void)
{
	enum pmic_id  id = get_rockchip_pmic_id();
	switch (id) {
		case PMIC_ID_ACT8846:
			pmic_act8846_shut_down();
			break;
		case PMIC_ID_RICOH619:
			pmic_ricoh619_shut_down();
			break;
		case PMIC_ID_RK808:
			pmic_rk808_shut_down();
			break;
		default:
			break;
	}
	return 0;
}

