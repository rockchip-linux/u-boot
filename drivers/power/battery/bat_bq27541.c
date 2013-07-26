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
#include <power/battery.h>
#include <power/max8997_pmic.h>
#include <errno.h>

int get_power_bat_status(struct battery *batt_status)
{
    int i2c_buf[2];
    int read_buffer = 0;
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
    
    //batt_status->state_of_chrg = !GetPortState(&charger_state);  //gpio0_b2, charger in status

	return 0;
}
