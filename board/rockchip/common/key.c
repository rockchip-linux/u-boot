
#include <asm/arch/drivers.h>
#include <asm/arch/gpio.h>
#include <common.h>
#include "key.h"

int gpio_reg[] = {
	RK3288_GPIO0_PHYS,
	RK3288_GPIO1_PHYS,
	RK3288_GPIO2_PHYS,
	RK3288_GPIO3_PHYS,
	RK3288_GPIO4_PHYS,
	RK3288_GPIO5_PHYS,
	RK3288_GPIO6_PHYS,
	RK3288_GPIO7_PHYS,
	RK3288_GPIO8_PHYS
};

extern void DRVDelayUs(uint32 us);

gpio_conf charge_state_gpio;
gpio_conf power_hold_gpio;

key_config key_rockusb;
key_config key_recovery;
key_config key_fastboot;
key_config key_power;

static inline unsigned int get_duration(unsigned int base)
{
	unsigned int max = 0xFFFFFFFF;
	unsigned int now = get_rk_current_tick();
	unsigned int tick_duration =
	    base >= now ? base - now : max - (base - now) + 1;
	return tick_duration / (CONFIG_SYS_CLK_FREQ / CONFIG_SYS_HZ);	//tick_to_time(tick_duration);
}

void gpio_isr(int gpio_group)
{
	int i = 0;
	int_conf *ioint = NULL;
	printf("gpio isr,gpio_group=%d \n", gpio_group);
	ioint = &key_power.key.ioint;

	if ((gpio_irq_state(ioint->gpio))) {
		printf("key_power gpio isr\n");
		gpio_irq_clr(ioint->gpio);
		if (get_wfi_status()) {
			printf("gpio isr in wfi\n");
			return;
		}
		if (ioint->press_time) {
			//it's key up.
			int type =
			    (get_duration(ioint->press_time) >
			     LONE_PRESS_TIME) ? KEY_LONG_PRESS :
			    KEY_SHORT_PRESS;
			ioint->press_time = 0;

			//we should make sure long press event will pass to user.
			//we should ignore double short press.
			switch (ioint->pressed_state) {
			case KEY_LONG_PRESS:
				//still have a long pressed to be process.
				//do nothing here, should not skip this long press event.
				break;
			case KEY_SHORT_PRESS:
				if (type == KEY_LONG_PRESS)
					//new long press, ignore old short press event.
					ioint->pressed_state = type;
				else
					//ignore double press.
					ioint->pressed_state = 0;
				break;
			default:
				//new press event.
				ioint->pressed_state = type;
				break;
			}
		} else {
			ioint->press_time = get_rk_current_tick();
		}

		serial_printf("switch polarity\n");
		if (ioint->flags == IRQ_TYPE_EDGE_FALLING) {
			ioint->flags = IRQ_TYPE_EDGE_RISING;
		} else
			ioint->flags = IRQ_TYPE_EDGE_FALLING;

		gpio_irq_request(ioint->gpio, ioint->flags);
	}

	if (gpio_group >= 0 && gpio_group < sizeof(gpio_reg) / sizeof(int))
		for (i = 0; i < 32; i++)
			gpio_irq_clr(gpio_reg[gpio_group] + i);	//clr all gpio0 int
}


/*
    固定GPIOA_0口作为烧写检测口,系统部分不能使用该口
*/
int GetPortState(key_config * key)
{
	uint32 tt;
	uint32 hCnt = 0;
	adc_conf *adc = &key->key.adc;
	int_conf *ioint = &key->key.ioint;
	if (key->type == KEY_AD) {
		// TODO: clk没有配置
		for (tt = 0; tt < 10; tt++) {
			// read special gpio port value.
			uint32 value;
			uint32 timeout = 0;
			write_XDATA32(adc->ctrl, 0);
			DRVDelayUs(1);
			write_XDATA32(adc->ctrl, 0x0028 | (adc->index));
			DRVDelayUs(1);
			do {
				value = read_XDATA32(adc->ctrl);
				timeout++;
			} while ((value & 0x40) == 0);
			value = read_XDATA32(adc->data);
			//printf("adc key = %d\n",value);
			//DRVDelayUs(1000);
			if (value <= adc->keyValueHigh
			    && value >= adc->keyValueLow)
				hCnt++;
		}
		write_XDATA32(adc->ctrl, 0);
		return (hCnt > 8);
	} else if (key->type == KEY_INT) {
		if (ioint->press_time) {
			//still pressed
			if (get_duration(ioint->press_time) >
			    LONE_PRESS_TIME) {
				//update state, if it's a long press.
				ioint->pressed_state = KEY_LONG_PRESS;
			}
		}
		//return and reset state.
		int type = ioint->pressed_state;
		ioint->pressed_state = 0;
		return type;
	}
}


