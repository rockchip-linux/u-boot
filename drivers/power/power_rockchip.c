/*
 * pmic auto compatible driver on rockchip platform
 * Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <power/rockchip_power.h>
#include <power/battery.h>
#include <asm/arch/rkplat.h>
#include <fastboot.h>

DECLARE_GLOBAL_DATA_PTR;

struct regulator_init_reg_name regulator_init_pmic_matches[MAX_REGULATOR_NUM];

#if defined(CONFIG_POWER_RK818)
extern void pmic_rk818_power_init(void);
extern void pmic_rk818_power_on(void);
extern void pmic_rk818_power_off(void);
#endif

#if defined(CONFIG_POWER_FG_ADC)
extern int adc_battery_init(void);
#endif

extern int low_power_level;

static unsigned char rockchip_pmic_id;
struct fdt_gpio_state gpio_pwr_hold;
static const char * const fg_names[] = {
	"CW201X_FG",
	"RICOH619_FG",
	"RK818_FG",
	"RK816_FG",
	"RT5025_FG",
	"RK-ADC-FG",
	"RT5036_FG",
	"EC_FG",
};

static const char * const charger_names[] = {
	"BQ25700_CHARGER",
};

/* rockchip first i2c node as parent for pmic */
int g_i2c_node = 0;
#define COMPAT_ROCKCHIP_I2C	"rockchip,rk30-i2c"

__maybe_unused static void set_rockchip_pmic_id(unsigned char id)
{
	rockchip_pmic_id = id;
}

unsigned char get_rockchip_pmic_id(void)
{
	return rockchip_pmic_id;
}

void charge_led_enable(int enable)
{
	const void *blob;
	struct fdt_gpio_state led1_enable_gpio, led2_enable_gpio;
	int node, subnode1, subnode2;

	blob = gd->fdt_blob;
	node = fdt_node_offset_by_compatible(blob, 0, "gpio-leds");
	if (node < 0) {
		debug("can't find dts node for gpio-leds\n");
		return;
	}

	subnode1 = fdt_subnode_offset(blob, node, "led@1");
	if (subnode1 < 0) {
		debug("can't find dts node for led@1\n");
	} else {
		fdtdec_decode_gpio(blob, subnode1, "gpios", &led1_enable_gpio);
		if (gpio_is_valid(led1_enable_gpio.gpio)) {
			printf("charge led1 %d\n", enable);
			gpio_direction_output(led1_enable_gpio.gpio, enable);
		}
	}

	subnode2 = fdt_subnode_offset(blob, node, "led@2");
	if (subnode2 < 0) {
		debug("can't find dts node for led@1\n");
	} else {
		fdtdec_decode_gpio(blob, subnode2, "gpios", &led2_enable_gpio);
		if (gpio_is_valid(led2_enable_gpio.gpio)) {
			printf("charge led2 %d\n", enable);
			gpio_direction_output(led2_enable_gpio.gpio, enable);
		}
	}
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

int get_power_charger_status(struct power_chrg *chrg)
{
	int i;
	struct pmic *p_chrg = NULL;
	for (i = 0; i < ARRAY_SIZE(charger_names); i++) {
		p_chrg = pmic_get(charger_names[i]);
		if (p_chrg)
			break;
	}

	if (p_chrg) {
		if (p_chrg->chrg->chrg_type)
			p_chrg->chrg->chrg_type(p_chrg);
			chrg->state_of_charger =
				p_chrg->chrg->state_of_charger;
	} else {
		return -ENODEV;
	}

	return 0;
}


/*
return 0: bat exist
return 1: bat no exit
*/
int is_exist_battery(void)
{
	int ret;
	struct battery battery;
	memset(&battery,0, sizeof(battery));
	ret = get_power_bat_status(&battery);
	if (ret < 0)
		return 0;
	return battery.isexistbat;
}

/*
return 0: no charger
return 1: charging
*/
int is_charging(void)
{
	int ret;
	struct battery battery;
	struct power_chrg chrg;

	memset(&battery,0, sizeof(battery));
	memset(&chrg, 0, sizeof(chrg));

	ret = get_power_charger_status(&chrg);

	if (!ret && chrg.state_of_charger)
		return chrg.state_of_charger;

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
	memset(&battery, 0, sizeof(battery));
	ret = get_power_bat_status(&battery);
	if (ret < 0)
		return 0;

	if (battery.capacity < low_power_level)
		return 1;

	return (battery.voltage_uV < CONFIG_SYSTEM_ON_VOL_THRESD) ? 1 : 0;
}


/*screen on thresd*/
int is_power_extreme_low(void)
{
	int ret;
	struct battery battery;
	memset(&battery, 0, sizeof(battery));
	ret = get_power_bat_status(&battery);
	if (ret < 0)
		return 0;
	return (battery.voltage_uV < CONFIG_SCREEN_ON_VOL_THRESD) ? 1 : 0;
}


/* for no pmic init */
static void pmic_null_init(void)
{
	int node;

	printf("No pmic detect.\n");
	set_rockchip_pmic_id(PMIC_ID_UNKNOW);
	gpio_pwr_hold.gpio = -1;

	if (gd->fdt_blob != NULL) {
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
	int ret = -1;
	int i;
	char *pmic_name;

	for (i = 0; i < MAX_DCDC_NUM; i++)
		regulator_init_pmic_matches[i].name = "NULL";

	/* detect first i2c node as parent for pmic detect */
	g_i2c_node = 0;
	if (gd->fdt_blob) {
		g_i2c_node = fdt_node_offset_by_compatible(gd->fdt_blob, 0, COMPAT_ROCKCHIP_I2C);
		if (g_i2c_node < 0)
			g_i2c_node = 0;
	}

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

	ret = pmic_rk816_init(bus);
	if (ret >= 0) {
		pmic_name = pmic_get_rk8xx_id(bus);
		if (!strcmp(pmic_name, "rk816"))
			set_rockchip_pmic_id(PMIC_ID_RK816);
		else if (!strcmp(pmic_name, "rk805"))
			set_rockchip_pmic_id(PMIC_ID_RK805);

		printf("pmic:%s\n", pmic_name);
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
#if defined(CONFIG_POWER_FG_CW201X)
	if (fg_cw201x_init(bus) >= 0) {
		printf("fg:cw201x\n");
		return 0;
	}
#endif
#if defined(CONFIG_POWER_FG_ADC)
	if (adc_battery_init() >= 0) {
		printf("fg:adc-battery\n");
		return 0;
	}

#endif
#if defined(CONFIG_BATTERY_EC)
	if (ec_battery_init() >= 0) {
		printf("fg:ec-battery\n");
		return 0;
	}
#endif
	return 0;
}

void plat_charger_init(void)
{
#if defined(CONFIG_UBOOT_CHARGE) && defined(CONFIG_POWER_FUSB302)
	get_exit_uboot_charge_level();
	if (!board_fbt_exit_uboot_charge())
		fusb302_init();
#endif
#if defined(CONFIG_CHARGER_BQ25700)
	charger_bq25700_init();
#endif
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
		case PMIC_ID_RK816:
		case PMIC_ID_RK805:
			pmic_rk816_shut_down();
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

// shutdown no use ldo
void power_pmic_init(void){
	
#if defined(CONFIG_POWER_RK818)
	pmic_rk818_power_init();
#endif
}

// by wakeup open ldo
void power_on_pmic(void){	
#if defined(CONFIG_POWER_RK818)
	pmic_rk818_power_on();
#endif
}


// by wakeup close ldo
void power_off_pmic(void){
#if defined(CONFIG_POWER_RK818)
	pmic_rk818_power_off();
#endif		
}


