
//#include "config.h"
#include    "../../armlinux/config.h"
uint32 g_Rk2928xxChip;
#define RK2928G_CHIP_TAG 0x1
#define RK2928L_CHIP_TAG 0x2
#define RK2926_CHIP_TAG  0x0
extern void DRVDelayUs(uint32 us);

void setup_gpio(gpio_conf *key_gpio)
{
    uint32 base_addr = 0;
    switch( key_gpio->group )
    {
    case 0:
        base_addr = GPIO0_BASE_ADDR;
        break;
    case 1:
        base_addr = GPIO1_BASE_ADDR;
        break;
    case 2:
        base_addr = GPIO2_BASE_ADDR;
        break;
    default:
        return;
        break;
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
    		DRVDelayUs(1);
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
    	    DRVDelayUs(1);
            write_XDATA32( adc->ctrl, 0x0028|(adc->index));
    		DRVDelayUs(1);
    		do{
    		    value = read_XDATA32(adc->ctrl);
    		    timeout++;
            }while((value&0x40)==0);
    		value = read_XDATA32(adc->data);
            //PRINT_E("adc key = %d\n",value);
    		//DRVDelayUs(1000);
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
	    PRINT_E("RECOVERY key is pressed\n");
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
    			    PRINT_E("COMBINATION key is pressed\n");
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

/*int RKGetChipTag(void)
{
    uint32 i;
    uint32 hCnt = 0;
    uint32 valueL;
    uint32 valueH;
    uint32 value;
    WriteReg32((GPIO3_BASE_ADDR+0x4),  (ReadReg32(GPIO3_BASE_ADDR+0x4)&(~(0x7ul<<0)))); //gpio3   portA 0:2 input
    value = (ReadReg32(GPIO3_BASE_ADDR+0x50))&0x07;
    return value;
}*/

void RockusbKeyInit(key_config *key)
{
    key->type = KEY_AD;
    key->key.adc.index = 1;
    key->key.adc.keyValueLow = 0;
    key->key.adc.keyValueHigh= 30;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
	
	g_Rk2928xxChip = RKGetChipTag();
    /*if(g_Rk2928xxChip != RK2928G_CHIP_TAG && g_Rk2928xxChip != RK2928L_CHIP_TAG
    &&g_Rk2928xxChip != RK2926_CHIP_TAG ) //只支持3066
    {
        key->key.adc.keyValueLow = 0;
        key->key.adc.keyValueHigh= 1023;
    }*/
    key_power.type = KEY_GPIO;
    key_power.key.gpio.group = 1;
    key_power.key.gpio.valid = 1; 
    if(g_Rk2928xxChip == RK2928G_CHIP_TAG)
    {
    	key_power.key.gpio.index = 1; 
    }
    else if(g_Rk2928xxChip == RK2926_CHIP_TAG)
    {
    	key_power.key.gpio.index = 2; 
    }
    setup_gpio(&key_power.key.gpio);
}

#if 0
void test_port()
{
    key_config key;
    int i,j;
    uint32 value = 0;
    uint32 iovalue[7];

	memset(&key, 0, sizeof(key));
    key.type = KEY_GPIO;
    key.key.gpio.index = 0;
    key.key.gpio.valid = 0;
    memset(iovalue, 0, 7*4);

    while(1)
    {
        for(i=0; i<7; i++)
        {
            key.key.gpio.group = i;
            setup_gpio(&key.key.gpio);
            // set direction as input
            write_XDATA32( key.key.gpio.io_dir_conf, (read_XDATA32(key.key.gpio.io_dir_conf)&0x00000000));
            // enable debounce
            write_XDATA32((key.key.gpio.io_debounce), (read_XDATA32(key.key.gpio.io_debounce)|0xFFFFFFFF));
            value = read_XDATA32(key.key.gpio.io_read);
            if(iovalue[i] != value)
            {
                RkPrintf("GPIO%d: %08X\n", i, value);
                iovalue[i] = value;
            }
        }
//        RkPrintf("---------------\n", i, value);
		DRVDelayS(1);
    }
}
#endif