int checkKey(uint32 * boot_rockusb, uint32 * boot_recovery,
	     uint32 * boot_fastboot)
{
	int i;
	int recovery_key = 0;
	*boot_rockusb = 0;
	*boot_recovery = 0;
	*boot_fastboot = 0;
	printf("checkKey\n");
	if (GetPortState(&key_rockusb)) {
		*boot_rockusb = 1;
		//printf("rockusb key is pressed\n");
	}
	if (GetPortState(&key_recovery)) {
		*boot_recovery = 1;
		//printf("recovery key is pressed\n");
	}
	if (GetPortState(&key_fastboot)) {
		*boot_fastboot = 1;
		//printf("fastboot key is pressed\n");
	}
	return 0;
}

void RockusbKeyInit(key_config * key)
{
	key->type = KEY_AD;
	key->key.adc.index = 1;
	key->key.adc.keyValueLow = 0;
	key->key.adc.keyValueHigh = 30;
	key->key.adc.data = SARADC_BASE;
	key->key.adc.stas = SARADC_BASE + 4;
	key->key.adc.ctrl = SARADC_BASE + 8;
}

void RecoveryKeyInit(key_config * key)
{
	key->type = KEY_AD;
	key->key.adc.index = 1;
	key->key.adc.keyValueLow = 0;
	key->key.adc.keyValueHigh = 30;
	key->key.adc.data = SARADC_BASE;
	key->key.adc.stas = SARADC_BASE + 4;
	key->key.adc.ctrl = SARADC_BASE + 8;
}


void FastbootKeyInit(key_config * key)
{
	key->type = KEY_AD;
	key->key.adc.index = 1;
	key->key.adc.keyValueLow = 170;
	key->key.adc.keyValueHigh = 180;
	key->key.adc.data = SARADC_BASE;
	key->key.adc.stas = SARADC_BASE + 4;
	key->key.adc.ctrl = SARADC_BASE + 8;
}


void PowerKeyInit()
{
	int i = 0;
	//power_hold_gpio.name
	key_power.type = KEY_INT;
	key_power.key.ioint.name = "power_key";
	key_power.key.ioint.gpio = RK3288_GPIO0_PHYS | GPIO_A5;
	key_power.key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
	key_power.key.ioint.pressed_state = 0;
	key_power.key.ioint.press_time = 0;

	printf("%s start\n", __func__);
	// for(i=0; i<32; i++)
	gpio_irq_request(RK3288_GPIO0_PHYS + i, IRQ_TYPE_NONE);	//disable all gpio 0 Interrupt
	gpio_direction_input(key_power.key.ioint.gpio);
	gpio_irq_request(key_power.key.ioint.gpio,
			 key_power.key.ioint.flags);
	IRQEnable(INT_GPIO0);
	printf("%s end\n", __func__);
}

int power_hold()
{
	return GetPortState(&key_power);
}


void key_init()
{
	charge_state_gpio.name = "charge_state";
	charge_state_gpio.flags = 0;
	charge_state_gpio.gpio = RK3288_GPIO0_PHYS | GPIO_B0;
	gpio_direction_input(charge_state_gpio.gpio);

	//power_hold_gpio.name

	RockusbKeyInit(&key_rockusb);
	FastbootKeyInit(&key_fastboot);
	RecoveryKeyInit(&key_recovery);
	PowerKeyInit();
}

/*
return 0: no charger
return 1: charging
*/
int is_charging()
{
	if (charge_state_gpio.name != NULL)
		return (charge_state_gpio.flags ==
			gpio_get_value(charge_state_gpio.gpio));
	else
		return 0;
}

void powerOn(void)
{
	if (power_hold_gpio.name != NULL)
		gpio_direction_output(power_hold_gpio.gpio,
				      power_hold_gpio.flags);
}

void powerOff(void)
{
	if (power_hold_gpio.name != NULL)
		gpio_direction_output(power_hold_gpio.gpio,
				      !power_hold_gpio.flags);
}
