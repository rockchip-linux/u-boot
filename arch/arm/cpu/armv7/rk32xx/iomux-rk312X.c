/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * rk3288 iomux driver
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
#include <asm/arch/rkplat.h>

static void rk_pwm_iomux_config(int pwm_id)
{
	switch (pwm_id) {
		case RK_PWM0_IOMUX:
			grf_writel((1<<20)|(1<<4), GRF_GPIO0D_IOMUX);     
			break;
		case RK_PWM1_IOMUX:
			grf_writel((1<<22)|(1<<6), GRF_GPIO0D_IOMUX); 
			break;
		case RK_PWM2_IOMUX:
			grf_writel((1<<24)|(1<<8), GRF_GPIO0D_IOMUX); 
			break;
		default :
			debug("RK have not this pwm iomux id!\n");
			break;
	}
}

static void rk_i2c_iomux_config(int i2c_id)
{
	switch (i2c_id) {
		case RK_I2C0_IOMUX: 
			grf_writel((1<<18)|(1<<16)|(1<<2)|(1<<0), GRF_GPIO0A_IOMUX); 
			break;
		case RK_I2C1_IOMUX:
			grf_writel((3<<22)|(1<<20)|(1<<6)|(1<<4), GRF_GPIO0A_IOMUX);
			break;
		case RK_I2C2_IOMUX:
			grf_writel((7<<20)|(7<<16)|(3<<4)|(3<<0), GRF_GPIO2C_IOMUX2);
			break;
		case RK_I2C3_IOMUX:
			grf_writel((3<<30)|(3<<28)|(1<<14)|(1<<12), GRF_GPIO0A_IOMUX);
			break;
		default :
			debug("RK have not this i2c iomux id!\n");
			break;		  
	}
}

static void rk_lcdc_iomux_config(int lcd_id)
{
	switch (lcd_id) {
		case RK_LCDC0_IOMUX: 
			grf_writel(0x00550055, GRF_GPIO2B_IOMUX);  //lcdc0 iomux
			break;
		default :
			debug("RK have not this lcdc iomux id!\n");
		break;    
	}
}


static void rk_spi_iomux_config(int spi_id)
{
	switch (spi_id) {
		case RK_SPI0_CS0_IOMUX:
			grf_writel((3<<28)|(3<<26)|(3<<22)|(3<<18)|(2<<12)|(2<<10)|(2<<6)|(2<<2), GRF_GPIO0B_IOMUX); 
			break;
		default :
			debug("RK have not this spi iomux id!\n");
			break;
	}
}

static void rk_uart_iomux_config(int uart_id)
{
	switch (uart_id) {
		case RK_UART0_IOMUX:
			grf_writel((3<<22)|(3<<20)|(2<<6)|(2<<4), GRF_GPIO2D_IOMUX); 
			break;
		case RK_UART1_IOMUX:
			grf_writel((3<<20)|(3<<18)|(2<<4)|(2<<2), GRF_GPIO1B_IOMUX); 
			break;
		case RK_UART2_IOMUX:
			grf_writel((3<<22)|(3<<20)|(2<<6)|(2<<4), GRF_GPIO1C_IOMUX); 
			break;
		default:
			debug("RK have not this uart iomux id!\n");
			break;		 
	}
}

static void rk_emmc_iomux_config(int emmc_id)
{
	switch (emmc_id) {
		case RK_EMMC_IOMUX:
			// emmc data0-7
			grf_writel((0xFFFF << 16) | 0xAAAA, GRF_GPIO1D_IOMUX);
			// emmc emmc rstn out, pwren, emmc clkout
			// note: here no iomux emmc cmd for maskrom has do it for rk3126 or rk3128
			grf_writel((3<<18) | (3<<26) | (3<<30) | (2<<2) | (2<<10) | (2<<14), GRF_GPIO2A_IOMUX);
			break;
		default:
			debug("RK have not this emmc iomux id!\n");
			break;
	}
}
