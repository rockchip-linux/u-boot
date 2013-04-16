#include <linux/types.h>
#include <asm/arch/rk30_drivers.h>
#include "gpio_oper.h"
#include "parameter.h"


#define     read_XDATA(address)             (*((uint16 volatile*)(address)))
#define     read_XDATA32(address)           (*((uint32 volatile*)(address)))
#define     write_XDATA(address, value)     (*((uint16 volatile*)(address)) = value)
#define     write_XDATA32(address, value)   (*((uint32 volatile*)(address)) = value)


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
void setup_gpio(gpio_conf *key_gpio)
{
    uint32 base_addr = 0;
    
#if 0
    if (ChipType == CHIP_RK3066)
    {
#endif
        if(key_gpio->group >= 7)
            return;
        base_addr = RK3066GpioBaseAddr[key_gpio->group];
#if 0
    }
    else
    {
        if(key_gpio->group >= 4)
            return;
        base_addr = RK3188GpioBaseAddr[key_gpio->group];
    }
#endif

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
#ifndef RK_LOADER_FOR_FT
    uint32 tt;
    uint32 hCnt = 0;
    gpio_conf* gpio = &key->key.gpio;
    adc_conf* adc = &key->key.adc; 
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
    		udelay(1);
    	}
        return (hCnt>80);
        #endif
    }
    else
    {
        // TODO: clk没有配置
    	for(tt = 0; tt < 10; tt++)
    	{
    	    // read special gpio port value.
    	    uint32 value;
    	    uint32 timeout = 0;
            write_XDATA32( adc->ctrl, 0);
    	    udelay(1);
            write_XDATA32( adc->ctrl, 0x0028|(adc->index));
    		udelay(1);
    		do{
    		    value = read_XDATA32(adc->ctrl);
    		    timeout++;
            }while((value&0x40)==0);
    		value = read_XDATA32(adc->data);
            //PRINT_E("adc key = %d\n",value);
    		//udelay(1000);
    		if( value<=adc->keyValueHigh && value>=adc->keyValueLow)
                hCnt++;
    	}
        write_XDATA32( adc->ctrl, 0);
        return (hCnt>8);
    }
#else
    return 0;
#endif
}

int checkKey(uint32* boot_rockusb, uint32* boot_recovery)
{
    int i;
    int recovery_key = 0;
	if( GetPortState(&key_recover) )// 按下Recover键
	{
        *boot_rockusb = 1;// 进入rockusb
	    //PRINT_E("RECOVERY key is pressed\n");
	}
	//else
	{
        for(i=0;i<KeyCombinationNum;i++)
        {
    		//if(key_combination[i].type)
    		{
    		    // cmy@20100409: 在判断combination按键时，先初始化该IO口
    			if(GetPortState(&key_combination[i]))// 按下组合键
    			{
    			    //PRINT_E("COMBINATION key is pressed\n");
    				*boot_recovery = TRUE;// 进入Recovery 系统
                    *boot_rockusb = 0;// 进入rockusb
                    if(key_combination[i].type == KEY_GPIO) // TODO:按键值需要修改
                        recovery_key = key_combination[i].key.gpio.group*32+key_combination[i].key.gpio.index;
    				break; 
    			}	
    		}
	    }
	}
	return recovery_key;
}

/*static int RKGetChipTag(void)
{
    uint32 i;
    uint32 hCnt = 0;
    uint32 valueL;
    uint32 valueH;
    uint32 value;
    write_XDATA32((GPIO6_BASE_ADDR+0x4), (read_XDATA32(GPIO6_BASE_ADDR+0x4)&(~(0x3ul<<14)))); // portD 4:6 input
    valueH = (read_XDATA32(GPIO6_BASE_ADDR+0x50)>>14)&0x03;
    //PRINT_E("valueH = %x\n",valueH);
    write_XDATA32((GRF_BASE+0x148), 0xC000C000); // GPIO 6 B6~7 disable pull up
    write_XDATA32((GPIO6_BASE_ADDR+0x4), (read_XDATA32(GPIO6_BASE_ADDR+0x4)|((0x3ul<<14)))); // portD 4:6 out
    write_XDATA32((GPIO6_BASE_ADDR+0), (read_XDATA32(GPIO6_BASE_ADDR+0)&(~(0x3ul<<14)))); //out put low
    write_XDATA32((GPIO6_BASE_ADDR+0x4), (read_XDATA32(GPIO6_BASE_ADDR+0x4)&(~(0x3ul<<14)))); // portD 4:6 input
    valueL = (read_XDATA32(GPIO6_BASE_ADDR+0x50)>>14)&0x03;
    //PRINT_E("valueL = %x\n",valueL);
    write_XDATA32((GRF_BASE+0x148), 0xC0000000); // GPIO 6 B6~7 enable pull up
    value = (valueH<<2)|valueL;
    //PRINT_E("value = 0x%x\n",value);
    return value;
}*/


void recoveryKeyInit(key_config *key)
{
    key->type = KEY_AD;
    key->key.adc.index = 1;
    key->key.adc.keyValueLow = 0;
    key->key.adc.keyValueHigh= 30;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
    key_powerHold.type = KEY_GPIO;
    key_powerHold.key.gpio.valid = 1; 
#if 0
    if(ChipType == CHIP_RK3066)
    {
#endif
        key_powerHold.key.gpio.group = 6;
        key_powerHold.key.gpio.index = 8; // gpio6B0
#if 0
    }
    else
    {
        key_powerHold.key.gpio.group = 0;
        key_powerHold.key.gpio.index = 0; // gpio0A0
        //rknand_print_hex("grf:", g_3066B_grfReg,1,512);
    }
#endif
    setup_gpio(&key_powerHold.key.gpio);
}

