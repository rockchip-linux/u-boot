/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK312X_GPIO_H
#define __RK312X_GPIO_H


/* gpio banks defined */
#define GPIO_BANK0		(0 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK1		(1 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK2		(2 << RK_GPIO_BANK_OFFSET)
#define GPIO_BANK3		(3 << RK_GPIO_BANK_OFFSET)

/* gpio max banks and group defined */
#define GPIO_BANKS		4
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
#define RK_GPIO_BANK_REGISTER(ID)			\
	{								\
		.regbase = (unsigned char __iomem *) RKIO_GPIO##ID##_PHYS, \
		.id = ID,	\
		.irq_base         = PIN_BASE + ID*NUM_GROUP,	\
		.ngpio            = NUM_GROUP,	\
	}

static struct rk_gpio_bank rk_gpio_banks[GPIO_BANKS] = {
	RK_GPIO_BANK_REGISTER(0),
	RK_GPIO_BANK_REGISTER(1),
	RK_GPIO_BANK_REGISTER(2),
	RK_GPIO_BANK_REGISTER(3)
};
#endif /* RK_GPIO_BANKS_DATA */

#endif	/* __RK312X_GPIO_H */
