/*
 * (C) Copyright 2008-2013 Rockchip Electronics
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

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066) || (CONFIG_RKCHIPTYPE == CONFIG_RK3168)

/* Cpu clock source select */
#define CPU_SRC_ARM_PLL			0
#define CPU_SRC_GENERAL_PLL		1

/* Periph clock source select */
#define PERIPH_SRC_GENERAL_PLL		0
#define PERIPH_SRC_CODEC_PLL		1

/* DDR clock source select */
#define DDR_SRC_DDR_PLL			0
#define DDR_SRC_GENERAL_PLL		1

#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)

/* Cpu clock source select */
#define CPU_SRC_ARM_PLL			0
#define CPU_SRC_GENERAL_PLL		1

/* Periph clock source select */
#define PERIPH_SRC_GENERAL_PLL		1
#define PERIPH_SRC_CODEC_PLL		0

/* DDR clock source select */
#define DDR_SRC_DDR_PLL			0
#define DDR_SRC_GENERAL_PLL		1

#endif



/* config cpu and general clock in MHZ */
#define KHZ				(1000)
#define MHZ				(1000*1000)


/*
 * rkplat clock set for arm and general pll
 */
void rkclk_set_pll(void);


#endif	/* _RKXX_CLOCK_H */

