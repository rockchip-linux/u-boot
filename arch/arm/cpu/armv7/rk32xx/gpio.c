
#include <asm/arch/drivers.h>
#include <common.h>
#include "gpio.h"
uint32 g_Rk30xxChip;

extern void DRVDelayUs(uint32 us);
key_config		key_rockusb;
key_config		key_recovery;
key_config		key_fastboot;
key_config		pin_powerHold;
key_config      key_power;
extern uint8  ChipType;

uint32 RK3066GpioBaseAddr[7] = 
{
    0x20034000, //GPIO0
    0x2003C000, //GPIO1
    0x2003E000, //GPIO2
    0x20080000, //GPIO3
    0x20084000, //GPIO4
    0x20088000, //GPIO5
    0x2000A000, //GPIO6
};

uint32 RK3188GpioBaseAddr[7] = 
{
    0x2000A000, //GPIO0
    0x2003C000, //GPIO1
    0x2003E000, //GPIO2
    0x20080000, //GPIO3
};
uint32 RK3026GpioBaseAddr[7] = 
{
    0x2007c000, //GPIO0
    0x20080000, //GPIO1
    0x20084000, //GPIO2
    0x20088000, //GPIO3
};
uint32 RK3288GpioBaseAddr[9] = 
{
    0xff750000, //GPIO0
    0xff780000, //GPIO1
    0xff790000, //GPIO2
    0xff7a0000, //GPIO3
    0xff7b0000, //GPIO4
    0xff7c0000, //GPIO5
    0xff7d0000, //GPIO6
    0xff7e0000, //GPIO7
    0xff7f0000, //GPIO8
};

static inline unsigned int get_duration(unsigned int base) {
    unsigned int max = 0xFFFFFFFF;
    unsigned int now = get_rk_current_tick();
	unsigned int tick_duration = base >= now? base - now : max - (base - now) + 1;
	return tick_duration/(CONFIG_SYS_CLK_FREQ/CONFIG_SYS_HZ);//tick_to_time(tick_duration);
}

void setup_gpio(gpio_conf *key_gpio)
{
    uint32 base_addr = 0;
    ChipTypeCheck();

    if (ChipType == CONFIG_RK3066)
    {
        if(key_gpio->group >= 7)
            return;
        base_addr = RK3066GpioBaseAddr[key_gpio->group];
    }
    else if(ChipType == CONFIG_RK3026)
   {
	 if(key_gpio->group >= 4)
            return;
        base_addr = RK3026GpioBaseAddr[key_gpio->group];	
    }
    else if(ChipType == CONFIG_RK3188)
    {
        if(key_gpio->group >= 4)
            return;
        base_addr = RK3188GpioBaseAddr[key_gpio->group];
    }
    else
    {
        if(key_gpio->group >= 9)
            return;
        base_addr = RK3288GpioBaseAddr[key_gpio->group];
    }
	key_gpio->io_read = base_addr+0x50;
	key_gpio->io_write = base_addr;
	key_gpio->io_dir_conf = base_addr+0x4;
	key_gpio->io_debounce = base_addr+0x48;
}

void setup_adckey(adc_conf *key_adc)
{       
//key  0    1      2       3       4       5
//vol  0mv  300mv  576mv   753mv   852mv   952mv
//ad   0    124    233     304     345     386
    key_adc->keyValueLow *= 10;
    key_adc->keyValueHigh *= 10;
    key_adc->data = SARADC_BASE;
    key_adc->stas = SARADC_BASE+4;
    key_adc->ctrl = SARADC_BASE+8;
    //PRINT_E("keyValueLow: %d\n", key_adc->keyValueLow );
    //PRINT_E("keyValueHigh: %d\n", key_adc->keyValueHigh);
}

