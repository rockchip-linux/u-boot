/*
 *  Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *  zyw <zyw@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/battery.h>
#include <power/max8997_pmic.h>
#include <errno.h>

int state_of_chrg = 0;
#define TEMP_K              2731
#define MIN_CHARGE_TEMPERATURE       0
#define MAX_CHARGE_TEMPERATURE       450
/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/
int get_power_bat_status(struct battery *batt_status)
{
    int i2c_buf[2];
    int read_buffer = 0;
    int temperature = 280;
    i2c_set_bus_num(0);
    i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_BQ27541_I2C_ADDR);
    i2c_set_bus_speed(CONFIG_SYS_I2C_SPEED);

    i2c_buf[0] = i2c_reg_read(CONFIG_BQ27541_I2C_ADDR,0x08);// BQ27x00_REG_VOLT
    i2c_buf[1] = i2c_reg_read(CONFIG_BQ27541_I2C_ADDR,0x09);// BQ27x00_REG_VOLT+1  
    read_buffer = (i2c_buf[1] << 8) | i2c_buf[0];
    batt_status->voltage_uV = read_buffer*1000;

    i2c_buf[0] = i2c_reg_read(CONFIG_BQ27541_I2C_ADDR,0x2c);// BQ27500_REG_SOC
    i2c_buf[1] = i2c_reg_read(CONFIG_BQ27541_I2C_ADDR,0x2d);// BQ27500_REG_SOC+1  

    read_buffer = (i2c_buf[1] << 8) | i2c_buf[0];

    if(read_buffer==0 && batt_status->voltage_uV>3400000)
    batt_status->capacity=1;
    else if(read_buffer>=2) batt_status->capacity = (read_buffer-2)*100/98;
    else batt_status->capacity = 0;

    i2c_buf[0] = i2c_reg_read(CONFIG_BQ27541_I2C_ADDR,0x06);// BQ27x00_REG_TEMP
    i2c_buf[1] = i2c_reg_read(CONFIG_BQ27541_I2C_ADDR,0x07);// BQ27x00_REG_TEMP+1  
   
    temperature = ((i2c_buf[1] << 8) | i2c_buf[0]) - TEMP_K;

    if(!state_of_chrg)state_of_chrg = dwc_otg_check_dpdm();

    if((temperature<MIN_CHARGE_TEMPERATURE)||(temperature>MAX_CHARGE_TEMPERATURE))   //over temperature
    {
        batt_status->state_of_chrg = 0;
        printf("%s, over temperature = %d, usb type=%d\n ",__func__, temperature, state_of_chrg);
    }
    else batt_status->state_of_chrg = state_of_chrg;
    
	return 0;
}
