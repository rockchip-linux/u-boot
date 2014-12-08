/*pmic auto compatible driver on rockchip platform
 * Copyright (C) 2014 RockChip inc
 * Andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <power/rockchip_power.h>
#include <power/battery.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

static unsigned char rockchip_pmic_id;
struct fdt_gpio_state gpio_pwr_hold;
static const char * const fg_names[] = {
	"CW201X_FG",
	"RICOH619_FG",
	"RK818_FG",
	"RT5025_FG",
	"RK-ADC-FG",
	"RT5036_FG",
};


static void set_rockchip_pmic_id(unsigned char id)
{
	rockchip_pmic_id = id;
}

unsigned char get_rockchip_pmic_id(void)
{
	return rockchip_pmic_id;
}


int get_power_bat_status(struct battery *battery)
{
	int i;
	struct pmic *p_fg = NULL;
	for (i = 0; i < ARRAY_SIZE(fg_names); i++) {
		p_fg = pmic_get(fg_names[i]);
		if (p_fg)
			break;
	}

	if (p_fg) {
		p_fg->pbat->bat = battery;
		if (p_fg->fg->fg_battery_update)
			p_fg->fg->fg_battery_update(p_fg, p_fg);
	} else {
		printf("no fuel gauge found\n");
		return -ENODEV;
	}

	return 0;
}


/*
return 0: no charger
return 1: charging
*/
int is_charging(void)
{
	int ret;
	struct battery battery;
	memset(&battery,0, sizeof(battery));
	ret = get_power_bat_status(&battery);
	if (ret < 0)
		return 0;
	return battery.state_of_chrg;
}

int pmic_charger_setting(int current)
{
	enum pmic_id  id = get_rockchip_pmic_id();
	switch (id) {
#if defined(CONFIG_POWER_ACT8846)
		case PMIC_ID_ACT8846:
			pmic_act8846_charger_setting(current);
			break;
#endif
#if defined(CONFIG_POWER_RK808)
		case PMIC_ID_RK808:
			pmic_rk808_charger_setting(current);
			break;
#endif
		default:
			break;
	}
	return 0;
}


/*system on thresd*/
int is_power_low(void)
{
	int ret;
	struct battery battery;
	memset(&battery,0, sizeof(battery));
	ret = get_power_bat_status(&battery);
	if (ret < 0)
		return 0;
	return (battery.voltage_uV < CONFIG_SYSTEM_ON_VOL_THRESD) ? 1:0;	
}


/*screen on thresd*/
int is_power_extreme_low(void)
{
	int ret;
	struct battery battery;
	memset(&battery,0, sizeof(battery));
	ret = get_power_bat_status(&battery);
	if (ret < 0)
		return 0;
	return (battery.voltage_uV < CONFIG_SCREEN_ON_VOL_THRESD) ? 1:0;
}


/* for no pmic init */
static void pmic_null_init(void)
{
	int node;

	printf("No pmic detect.\n");

	gpio_pwr_hold.gpio = -1;

	node = fdt_node_offset_by_compatible(gd->fdt_blob, 0, "gpio-poweroff");
	if (node < 0) {
		debug("No detect gpio power off.\n");
	} else {
		fdtdec_decode_gpio(gd->fdt_blob, node, "gpios", &gpio_pwr_hold);
		gpio_pwr_hold.flags = !(gpio_pwr_hold.flags & OF_GPIO_ACTIVE_LOW);
		printf("power hold: bank-%d pin-%d, active level-%d\n",\
			RK_GPIO_BANK(gpio_pwr_hold.gpio), RK_GPIO_PIN(gpio_pwr_hold.gpio), !gpio_pwr_hold.flags);
		gpio_direction_output(gpio_pwr_hold.gpio, !gpio_pwr_hold.flags);
	}

	/********set APLL CLK 600M***********/
#if (defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128))
	rkclk_set_pll_rate_by_id(APLL_ID, 600);
#endif
}

/* for no pmic shut down */
static void pmic_null_shut_down(void)
{
	if (gpio_pwr_hold.gpio != -1) {
		gpio_direction_output(gpio_pwr_hold.gpio, gpio_pwr_hold.flags);
	}
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

#if defined(CONFIG_POWER_RK818)
	ret = pmic_rk818_init (bus);
	if (ret >= 0) {
		set_rockchip_pmic_id(PMIC_ID_RK818);
		printf("pmic:rk818\n");
		return 0;
	}
#endif

#if defined(CONFIG_POWER_RT5025)
	ret = pmic_rt5025_init (bus);
	if (ret >= 0) {
		set_rockchip_pmic_id(PMIC_ID_RT5025);
		printf("pmic:rt5025\n");
		return 0;
	}
#endif

#if defined(CONFIG_POWER_RT5036)
	ret = pmic_rt5036_init (bus);
	if (ret >= 0) {
		set_rockchip_pmic_id(PMIC_ID_RT5036);
		printf("pmic:rt5036\n");
		return 0;
	}
#endif

#if defined(CONFIG_POWER_ACT8931)
	ret = pmic_act8931_init (bus);
	if (ret >= 0) {
		set_rockchip_pmic_id(PMIC_ID_ACT8931);
		printf("pmic:act8931\n");
		return 0;
	}
#endif

	pmic_null_init();


	return ret;
}


int fg_init(unsigned char bus)
{
	int ret;
#if defined(CONFIG_POWER_FG_CW201X)
	ret = fg_cw201x_init(bus);
	if(ret >= 0) {
		printf("fg:cw201x\n");
		return 0;
	}
#endif
#if defined(CONFIG_POWER_FG_ADC)
	ret = adc_battery_init();
	if (ret >= 0) {
		printf("fg:adc-battery\n");
		return 0;
	}

#endif
	return 0;
}

void shut_down(void)
{
	enum pmic_id  id = get_rockchip_pmic_id();
	switch (id) {
#if defined(CONFIG_POWER_ACT8846)
		case PMIC_ID_ACT8846:
			pmic_act8846_shut_down();
			break;
#endif
#if defined(CONFIG_POWER_RICOH619)
		case PMIC_ID_RICOH619:
			pmic_ricoh619_shut_down();
			break;
#endif
#if defined(CONFIG_POWER_RK808)
		case PMIC_ID_RK808:
			pmic_rk808_shut_down();
			break;
#endif
#if defined(CONFIG_POWER_RK818)
		case PMIC_ID_RK818:
			pmic_rk818_shut_down();
			break;
#endif
#if defined(CONFIG_POWER_RT5025)
		case PMIC_ID_RT5025:
			pmic_rt5025_shut_down();
			break;
#endif
#if defined(CONFIG_POWER_RT5036)
		case PMIC_ID_RT5036:
			pmic_rt5036_shut_down();
			break;
#endif
#if defined(CONFIG_POWER_ACT8931)
		case PMIC_ID_ACT8931:
			pmic_act8931_shut_down();
			break;
#endif

		default:
			pmic_null_shut_down();
			break;
	}
}

