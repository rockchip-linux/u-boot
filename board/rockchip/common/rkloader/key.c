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

#define SARADC_BASE             RKIO_SARADC_PHYS

#define read_XDATA(address) 		(*((uint16 volatile*)(address)))
#define read_XDATA32(address)		(*((uint32 volatile*)(address)))
#define write_XDATA(address, value) 	(*((uint16 volatile*)(address)) = value)
#define write_XDATA32(address, value)	(*((uint32 volatile*)(address)) = value)

int gpio_reg[]={
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	RKIO_GPIO0_PHYS,
	RKIO_GPIO1_PHYS,
	RKIO_GPIO2_PHYS,
	RKIO_GPIO3_PHYS,
	RKIO_GPIO4_PHYS,
	RKIO_GPIO5_PHYS,
	RKIO_GPIO6_PHYS,
	RKIO_GPIO7_PHYS,
	RKIO_GPIO8_PHYS
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	RKIO_GPIO0_PHYS,
	RKIO_GPIO1_PHYS,
	RKIO_GPIO2_PHYS
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3126) || (CONFIG_RKCHIPTYPE == CONFIG_RK3128)
	RKIO_GPIO0_PHYS,
	RKIO_GPIO1_PHYS,
	RKIO_GPIO2_PHYS,
	RKIO_GPIO3_PHYS,
#else
	#error "PLS check CONFIG_RKCHIPTYPE for key."
#endif
};

extern void DRVDelayUs(uint32 us);

gpio_conf       charge_state_gpio;
gpio_conf       power_hold_gpio;

key_config	key_rockusb;
key_config	key_recovery;
key_config	key_fastboot;
key_config      key_power;
key_config      key_remote;


/*
    固定GPIOA_0口作为烧写检测口,系统部分不能使用该口
*/
int GetPortState(key_config *key)
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
	*boot_rockusb = 0;
	*boot_recovery = 0;
	*boot_fastboot = 0;

	printf("checkKey\n");

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

	return 0;
}


void RockusbKeyInit(key_config *key)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	key->type = KEY_INT;
	key->key.ioint.name = "rockusb_key";
	key->key.ioint.gpio = ((GPIO_BANK2 << RK_GPIO_BANK_OFFSET) | GPIO_B0);
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

void RecoveryKeyInit(key_config *key)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
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


void FastbootKeyInit(key_config *key)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
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


void PowerKeyInit(void)
{
	//power_hold_gpio.name
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	key_power.type = KEY_NULL;
	key_power.key.ioint.name = NULL;
#else
	key_power.type = KEY_INT;
	key_power.key.ioint.name = "power_key";
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	key_power.key.ioint.gpio = ((GPIO_BANK0 << RK_GPIO_BANK_OFFSET) | GPIO_A5);
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3126) || (CONFIG_RKCHIPTYPE == CONFIG_RK3128)
	key_power.key.ioint.gpio = ((GPIO_BANK1 << RK_GPIO_BANK_OFFSET) | GPIO_A2);
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
	int i = 0;
	key_remote.type = KYE_REMOTE;
	key_remote.key.ioint.name = NULL;
	key_remote.key.ioint.gpio = (GPIO_BANK0 | GPIO_D3);
	key_remote.key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
	key_remote.key.ioint.pressed_state = 0;
	key_remote.key.ioint.press_time = 0;
	
	remotectlInitInDriver();
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
        //select gpio0d3_sel for pwm3(IR)
        grf_writel(0x40 | (0x40 << 16), 0xb4);
#elif  (CONFIG_RKCHIPTYPE == CONFIG_RK3128)
	//select gpio3d2_sel for pwm3_irin
	grf_writel(0x08 | (0x08 << 16), 0xe4);
#endif
	//install the irq hander for PWM irq.
	irq_install_handler(IRQ_PWM, remotectl_do_something, NULL);
	irq_handler_enable(IRQ_PWM);
	return 0;
}
#endif

int power_hold(void)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	/* no power hold */
#else
	if (get_rockchip_pmic_id() == PMIC_ID_RICOH619)
		return ricoh619_poll_pwr_key_sta();
	else
		return GetPortState(&key_power);
#endif
}


void key_init(void)
{
#ifdef CONFIG_RK_PWM_REMOTE
        RemotectlInit();
#endif

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	RockusbKeyInit(&key_rockusb);
#else
	charge_state_gpio.name = "charge_state";
	charge_state_gpio.flags = 0;
	charge_state_gpio.gpio = ((GPIO_BANK0 << RK_GPIO_BANK_OFFSET) | GPIO_B0);
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
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	/* no power hold */
#else
	if(power_hold_gpio.name != NULL)
		gpio_direction_output(power_hold_gpio.gpio, power_hold_gpio.flags);
#endif
}

void powerOff(void)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	/* no power hold */
#else
	if(power_hold_gpio.name != NULL)
		gpio_direction_output(power_hold_gpio.gpio, !power_hold_gpio.flags);
#endif
}

