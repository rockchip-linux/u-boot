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
			grf_writel((3<<16)|1, GRF_GPIO7A_IOMUX);
			break;
		case RK_PWM1_IOMUX:
			grf_writel((1<<18)|(1<<2), GRF_GPIO7A_IOMUX);
			break;
		case RK_PWM2_IOMUX:
			grf_writel((1<<24)|(1<<8), GRF_GPIO7CH_IOMUX);
			break;
		case RK_PWM3_IOMUX:
			grf_writel((1<<16)|1, GRF_GPIO7CH_IOMUX);
			break;
		default :
			debug("pwm id = %d iomux error!\n", pwm_id);
			break;
	}
}

static void rk_i2c_iomux_config(int i2c_id)
{
	switch (i2c_id) {
		case RK_I2C0_IOMUX:
			pmu_writel(pmu_readl(0x88)|(1<<14), 0x88);
			pmu_writel(pmu_readl(0x8c)|1, 0x8c);
			break;
		case RK_I2C1_IOMUX:
			grf_writel((1<<26)|(1<<24)|(1<<10)|(1<<8), GRF_GPIO8A_IOMUX);
			break;
		case RK_I2C2_IOMUX:
			grf_writel((1<<20)|(1<<18)|(1<<4)|(1<<2), GRF_GPIO6B_IOMUX);
			break;
		case RK_I2C3_IOMUX:
			grf_writel((1<<20)|(1<<18)|(1<<2)|1, GRF_GPIO2C_IOMUX);
			break;
		case RK_I2C4_IOMUX:
			grf_writel((1<<28)|(1<<26)|(1<<12)|(1<<10), GRF_GPIO1D_IOMUX);
			break;
		case RK_I2C5_IOMUX:
			grf_writel((3<<28)|(1<<12), GRF_GPIO7CL_IOMUX);
			grf_writel((3<<16)| 1, GRF_GPIO7CH_IOMUX);
			break;
		default :
			debug("i2c id = %d iomux error!\n", i2c_id);
			break;
	}
}

static void rk_lcdc_iomux_config(int lcd_id)
{
	switch (lcd_id) {
		case RK_LCDC0_IOMUX:
			grf_writel(0x00550055, GRF_GPIO1D_IOMUX);  //lcdc0 iomux
			break;
		default :
			debug("lcdc id = %d iomux error!\n", lcd_id);
			break;
	}
}

static void rk_spi_iomux_config(int spi_id)
{
	switch (spi_id) {
		case RK_SPI0_CS0_IOMUX:
			grf_writel((((0x3<<14)|(0x3<<12)|(0x3<<10)|(0x3<<8))<<16)|(0x1<<14)|(0x1<<12)|(0x1<<10)|(0x1<<8), GRF_GPIO5B_IOMUX);
			break;
		case RK_SPI0_CS1_IOMUX:
			grf_writel((((0x3<<14)|(0x3<<12)|(0x3<<8))<<16)|(0x1<<14)|(0x1<<12)|(0x1<<8), GRF_GPIO5B_IOMUX);
			grf_writel(((0x3)<<16)|(0x1), GRF_GPIO5C_IOMUX);
			break;
		case RK_SPI1_CS0_IOMUX:
			grf_writel((((0x3<<14)|(0x3<<12)|((0x3<<10))|(0x3<<8))<<16)|(0x2<<14)|(0x2<<12)|((0x2<<10))|(0x2<<8), GRF_GPIO7B_IOMUX);
			break;
		case RK_SPI1_CS1_IOMUX:
			debug("rkspi: bus=1 cs=1 not support");
			break;
		case RK_SPI2_CS0_IOMUX:
			grf_writel(((0xf<<12)<<16) | (0x5<<12), GRF_GPIO8A_IOMUX);
			grf_writel((((0x3<<2)|(0x3))<<16)|(0x1<<2)|(0x1), GRF_GPIO8B_IOMUX);
			break;
		case RK_SPI2_CS1_IOMUX:
			grf_writel((((0x3<<12)|(0x3<<6))<<16)|(0x1<<12)|(0x1<<6), GRF_GPIO8A_IOMUX);
			grf_writel((((0x3<<2)|(0x3))<<16)|(0x1<<2)|(0x1), GRF_GPIO8B_IOMUX);
			break;
		default :
			debug("spi id = %d iomux error!\n", spi_id);
			break;
	}
}

static void rk_uart_iomux_config(int uart_id)
{
	switch (uart_id) {
		case RK_UART_BT_IOMUX:
			grf_writel((0x55<<16)|0x55, GRF_GPIO4C_IOMUX);
			break;
		case RK_UART_BB_IOMUX:
			grf_writel((0xff<<16)|0x55, GRF_GPIO5B_IOMUX);
			break;
		case RK_UART_DBG_IOMUX:
			grf_writel((3<<28)|(3<<24)|(1<<12)|(1<<8), GRF_GPIO7CH_IOMUX);
			break;
		case RK_UART_GPS_IOMUX:
			grf_writel((0xff<<16)|0x55, GRF_GPIO7B_IOMUX);
			break;
		case RK_UART_EXP_IOMUX:
			grf_writel((0xff<<24)|(0xff<<8), GRF_GPIO5B_IOMUX);
			break;
		default:
			debug("uart id = %d iomux error!\n", uart_id);
			break;
	}
}

static void rk_emmc_iomux_config(int emmc_id)
{
	switch (emmc_id) {
		case RK_EMMC_IOMUX:
			grf_writel((0xFFFF << 16) | 0xAAAA, GRF_GPIO3A_IOMUX); // emmc data0-7
			grf_writel((0x000C << 16) | 0x0008, GRF_GPIO3B_IOMUX); // emmc pwren
			grf_writel((0x003F << 16) | 0x002A, GRF_GPIO3C_IOMUX); // emmc cmd, emmc rstn out, emmc clkout
			break;
		default:
			debug("emmc id = %d iomux error!\n", emmc_id);
			break;
	}
}

static void rk_sdcard_iomux_config(int sdcard_id)
{
	switch (sdcard_id) {
		case RK_SDCARD_IOMUX:
			// sdcard dectn, cmd, clkout, d0 - d3
			grf_writel((0x1FFFF << 16) | 0x15555, GRF_GPIO6A_IOMUX);
			break;
		default:
			debug("sdcard id = %d iomux error!\n", sdcard_id);
			break;

	}
}

static void rk_hdmi_iomux_config(int hdmi_id)
{
	switch (hdmi_id) {
		case RK_HDMI_IOMUX:
			//i2c_hdmi_sda GRF_GPIO7CL_IOMUX[13:12]=10
			grf_writel((0x2 << 12) | ((0x3 << 12) << 16), GRF_GPIO7CL_IOMUX);
			//i2c_hdmi_scl GRF_GPIO7CH_IOMUX[1:0]=10
			grf_writel((0x2 << 0) | ((0x3 << 0) << 16), GRF_GPIO7CH_IOMUX);
			break;
		default:
			debug("hdmi id = %d iomux error!\n", hdmi_id);
			break;
	}
}
