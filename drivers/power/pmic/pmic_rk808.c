/*
 *  Copyright (C) 2012 rockchips
 *  zyw < zyw@rock-chips.com >
 *  for sample
 */

#include <common.h>
#include <power/pmic.h>
#include <i2c.h>
#include <errno.h>

/*
for chack charger status in boot
return 0, no charger
return 1, charging
*/
int check_charge(void)
{
    int reg=0;
    int ret = 0;
    if(IReadLoaderFlag() == 0 && 0==IReadLoaderMode()) {
		if(GetVbus()) { 	  //reboot charge
			printf("In charging! \n");
			ret = 1;
		}
    }
    
    return ret;
}



/*
set charge current
0. disable charging  
1. usb charging, 500mA
2. usb adapter charging, 2A
3. ac adapter charging , 3A
*/
int pmic_charger_setting(int current)
{
    printf("%s charger_type = %d\n",__func__,current);
    i2c_set_bus_num(0);
    i2c_init (CONFIG_SYS_I2C_SPEED, 0x6b);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
    printf("%s charge ic id = 0x%x\n",__func__,i2c_reg_read(0x6b,0x0a));
    switch (current){
    case 0:
        //disable charging
        break;
    case 1:
        //set charger current to 500ma
        break;
    case 2:
         //set charger current to 1.5A
        i2c_reg_write(0x6b,0,(i2c_reg_read(0x6b,0)&0xf8)|0x6);/* Input Current Limit  2A */
        break;
    case 3:
        //set charger current to 1.5A
        i2c_reg_write(0x6b,0,(i2c_reg_read(0x6b,0)&0xf8)|0x7);/* Input Current Limit 3A */
        break;
    default:
        break;
    }
    return 0;
}

int charger_init(unsigned char bus)
{
    int usb_charger_type = 1;//dwc_otg_check_dpdm();
    ChargerStateInit();
    printf("%s, charger_type = %d, dc_is_charging= %d\n",__func__,usb_charger_type,is_charging());
    if(is_charging()){
        pmic_charger_setting(3);
    }else if(usb_charger_type){
        pmic_charger_setting(usb_charger_type);
    }
    return 0;

}

int pmic_init(unsigned char bus)
{
    
    i2c_set_bus_num(0);
    i2c_init (CONFIG_SYS_I2C_SPEED, 0x1b);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
    //printf("*******%s MINUTES REG = 0x%x\n",__func__,i2c_reg_read(0x1b,0x1));
    i2c_reg_write(0x1b,0x23,i2c_reg_read(0x1b,0x23)|0x60);

    i2c_reg_write(0x1b,0x45,0x02); 
    return 0;
}


void shut_down(void)
{
    //shut down pmic or pull down power_hold pin

}

