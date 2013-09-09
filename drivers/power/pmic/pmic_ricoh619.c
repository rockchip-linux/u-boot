/*
 *  Copyright (C) 2012 rockchips
 *  zyw < zyw@rock-chips.com >
 *
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
    if(IReadLoaderFlag() == 0) {
        i2c_set_bus_num(1);
        i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
        i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);
        reg = i2c_reg_read(CONFIG_SYS_I2C_SLAVE,0x09);// read power on history
        printf("%s power on history %x\n",__func__,reg);
        if(reg == 0x04)
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
set charge current
0. disable charging  
1. usb charging, 500mA
2. ac adapter charging, 1.5A
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

