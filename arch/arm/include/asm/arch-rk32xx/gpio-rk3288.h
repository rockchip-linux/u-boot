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
#ifndef __RK3288_GPIO_H
#define __RK3288_GPIO_H


/* gpio banks defined */
#define GPIO_BANK0		(0 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK1		(1 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK2		(2 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK3		(3 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK4		(4 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK5		(5 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK6		(6 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK7		(7 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK8		(8 << RK_GPIO_BANK_OFFSET)

/* gpio max banks and group defined */
#define GPIO_BANKS		9
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
	RK_GPIO_BANK(2),
	RK_GPIO_BANK(3),
	RK_GPIO_BANK(4),
	RK_GPIO_BANK(5),
	RK_GPIO_BANK(6),
	RK_GPIO_BANK(7),
	RK_GPIO_BANK(8)
};
#endif /* RK_GPIO_BANKS_DATA */

#endif	/* __RK3288_GPIO_H */