void clr_all_gpio_int(void)
{
    uint32 base_addr,int_en,int_mask,int_eoi;
    int group = 0;
    
    for(group=0; group<7; group++)
    {
        if (ChipType == CONFIG_RK3066)
        {
            if(group >= 7)
                return;
            base_addr = RK3066GpioBaseAddr[group];
        }
    	else if(ChipType == CONFIG_RK3026)
    	 {
    	    if(group >= 4)
    	            return;
    	    base_addr = RK3026GpioBaseAddr[group];	
    	 }
        else if(ChipType == CONFIG_RK3188)
        {
            if(group >= 4)
                return;
            base_addr = RK3188GpioBaseAddr[group];
        }
        else
        {
            if(group >= 9)
                return;
            base_addr = RK3288GpioBaseAddr[group];
        }
    	int_en = base_addr+0x30;
    	write_XDATA32(int_en, 0);
    }
}


void setup_int(int_conf *key_int)
{
    uint32 base_addr = 0;

        if(key_int->group >= 9)
            return;
        base_addr = RK3288GpioBaseAddr[key_int->group];
	key_int->int_en = base_addr+0x30;
	key_int->int_mask = base_addr+0x34;
	key_int->int_level = base_addr+0x38;
	key_int->int_polarity = base_addr+0x3c;
	key_int->int_status = base_addr+0x40;
	key_int->int_eoi = base_addr+0x4c;
	key_int->io_read = base_addr+0x50;
	key_int->io_dir_conf = base_addr+0x4;
	key_int->io_debounce = base_addr+0x48;
	key_int->pressed_state = 0;
	key_int->press_time = 0;
	//write_XDATA32(key_int->io_dir_conf, read_XDATA32(key_int->int_mask)&(~(1ul<<key_int->index)));
	//write_XDATA32(key_int->int_eoi, (read_XDATA32(key_int->int_eoi)|(1ul<<key_int->index)));
	write_XDATA32(key_int->int_mask, read_XDATA32(key_int->int_mask)&(~(1ul<<key_int->index)));
	write_XDATA32(key_int->int_level, read_XDATA32(key_int->int_level)|(1ul<<key_int->index));//use edge sensitive
	write_XDATA32(key_int->io_debounce, (read_XDATA32(key_int->io_debounce)|((1ul<<key_int->index))));
	//printf("valid = %d\n", key_int->valid);
	if(key_int->valid)
		write_XDATA32(key_int->int_polarity, read_XDATA32(key_int->int_polarity)|(1ul<<key_int->index));
	else
		write_XDATA32(key_int->int_polarity, read_XDATA32(key_int->int_polarity)&(~(1ul<<key_int->index)));
	//printf("polarity:%x\n", read_XDATA32(key_int->int_polarity));
	write_XDATA32(key_int->int_en, read_XDATA32(key_int->int_en)|(1ul<<key_int->index));
}

void gpio_isr(int gpio_group)
{
   uint32 base_addr = 0;
   uint32  int_eoi;

       if(gpio_group >= 9)
            return;
       base_addr = RK3288GpioBaseAddr[gpio_group];
   int_eoi = base_addr+0x4c;

	if(key_power.type==KEY_INT)
	{
		int_conf* ioint = &key_power.key.ioint; 
		int i;
		serial_printf("gpio_group = %d, int_status = %x \n ",gpio_group,read_XDATA32(ioint->int_status));
		if(read_XDATA32(ioint->int_status)&(1ul<<ioint->index))
		{
			//serial_printf("gpio isr\n");
			write_XDATA32( ioint->int_eoi, (read_XDATA32(ioint->int_eoi)|(1ul<<ioint->index)));
			if(get_wfi_status())
			{
				serial_printf("gpio isr in wfi\n");
				return;
			}
			if(ioint->press_time)
			{
                //it's key up.
                int type = (get_duration(ioint->press_time)>LONE_PRESS_TIME) ? KEY_LONG_PRESS:KEY_SHORT_PRESS;
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
			}
			else
			{
				ioint->press_time = get_rk_current_tick();
			}
			
			serial_printf("switch polarity\n");
			if(read_XDATA32(ioint->int_polarity)&(1ul<<ioint->index))
				write_XDATA32(ioint->int_polarity, read_XDATA32(ioint->int_polarity)&(~(1ul<<ioint->index)));
			else
				write_XDATA32(ioint->int_polarity, read_XDATA32(ioint->int_polarity)|(1ul<<ioint->index));
		}
	}

	write_XDATA32(int_eoi, 0xffffffff);   //clr all gpio int
	
}

