/*
 * Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * zyw < zyw@rock-chips.com >
 * for sample
 *
 * SPDX-License-Identifier:	GPL-2.0+
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
	if(GetVbus()) { 	  //reboot charge
		printf("In charging! \n");
		ret = 1;
	}

    return ret;
}



/*
set charge current
0. disable charging  
1. usb charging, 500mA
2. ac adapter charging, 1.5A
*/
int pmic_charger_setting(int current)
{

    printf("%s %d\n",__func__,current);
    switch (current){
    case 0:
        //disable charging
        break;
    case 1:
        //set charger current to 500ma
        break;
    case 2:
         //set charger current to 1.5A
        break;
    default:
        break;
    }
    return 0;
}

int pmic_init(unsigned char bus)
{
    //enable lcdc power ldo, and enable other ldo   

	return 0;
}

void shut_down(void)
{
    //shut down pmic or pull down power_hold pin

}

