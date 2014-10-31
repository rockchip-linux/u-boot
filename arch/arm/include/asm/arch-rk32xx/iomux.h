/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * rockchips iomux driver
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
#ifndef _ASM_ROCKCHIP_IOMUX_H_
#define _ASM_ROCKCHIP_IOMUX_H_


/* The clocks supported by the hardware */
enum iomux_id {
	RK_PWM0_IOMUX,
	RK_PWM1_IOMUX,
	RK_PWM2_IOMUX,
	RK_PWM3_IOMUX,
	RK_PWM4_IOMUX,
	RK_I2C0_IOMUX,
	RK_I2C1_IOMUX,
	RK_I2C2_IOMUX,
	RK_I2C3_IOMUX,
	RK_I2C4_IOMUX,
	RK_I2C5_IOMUX,
	RK_SPI0_CS0_IOMUX,
	RK_SPI0_CS1_IOMUX,
	RK_SPI1_CS0_IOMUX,
	RK_SPI1_CS1_IOMUX,
	RK_SPI2_CS0_IOMUX,
	RK_SPI2_CS1_IOMUX,
	RK_UART0_IOMUX,
	RK_UART1_IOMUX,
	RK_UART2_IOMUX,
	RK_UART3_IOMUX,
	RK_UART4_IOMUX,
	RK_LCDC0_IOMUX,
	RK_LCDC1_IOMUX,
	RK_EMMC_IOMUX,
	RK_SDCARD_IOMUX,
	RK_HDMI_IOMUX,
};


#define RK_UART_BT_IOMUX	RK_UART0_IOMUX
#define RK_UART_BB_IOMUX	RK_UART1_IOMUX
#define RK_UART_DBG_IOMUX	RK_UART2_IOMUX
#define RK_UART_GPS_IOMUX	RK_UART3_IOMUX
#define RK_UART_EXP_IOMUX	RK_UART4_IOMUX


void rk_iomux_config(int iomux_id);


#endif /* _ASM_ROCKCHIP_IOMUX_H_ */

