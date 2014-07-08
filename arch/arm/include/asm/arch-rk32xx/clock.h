/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
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

#ifndef _RKXX_CLOCK_H
#define _RKXX_CLOCK_H

#include <asm/arch/typedef.h>


/* config cpu and general clock in MHZ */
#define KHZ				(1000)
#define MHZ				(1000*1000)

/* define pll mode */
#define RKCLK_PLL_MODE_SLOW		0
#define RKCLK_PLL_MODE_NORMAL		1


/*
 * rkplat clock set pll mode
 */
void rkclk_pll_mode(int pll_id, int pll_mode);

/*
 * rkplat clock set for arm and general pll
 */
void rkclk_set_pll(void);

/*
 * rkplat clock get arm pll, general pll and so on
 */
void rkclk_get_pll(void);
int rkclk_get_arm_pll(void);
int rkclk_get_general_pll(void);
int rkclk_get_codec_pll(void);
int rkclk_get_ddr_pll(void);
int rkclk_get_new_pll(void);

/*
 * rkplat clock pll dump
 */
void rkclk_dump_pll(void);


/*
 * rkplat clock set codec pll
 */
void rkclk_set_cpll_rate(uint32 pll_hz);


/*
 * rkplat set sd clock src
 * 0: codec pll; 1: general pll; 2: 24M
 */
void rkclk_set_sdclk_src(uint32 sdid, uint32 src);

/*
 * rkplat set sd/sdmmc/emmc clock src
 */
unsigned int rkclk_get_sdclk_src_freq(uint32 sdid);

/*
 * rkplat set sd clock div, from source input
 */
int rkclk_set_sdclk_div(uint32 sdid, uint32 div);


/*
 * rkplat get PWM clock, PWM01 from pclk_cpu, PWM23 from pclk_periph
 */
unsigned int rkclk_get_pwm_clk(uint32 id);


/*
 * rkplat get I2C clock, I2c0 and i2c1 from pclk_cpu, I2c2 and i2c3 from pclk_periph
 */
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id);


/*
 * rkplat get spi clock, spi0 and spi1 from  pclk_periph
 */
unsigned int rkclk_get_spi_clk(uint32 spi_bus);


/*
 * rkplat lcdc aclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc aclk source pll select) : 0 - codec pll, 1 - general pll
 * div (lcdc aclk div from pll) : 0x00 - 0x1f
 */
int rkclk_lcdc_aclk_set(uint32 lcdc_id, uint32 pll_sel, uint32 div);


/*
 * rkplat lcdc dclk config
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc dclk source pll select) : 0 - codec pll, 1 - general pll
 * div (lcdc dclk div from pll) : 0x00 - 0xff
 */
int rkclk_lcdc_dclk_set(uint32 lcdc_id, uint32 pll_sel, uint32 div);


/*
 * rkplat lcdc dclk and aclk parent pll source
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * dclk_hz: dclk rate
 * return dclk rate
 */
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz);


/*
 * rkplat pll select by clock
 * clock: device request freq HZ
 * return value:
 * high 16bit: 0 - codec pll, 1 - general pll
 * low 16bit : div
 */
uint32 rkclk_select_pll_source(uint32 clock, uint32 even);


#endif	/* _RKXX_CLOCK_H */

