/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
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
 *
 */
#ifndef __RK3036_GPIO_H
#define __RK3036_GPIO_H


/* gpio banks defined */
#define GPIO_BANK0		0
#define GPIO_BANK1		1
#define GPIO_BANK2		2


/* gpio pin define */
#define	GPIO_A0			0
#define	GPIO_A1			1
#define	GPIO_A2			2
#define	GPIO_A3			3
#define	GPIO_A4			4
#define	GPIO_A5			5
#define	GPIO_A6			6
#define	GPIO_A7			7
#define	GPIO_B0			8
#define	GPIO_B1			9
#define	GPIO_B2			10
#define	GPIO_B3			11
#define	GPIO_B4			12
#define	GPIO_B5			13
#define	GPIO_B6			14
#define	GPIO_B7			15
#define	GPIO_C0			16
#define	GPIO_C1			17
#define	GPIO_C2			18
#define	GPIO_C3			19
#define	GPIO_C4			20
#define	GPIO_C5			21
#define	GPIO_C6			22
#define	GPIO_C7			23
#define	GPIO_D0			24
#define	GPIO_D1			25
#define	GPIO_D2			26
#define	GPIO_D3			27
#define	GPIO_D4			28
#define	GPIO_D5			29
#define	GPIO_D6			30
#define	GPIO_D7			31


#define GPIO_BANKS		3
#define NUM_GROUP		32
#define MAX_GPIO_NUM		(NUM_GROUP * GPIO_BANKS)

/* if define irq, PIN_BASE start from NR_GIC_IRQS */
#ifdef CONFIG_USE_IRQ
	#define PIN_BASE	NR_GIC_IRQS
#else
	#define PIN_BASE	0
#endif /* CONFIG_USE_RKIRQ */

#define INVALID_GPIO        	-1


/* rk gpio banks data only for gpio driver module */
#ifdef RK_GPIO_DRIVER_MODULE
#define RK_GPIO_BANK(ID)			\
	{								\
		.regbase = (unsigned char __iomem *) RKIO_GPIO##ID##_PHYS, \
		.id = ID,	\
		.irq_base         = PIN_BASE + ID*NUM_GROUP,	\
		.ngpio            = NUM_GROUP,	\
	}

static struct rk_gpio_bank rk_gpio_banks[GPIO_BANKS] = {
	RK_GPIO_BANK(0),
	RK_GPIO_BANK(1),
	RK_GPIO_BANK(2)
};
#endif /* RK_GPIO_BANKS_DATA */

#endif	/* __RK3036_GPIO_H */

