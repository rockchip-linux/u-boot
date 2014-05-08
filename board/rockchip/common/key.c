
#include <asm/arch/drivers.h>
#include <asm/arch/gpio.h>
#include <common.h>
#include "key.h"

int gpio_reg[]={
	RKIO_GPIO0_PHYS, 
	RKIO_GPIO1_PHYS, 
	RKIO_GPIO2_PHYS,
	RKIO_GPIO3_PHYS,
	RKIO_GPIO4_PHYS,
	RKIO_GPIO5_PHYS,
	RKIO_GPIO6_PHYS,
	RKIO_GPIO7_PHYS,
	RKIO_GPIO8_PHYS
};

extern void DRVDelayUs(uint32 us);

gpio_conf       charge_state_gpio;
gpio_conf       power_hold_gpio;

key_config		key_rockusb;
key_config		key_recovery;
key_config		key_fastboot;
key_config      key_power;


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
    		do{
    		    value = read_XDATA32(adc->ctrl);
    		    timeout++;
            }while((value&0x40)==0);
    		value = read_XDATA32(adc->data);
            //printf("adc key = %d\n",value);
    		//DRVDelayUs(1000);
    		if( value<=adc->keyValueHigh && value>=adc->keyValueLow)
                hCnt++;
    	}
        write_XDATA32( adc->ctrl, 0);
        return (hCnt>8);
    }
}


int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot)
{
    int i;
    int recovery_key = 0;
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
    key->type = KEY_AD;
	key->key.adc.index = 1;	
    key->key.adc.keyValueLow = 0;
    key->key.adc.keyValueHigh= 30;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
}

void RecoveryKeyInit(key_config *key)
{
    key->type = KEY_AD;
	key->key.adc.index = 1;	
    key->key.adc.keyValueLow = 0;
    key->key.adc.keyValueHigh= 30;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
}


void FastbootKeyInit(key_config *key)
{
    key->type = KEY_AD;
	key->key.adc.index = 1;	
    key->key.adc.keyValueLow = 170;
    key->key.adc.keyValueHigh= 180;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
}


void PowerKeyInit()
{
    int i=0;
    //power_hold_gpio.name
    key_power.type = KEY_INT;
    key_power.key.ioint.name = "power_key";
    key_power.key.ioint.gpio = (GPIO_BANK0 | GPIO_A5);
    key_power.key.ioint.flags = IRQ_TYPE_EDGE_FALLING;
    key_power.key.ioint.pressed_state = 0;
    key_power.key.ioint.press_time = 0;
}

int power_hold() {
    return GetPortState(&key_power);
}


void key_init()
{

    charge_state_gpio.name = "charge_state";
    charge_state_gpio.flags = 0;
    charge_state_gpio.gpio = (GPIO_BANK0 | GPIO_B0);
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
    if(charge_state_gpio.name != NULL)
        return (charge_state_gpio.flags == gpio_get_value(charge_state_gpio.gpio));
    else return 0;
}

void powerOn(void)
{
    if(power_hold_gpio.name!=NULL)gpio_direction_output(power_hold_gpio.gpio, power_hold_gpio.flags);
}

void powerOff(void)
{
    if(power_hold_gpio.name!=NULL)gpio_direction_output(power_hold_gpio.gpio, !power_hold_gpio.flags);
}



