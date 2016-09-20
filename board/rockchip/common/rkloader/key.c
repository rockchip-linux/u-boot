/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <power/rockchip_power.h>

#include "../config.h"
#include "key.h"

DECLARE_GLOBAL_DATA_PTR;

#define SARADC_BASE             	RKIO_SARADC_BASE

#define read_XDATA(address) 		(*((uint16 volatile*)(unsigned long)(address)))
#define read_XDATA32(address)		(*((uint32 volatile*)(unsigned long)(address)))
#define write_XDATA(address, value) 	(*((uint16 volatile*)(unsigned long)(address)) = value)
#define write_XDATA32(address, value)	(*((uint32 volatile*)(unsigned long)(address)) = value)


__maybe_unused static gpio_conf		charge_state_gpio;
__maybe_unused static gpio_conf		power_hold_gpio;

__maybe_unused static key_config	key_rockusb;
__maybe_unused static key_config	key_recovery;
__maybe_unused static key_config	key_fastboot;
__maybe_unused static key_config	key_power;


#ifdef CONFIG_OF_LIBFDT
__maybe_unused static struct fdt_gpio_state	gPowerKey;
__maybe_unused static int rkkey_parse_powerkey_dt(const void *blob, struct fdt_gpio_state *powerkey_gpio);
#endif


/*
    固定GPIOA_0口作为烧写检测口,系统部分不能使用该口
*/
static int GetPortState(key_config *key)
{
	uint32 tt;
	uint32 hCnt = 0;
	adc_conf* adc = &key->key.adc;
	int_conf* ioint = &key->key.ioint;

	if (key->type == KEY_AD) {
		/* TODO: clk没有配置 */
#if defined(CONFIG_RKCHIP_RK322XH)
		rkclk_set_saradc_clk();
#endif
		for (tt = 0; tt < 10; tt++) {
			/* read special gpio port value. */
			uint32 value;
			uint32 timeout = 0;

			write_XDATA32(adc->ctrl, 0);
			DRVDelayUs(5);
			write_XDATA32(adc->ctrl, 0x0028 | (adc->index));
			DRVDelayUs(5);
			do {
				value = read_XDATA32(adc->ctrl);
				timeout++;
			} while((value & 0x40) == 0);
			value = read_XDATA32(adc->data);
			//printf("adc key = %d\n",value);
			//DRVDelayUs(1000);
			if ((value <= adc->keyValueHigh) && (value >= adc->keyValueLow))
				hCnt++;
		}
		write_XDATA32(adc->ctrl, 0);
		return (hCnt > 8);
	} else if (key->type == KEY_INT) {
		int state;

		state = gpio_get_value(key->key.ioint.gpio);
		if (ioint->pressed_state == 0) { // active low
			return !state;
		} else {
			return state;
		}
	}

	return 0;
}


int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot)
{
	printf("checkKey\n");

	*boot_rockusb = 0;
	*boot_recovery = 0;
	*boot_fastboot = 0;

	if(GetPortState(&key_rockusb)) {
		*boot_rockusb = 1;
	}
	if(GetPortState(&key_recovery)) {
		*boot_recovery = 1;
	}
	if(GetPortState(&key_fastboot)) {
		*boot_fastboot = 1;
	}

	return 0;
}


__maybe_unused static void RockusbKeyInit(void)
{
#if defined(CONFIG_RKCHIP_RK3036)
	key_rockusb.type = KEY_INT;
	key_rockusb.key.ioint.name = "rockusb_key";
	key_rockusb.key.ioint.gpio = (GPIO_BANK2 | GPIO_B0);
	key_rockusb.key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
	key_rockusb.key.ioint.pressed_state = 0;
	key_rockusb.key.ioint.press_time = 0;
#elif defined(CONFIG_RKCHIP_RK322X)
	key_rockusb.type = KEY_INT;
	key_rockusb.key.ioint.name = "rockusb_key";
	key_rockusb.key.ioint.gpio = (GPIO_BANK3 | GPIO_D1);
	key_rockusb.key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
	key_rockusb.key.ioint.pressed_state = 0;
	key_rockusb.key.ioint.press_time = 0;
#else
	key_rockusb.type = KEY_AD;
	key_rockusb.key.adc.index = KEY_ADC_CN;
	key_rockusb.key.adc.keyValueLow = 0;
	key_rockusb.key.adc.keyValueHigh = 30;
	key_rockusb.key.adc.data = SARADC_BASE + 0;
	key_rockusb.key.adc.stas = SARADC_BASE + 4;
	key_rockusb.key.adc.ctrl = SARADC_BASE + 8;
#endif
}


__maybe_unused static void RecoveryKeyInit(void)
{
	key_recovery.type = KEY_AD;
	key_recovery.key.adc.index = KEY_ADC_CN;
	key_recovery.key.adc.keyValueLow = 0;
	key_recovery.key.adc.keyValueHigh = 30;
	key_recovery.key.adc.data = SARADC_BASE + 0;
	key_recovery.key.adc.stas = SARADC_BASE + 4;
	key_recovery.key.adc.ctrl = SARADC_BASE + 8;
}