#if 0
void dump_gpio(gpio_conf* gpio)
{
    RkPrintf("group: %d\n", gpio->group);
    RkPrintf("index: %d\n", gpio->index);
    RkPrintf("valid: %d\n", gpio->valid);
    RkPrintf("io_read: %08X\n", gpio->io_read);
    RkPrintf("io_write: %08X\n", gpio->io_write);
    RkPrintf("io_dir_conf: %08X\n", gpio->io_dir_conf);
    RkPrintf("io_debounce: %08X\n", gpio->io_debounce);
}
#endif
/*
    固定GPIOA_0口作为烧写检测口,系统部分不能使用该口
*/
int GetPortState(key_config *key)
{
    uint32 tt;
    uint32 hCnt = 0;
    gpio_conf* gpio = &key->key.gpio;
    adc_conf* adc = &key->key.adc; 
    int_conf* ioint = &key->key.ioint; 
    if(key->type == KEY_GPIO)
    {
        // TODO: 按键没有处理
        #if 1 
        // set direction as input 
        write_XDATA32( gpio->io_dir_conf, (read_XDATA32(gpio->io_dir_conf)&(~(1ul<<gpio->index))));
        // enable debounce
        write_XDATA32((gpio->io_debounce), (read_XDATA32(gpio->io_debounce)|((1ul<<gpio->index))));
        for(tt = 0; tt < 100; tt++)
    	{
    	    // read special gpio port value.
    		uint32 value = read_XDATA32(gpio->io_read);
    		if( ((value>>gpio->index)&0x01) == gpio->valid )
    	            hCnt++;
    		DRVDelayUs(1);
    	}
        return (hCnt>80);
        #endif
    }
    else if(key->type == KEY_AD)
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
    else if(key->type == KEY_INT)
	{
		if(read_XDATA32(ioint->int_en)&(1ul<<ioint->index))
		{
			if(ioint->press_time)
			{
                //still pressed
				if(get_duration(ioint->press_time)>LONE_PRESS_TIME) {
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
}

int SetPortOutput(int group, int index, int level)
{
    gpio_conf key_gpio;
    key_gpio.group = group;
    setup_gpio(&key_gpio);
    key_gpio.index = index;

    write_XDATA32( key_gpio.io_dir_conf, (read_XDATA32(key_gpio.io_dir_conf)|(1ul<<key_gpio.index)));
    if(level)
        write_XDATA32( key_gpio.io_write, (read_XDATA32(key_gpio.io_write)|(1ul<<key_gpio.index)));
    else write_XDATA32( key_gpio.io_write, (read_XDATA32(key_gpio.io_write)&(~(1ul<<key_gpio.index))));
    return 0 ;
}

int GetPortInput(int group, int index)
{
    gpio_conf key_gpio;
    key_gpio.group = group;
    setup_gpio(&key_gpio);
    key_gpio.index = index;

    // set direction as input 

    write_XDATA32( key_gpio.io_dir_conf, (read_XDATA32(key_gpio.io_dir_conf)&(~(1ul<<key_gpio.index))));
    return read_XDATA32(key_gpio.io_read);
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
    if(ChipType == CONFIG_RK3026)
    	key->key.adc.index = 3;
    else
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
    if(ChipType == CONFIG_RK3026)
    	key->key.adc.index = 3;
    else
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
    if(ChipType == CONFIG_RK3026)
    	key->key.adc.index = 3;
    else
	key->key.adc.index = 1;	
    key->key.adc.keyValueLow = 170;
    key->key.adc.keyValueHigh= 180;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
}

void PowerHoldPinInit()
{
   // pin_powerHold.type = KEY_GPIO;
   // pin_powerHold.key.gpio.valid = 1; 

  //  pin_powerHold.key.gpio.group = 0;
  //  pin_powerHold.key.gpio.index = 0; // gpio0A0
  //  setup_gpio(&pin_powerHold.key.gpio);
  //  if(pin_powerHold.key.gpio.valid)
  //      powerOn();
}
void PowerKeyInit()
{
#if 0
    key_power.type = KEY_GPIO;
    key_power.key.gpio.valid = 0; 
    if(ChipType == CONFIG_RK3066)
    {
        key_power.key.gpio.group = 6;
        key_power.key.gpio.index = 8; // gpio6B0
    }
    else
    {
        key_power.key.gpio.group = 0;
        key_power.key.gpio.index = 4; // gpio0A4
        //rknand_print_hex("grf:", g_3188_grfReg,1,512);
    }

    setup_gpio(&key_power.key.gpio);
    if(key_power.key.gpio.valid)
        powerOn();
#else
    key_power.type = KEY_INT;
    key_power.key.ioint.valid = 0; 
    if(ChipType == CONFIG_RK3066)
    {
        key_power.key.ioint.group = 6;
        key_power.key.ioint.index = 8; // gpio6B0
    }
    else
    {
        key_power.key.ioint.group = 0;
        key_power.key.ioint.index = 4; // gpio0A4
    }
	printf("setup gpio int\n");
	clr_all_gpio_int();
    setup_int(&key_power.key.ioint);
	IRQEnable(INT_GPIO0);
#endif

}

int power_hold() {
    return GetPortState(&key_power);
}


static key_config charger_state;
void ChargerStateInit()
{
    charger_state.type = KEY_GPIO;
    charger_state.key.gpio.valid = 1;
#ifdef CONFIG_RK3288SDK 
     charger_state.key.gpio.group = 0;//gpio0
     charger_state.key.gpio.index = 8;//b0
#else
     charger_state.key.gpio.group = 0;
     charger_state.key.gpio.index = 10;
#endif


    setup_gpio(&charger_state.key.gpio);
}
/*
return 0: no charger
return 1: charging
*/
int is_charging()
{
    return !GetPortState(&charger_state);  //gpio0_b2, charger in status
}


void key_init()
{
	RockusbKeyInit(&key_rockusb);
	FastbootKeyInit(&key_fastboot);
	RecoveryKeyInit(&key_recovery);
	PowerKeyInit();
}
void power_io_ctrl(uint8 mode)
{
    gpio_conf *key_gpio = &pin_powerHold.key.gpio;
    if(mode)         // 输出高电平
    {
        write_XDATA32((key_gpio->io_write), ReadReg32((key_gpio->io_write))|((1ul<<key_gpio->index))); //out put high
        write_XDATA32(key_gpio->io_dir_conf, (read_XDATA32(key_gpio->io_dir_conf)|((1ul<<key_gpio->index)))); // port6 B0 out
    }
    else                //其他情况输出低电平
    {
        write_XDATA32((key_gpio->io_write), ReadReg32((key_gpio->io_write))&(~(1ul<<key_gpio->index))); //out put high
        write_XDATA32(key_gpio->io_dir_conf, (read_XDATA32(key_gpio->io_dir_conf)|((1ul<<key_gpio->index)))); // port6 B0 out
    }
}

void powerOn(void)
{
    gpio_conf *key_gpio = &pin_powerHold.key.gpio;
    power_io_ctrl((key_gpio->valid)&0x01);
}

void powerOff(void)
{
    gpio_conf *key_gpio = &pin_powerHold.key.gpio;
    power_io_ctrl((key_gpio->valid&0x01)==0);
}



