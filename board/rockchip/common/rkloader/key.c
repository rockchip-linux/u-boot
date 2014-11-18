/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * Configuation settings for the rk3xxx chip platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <power/rockchip_power.h>

#include "../config.h"
#include "key.h"

DECLARE_GLOBAL_DATA_PTR;

#define SARADC_BASE             RKIO_SARADC_PHYS

#define read_XDATA(address) 		(*((uint16 volatile*)(address)))
#define read_XDATA32(address)		(*((uint32 volatile*)(address)))
#define write_XDATA(address, value) 	(*((uint16 volatile*)(address)) = value)
#define write_XDATA32(address, value)	(*((uint32 volatile*)(address)) = value)

int gpio_reg[]={
#if defined(CONFIG_RKCHIP_RK3288)
	RKIO_GPIO0_PHYS,
	RKIO_GPIO1_PHYS,
	RKIO_GPIO2_PHYS,
	RKIO_GPIO3_PHYS,
	RKIO_GPIO4_PHYS,
	RKIO_GPIO5_PHYS,
	RKIO_GPIO6_PHYS,
	RKIO_GPIO7_PHYS,
	RKIO_GPIO8_PHYS
#elif defined(CONFIG_RKCHIP_RK3036)
	RKIO_GPIO0_PHYS,
	RKIO_GPIO1_PHYS,
	RKIO_GPIO2_PHYS
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	RKIO_GPIO0_PHYS,
	RKIO_GPIO1_PHYS,
	RKIO_GPIO2_PHYS,
	RKIO_GPIO3_PHYS,
#else
	#error "PLS config rk chip for key."
#endif
};

extern void DRVDelayUs(uint32 us);

static gpio_conf	charge_state_gpio;
static gpio_conf	power_hold_gpio;

static key_config	key_rockusb;
static key_config	key_recovery;
static key_config	key_fastboot;
static key_config	key_power;
static key_config	key_remote;

#ifdef CONFIG_OF_LIBFDT
static struct fdt_gpio_state	gPowerKey;
static int rkkey_parse_powerkey_dt(const void *blob, struct fdt_gpio_state *powerkey_gpio);
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

	if(key->type == KEY_AD)
	{
		// TODO: clk没有配置
		for(tt = 0; tt < 10; tt++)
		{
			// read special gpio port value.
			uint32 value;
			uint32 timeout = 0;

			write_XDATA32( adc->ctrl, 0);
			DRVDelayUs(1);
			write_XDATA32( adc->ctrl, 0x0028|(adc->index));
			DRVDelayUs(1);
			do {
				value = read_XDATA32(adc->ctrl);
				timeout++;
			} while((value&0x40) == 0);
			value = read_XDATA32(adc->data);
			//printf("adc key = %d\n",value);
			//DRVDelayUs(1000);
			if( value<=adc->keyValueHigh && value>=adc->keyValueLow)
				hCnt++;
		}
		write_XDATA32( adc->ctrl, 0);
		return (hCnt>8);
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

	if(GetPortState(&key_rockusb))
	{
		*boot_rockusb = 1;
		//printf("rockusb key is pressed\n");
	}
	if(GetPortState(&key_recovery))
	{
		*boot_recovery = 1;
		//printf("recovery key is pressed\n");
	}
	if(GetPortState(&key_fastboot))
	{
		*boot_fastboot = 1;
		//printf("fastboot key is pressed\n");
	}

#if defined(CONFIG_RK_PWM_REMOTE)
#if 0 /* 0: no need delay */
	int i, ir_keycode = 0;
	extern int g_ir_keycode;

	for(i=0; i<1000; i++)
	{
		ir_keycode = g_ir_keycode;
		if(ir_keycode != 0) {
			break;
		}
		udelay(1000);
	}
	printf("%s: delay %dus\n", __func__, i*1000);
#endif
#endif

	return 0;
}


static void RockusbKeyInit(key_config *key)
{
#if defined(CONFIG_RKCHIP_RK3036)
	key->type = KEY_INT;
	key->key.ioint.name = "rockusb_key";
	key->key.ioint.gpio = (GPIO_BANK2 | GPIO_B0);
	key->key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
	key->key.ioint.pressed_state = 0;
	key->key.ioint.press_time = 0;
#else
	key->type = KEY_AD;
	key->key.adc.index = KEY_ADC_CN;
	key->key.adc.keyValueLow = 0;
	key->key.adc.keyValueHigh= 30;
	key->key.adc.data = SARADC_BASE;
	key->key.adc.stas = SARADC_BASE+4;
	key->key.adc.ctrl = SARADC_BASE+8;
#endif
}

