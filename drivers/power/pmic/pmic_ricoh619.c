/*
 *  Copyright (C) 2012 Samsung Electronics
 *  Lukasz Majewski <l.majewski@samsung.com>
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
#include <power/pmic.h>
#include <i2c.h>
#include <errno.h>


int check_charge(void)
{
    int reg=0;
    int ret = 0;
    if(0==IReadLoaderFlag() && 0==IReadLoaderMode()) {
        i2c_set_bus_num(1);
        i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
        i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
        reg = i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0x09);//
        printf("%s power on history %x \n",__func__,reg);
        if((reg & 0x04) || ((reg & 0x02)&& is_charging()))
        {
            printf("In charging! \n");
            ret = 1;
        }
    }
    
    return ret;
}

static int pmic_charger_state(struct pmic *p, int state, int current)
{
    return 0;
}


/*
0. disable charging  
1. usb charging
2. ac adapter charging
*/
int pmic_charger_setting(int current)
{
    i2c_set_bus_num(1);
    i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);

    i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0x0d, i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0x0d)|0x20);
    
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xbb, 0x23);  /* VFCHG	4.15v,	= 0x0-0x4 (4.05v 4.10v 4.15v 4.20v 4.35v)   */                                             
                                                     /* VRCHG   4.00,	= 0x0-0x4 (3.85v 3.90v 3.95v 4.00v 4.10v) */
    printf("%s %d\n",__func__,current);
    switch (current){
    case 0:
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb3, i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0xb3)&~0x03);      //disable charging    
        break;
    case 1:
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb8,0x4f);	 /* ICHG	1600ma,	= 0x0-0x1D (100mA - 3000mA) */
                                                         /* ICCHG   100ma   = 0x0-3 (50mA 100mA 150mA 200mA)*/
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb3, i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0xb3)|0x03);      //enable charging    
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb6,0x7);  /* ILIM_ADP 0x11= 0x0-0x1D (100mA - 3000mA) */
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb7,(i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0xb7)&0xe0)|0x07);/* ILIM_USB 0x07,= 0x0-0x1D (100mA - 3000mA) */
        break;
    case 2:
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb8,0x4f);	 /* ICHG	1600ma,	= 0x0-0x1D (100mA - 3000mA) */
                                                         /* ICCHG   100ma   = 0x0-3 (50mA 100mA 150mA 200mA)*/
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb3, i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0xb3)|0x03);      //enable charging 
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb6,0x10);	/* ILIM_ADP	0x11= 0x0-0x1D (100mA - 3000mA) */
        i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0xb7,(i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0xb7)&0xe0)|0x10);/* ILIM_USB 0x07,= 0x0-0x1D (100mA - 3000mA) */
        break;
    default:
        break;
    }
    return 0;
}

int pmic_init(unsigned char bus)
{
    //enable lcdc power ldo
    i2c_set_bus_num(1);
    i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0x10,0x4c);// DIS_OFF_PWRON_TIM bit 0; OFF_PRESS_PWRON 6s; OFF_JUDGE_PWRON bit 1; ON_PRESS_PWRON bit 2s
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0x39,0xc8);// dcdc4 output 3.1v for vccio
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0x50,0x30);// ldo5 output 1.8v for VCC18_LCD
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE,0x44,i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0x44)|(1<<4));//ldo5 enable

	return 0;
}

void shut_down(void)
{
    i2c_set_bus_num(1);
    i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE, 0xe0, i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0xe0) & 0xfe);
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE, 0x0f, i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0x0f) & 0xfe);   
    i2c_reg_write(CONFIG_SYS_I2C_SLAVE, 0x0e, i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0x0e) | 0x01);  
}