__maybe_unused static void FastbootKeyInit(void)
{
	key_fastboot.type = KEY_AD;
	key_fastboot.key.adc.index = KEY_ADC_CN;
	key_fastboot.key.adc.keyValueLow = 160;
	key_fastboot.key.adc.keyValueHigh = 180;
	key_fastboot.key.adc.data = SARADC_BASE + 0;
	key_fastboot.key.adc.stas = SARADC_BASE + 4;
	key_fastboot.key.adc.ctrl = SARADC_BASE + 8;
}


__maybe_unused static void PowerKeyInit(void)
{
#ifdef CONFIG_OF_LIBFDT
	memset(&gPowerKey, 0, sizeof(struct fdt_gpio_state));
	if (gd->fdt_blob != NULL)
		rkkey_parse_powerkey_dt(gd->fdt_blob, &gPowerKey);
#endif

	//power_hold_gpio.name

	key_power.type = KEY_INT;
	key_power.key.ioint.name = "power_key";
#ifdef CONFIG_OF_LIBFDT
	key_power.key.ioint.gpio = gPowerKey.gpio;
#else
	key_power.key.ioint.gpio = INVALID_GPIO;
#endif
	key_power.key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
	key_power.key.ioint.pressed_state = 0;
	key_power.key.ioint.press_time = 0;
}


__maybe_unused static void PowerHoldGpioInit(void)
{

}

__maybe_unused static void ChargeStateGpioInit(void)
{
	charge_state_gpio.name = "charge_state";
	charge_state_gpio.flags = 0;
	charge_state_gpio.gpio = (GPIO_BANK0 | GPIO_B0);
	gpio_direction_input(charge_state_gpio.gpio);
}

#ifdef CONFIG_RK_PWM_REMOTE

extern int g_ir_keycode;
extern int remotectl_do_something(void);
extern void remotectlInitInDriver(void);


int RemotectlInit(void)
{
	remotectlInitInDriver();
	//install the irq hander for PWM irq.
	irq_install_handler(RK_PWM_REMOTE_IRQ, (interrupt_handler_t *)remotectl_do_something, NULL);
	irq_handler_enable(RK_PWM_REMOTE_IRQ);

	return 0;
}

int RemotectlDeInit(void)
{
	//uninstall and disable PWM irq.
	irq_uninstall_handler(RK_PWM_REMOTE_IRQ);
	irq_handler_disable(RK_PWM_REMOTE_IRQ);

	return 0;
}
#endif /* CONFIG_RK_PWM_REMOTE */


int rkkey_power_state(void)
{
#if defined(CONFIG_RK_POWER) && defined(CONFIG_POWER_RICOH619) && defined(CONFIG_POWER_RK818)
	if (get_rockchip_pmic_id() == PMIC_ID_RICOH619)
		return ricoh619_poll_pwr_key_sta();
	else if (get_rockchip_pmic_id() == PMIC_ID_RK816)
		return pmic_rk816_poll_pwrkey_stat();
	else
#endif
		return GetPortState(&key_power);
}

#ifdef CONFIG_OF_LIBFDT
static int rkkey_parse_powerkey_dt(const void *blob, struct fdt_gpio_state *powerkey_gpio)
{
	int powerkey_node;

#if 0
	powerkey_node = fdt_path_offset(blob, "/adc/key/power-key");
#else
	powerkey_node = fdt_node_offset_by_compatible(blob, -1, "rockchip,key");
	if (powerkey_node < 0) {
		printf("no key node\n");
		return -1;
	}
	powerkey_node = fdt_subnode_offset(blob, powerkey_node, "power-key");
#endif
	if (powerkey_node < 0) {
		printf("no power key node\n");
		return -1;
	}

	fdtdec_decode_gpio(blob, powerkey_node, "gpios", powerkey_gpio);
	powerkey_gpio->flags = !(powerkey_gpio->flags & OF_GPIO_ACTIVE_LOW);

	printf("power key: bank-%d pin-%d\n", RK_GPIO_BANK(powerkey_gpio->gpio), RK_GPIO_PIN(powerkey_gpio->gpio));
	return 0;
}

struct fdt_gpio_state *rkkey_get_powerkey(void)
{
	if (gPowerKey.gpio != FDT_GPIO_NONE) {
		return &gPowerKey;
	}

	return NULL;
}
#endif /* CONFIG_OF_LIBFDT */


void key_init(void)
{
	memset(&key_rockusb, 0, sizeof(key_config));
	memset(&key_recovery, 0, sizeof(key_config));
	memset(&key_fastboot, 0, sizeof(key_config));
	memset(&key_power, 0, sizeof(key_config));

	memset(&power_hold_gpio, 0, sizeof(gpio_conf));
	memset(&charge_state_gpio, 0, sizeof(gpio_conf));

	RockusbKeyInit();
#if !(defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK322X))
	FastbootKeyInit();
	RecoveryKeyInit();
	PowerKeyInit();

	ChargeStateGpioInit();
	PowerHoldGpioInit();
#endif
}


void powerOn(void)
{
	if (power_hold_gpio.name != NULL) {
		gpio_direction_output(power_hold_gpio.gpio, power_hold_gpio.flags);
	}
}

void powerOff(void)
{
	if (power_hold_gpio.name != NULL) {
		gpio_direction_output(power_hold_gpio.gpio, !power_hold_gpio.flags);
	}
}