static void RecoveryKeyInit(key_config *key)
{
#if defined(CONFIG_RKCHIP_RK3036)
	key->type = KEY_NULL;
#else
	key->type = KEY_AD;
	key->key.adc.index = KEY_ADC_CN;
	key->key.adc.keyValueLow = 0;
	key->key.adc.keyValueHigh= 30;
	key->key.adc.data = SARADC_BASE;
	key->key.adc.stas = SARADC_BASE+4;
	key->key.adc.ctrl = SARADC_BASE+8;
#endif
}


static void FastbootKeyInit(key_config *key)
{
#if defined(CONFIG_RKCHIP_RK3036)
	key->type = KEY_NULL;
#else
	key->type = KEY_AD;
	key->key.adc.index = KEY_ADC_CN;
	key->key.adc.keyValueLow = 170;
	key->key.adc.keyValueHigh= 180;
	key->key.adc.data = SARADC_BASE;
	key->key.adc.stas = SARADC_BASE+4;
	key->key.adc.ctrl = SARADC_BASE+8;
#endif
}


static void PowerKeyInit(void)
{
#ifdef CONFIG_OF_LIBFDT
	memset(&gPowerKey, 0, sizeof(struct fdt_gpio_state));
	rkkey_parse_powerkey_dt(gd->fdt_blob, &gPowerKey);
#endif

	//power_hold_gpio.name
#if defined(CONFIG_RKCHIP_RK3036)
	key_power.type = KEY_NULL;
	key_power.key.ioint.name = NULL;
#else
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
#endif
}

#ifdef CONFIG_RK_PWM_REMOTE
extern int g_ir_keycode;
extern int remotectl_do_something(void);
extern void remotectlInitInDriver(void);

extern unsigned int rkclk_get_pwm_clk(uint32 pwm_id);


int RemotectlInit(void)
{
	key_remote.type = KYE_REMOTE;
	key_remote.key.ioint.name = NULL;
	key_remote.key.ioint.gpio = (GPIO_BANK0 | GPIO_D3);
	key_remote.key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
	key_remote.key.ioint.pressed_state = 0;
	key_remote.key.ioint.press_time = 0;

	remotectlInitInDriver();
	rk_iomux_config(RK_PWM3_IOMUX);
	//install the irq hander for PWM irq.
	irq_install_handler(IRQ_PWM, remotectl_do_something, NULL);
	irq_handler_enable(IRQ_PWM);

	return 0;
}

int RemotectlDeInit(void)
{
	//uninstall and disable PWM irq.
	irq_uninstall_handler(IRQ_PWM);
	irq_handler_disable(IRQ_PWM);

	return 0;
}

#endif

int rkkey_power_state(void)
{
#if !defined(CONFIG_RKCHIP_RK3036)
#ifdef CONFIG_POWER_RK
	if (get_rockchip_pmic_id() == PMIC_ID_RICOH619)
		return ricoh619_poll_pwr_key_sta();
	else
#endif
		return GetPortState(&key_power);
#endif
}


#ifdef CONFIG_OF_LIBFDT
static int rkkey_parse_powerkey_dt(const void *blob, struct fdt_gpio_state *powerkey_gpio)
{
	int adc_node, key_node, powerkey_node;

	adc_node = fdt_path_offset(blob, "/adc");
	if (adc_node < 0) {
		printf("no adc node\n");
		return -1;
	}

	key_node = fdt_subnode_offset(blob, adc_node, "key");
	if (key_node < 0) {
		printf("no key node\n");
		return -1;
	}

	powerkey_node = fdt_subnode_offset(blob, key_node, "power-key");
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
#endif

void key_init(void)
{
#if defined(CONFIG_RKCHIP_RK3036)
	RockusbKeyInit(&key_rockusb);
#else
	charge_state_gpio.name = "charge_state";
	charge_state_gpio.flags = 0;
	charge_state_gpio.gpio = (GPIO_BANK0 | GPIO_B0);
	gpio_direction_input(charge_state_gpio.gpio);

	//power_hold_gpio.name

	RockusbKeyInit(&key_rockusb);
	FastbootKeyInit(&key_fastboot);
	RecoveryKeyInit(&key_recovery);
	PowerKeyInit();
#endif
}


void powerOn(void)
{
#if defined(CONFIG_RKCHIP_RK3036)
	/* no power hold */
#else
	if(power_hold_gpio.name != NULL)
		gpio_direction_output(power_hold_gpio.gpio, power_hold_gpio.flags);
#endif
}

void powerOff(void)
{
#if defined(CONFIG_RKCHIP_RK3036)
	/* no power hold */
#else
	if(power_hold_gpio.name != NULL)
		gpio_direction_output(power_hold_gpio.gpio, !power_hold_gpio.flags);
#endif
}

