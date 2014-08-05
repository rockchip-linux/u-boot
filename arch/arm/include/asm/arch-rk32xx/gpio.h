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
#ifndef __RKXX_GPIO_H
#define __RKXX_GPIO_H

#include "rkplat.h"

/* 定义GPIO相关寄存器偏移地址 */
#define GPIO_SWPORT_DR		0x00
#define GPIO_SWPORT_DDR		0x04
#define GPIO_INTEN		0x30
#define GPIO_INTMASK		0x34
#define GPIO_INTTYPE_LEVEL	0x38
#define GPIO_INT_POLARITY	0x3c
#define GPIO_INT_STATUS		0x40
#define GPIO_INT_RAWSTATUS	0x44
#define GPIO_DEBOUNCE		0x48
#define GPIO_PORTS_EOI		0x4c
#define GPIO_EXT_PORT		0x50
#define GPIO_LS_SYNC		0x60


/*
 * This is Linux-specific flags. By default controllers' and Linux' mapping
 * match, but GPIO controllers are free to translate their own flags to
 * Linux-specific in their .xlate callback. Though, 1:1 mapping is recommended.
 */
enum of_gpio_flags {
	OF_GPIO_ACTIVE_LOW = 0x1,
};


/* rk gpio bank */
struct rk_gpio_bank {
	void __iomem *regbase;
	int id;
	int irq_base;
	int ngpio;
};


/*
 * rk gpio api define the gpio format as:
 * using 32 bit for rk gpio value,
 * the high 16bit of gpio is bank id, the low 16bit of gpio is pin number
 * eg: gpio = 0x00010008, it mean gpio1_b0, 0x00010000 is bank id of GPIO_BANK1, 0x00000008 is GPIO_B0
 */

/* bank and pin bit mask */
#define RK_GPIO_BANK_MASK	0xFFFF0000
#define RK_GPIO_BANK_OFFSET	16
#define RK_GPIO_PIN_MASK	0x0000FFFF
#define RK_GPIO_PIN_OFFSET	0


/* gpio pin defined */
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	#include "gpio-rk3288.h"
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3036)
	#include "gpio-rk3036.h"
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3126) || (CONFIG_RKCHIPTYPE == CONFIG_RK3128)
	#include "gpio-rk312X.h"
#else
	#error "PLS config gpio-rkxx.h!"
#endif


#define RK_GPIO_BANK(gpio)		((gpio & RK_GPIO_BANK_MASK) >> RK_GPIO_BANK_OFFSET)
#define RK_GPIO_PIN(gpio)		((gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET)
#define RK_GPIO_BANK_VALID(gpio)	(RK_GPIO_BANK(gpio) < GPIO_BANKS)
#define RK_GPIO_PIN_VALID(gpio)		(RK_GPIO_PIN(gpio) < NUM_GROUP)


typedef enum eGPIOPinLevel {
	GPIO_LOW=0,
	GPIO_HIGH
} eGPIOPinLevel_t;

typedef enum eGPIOPinDirection {
	GPIO_IN=0,
	GPIO_OUT
} eGPIOPinDirection_t;

typedef enum GPIOPullType {
	PullDisable = 0,
	PullEnable = 1,
	GPIONormal = PullDisable,
	GPIOPullUp = 1,
	GPIOPullDown = 2
} eGPIOPullType_t;

typedef enum GPIODriveSlector {
	GPIODrv2mA = 0,
	GPIODrv4mA,
	GPIODrv8mA,
	GPIODrv12mA,
} eGPIODriveSlector_t;


static inline bool gpio_is_valid(int gpio)
{
	if ((gpio == INVALID_GPIO) || !RK_GPIO_BANK_VALID(gpio) || !RK_GPIO_PIN_VALID(gpio)) {
		debug("gpio = 0x%x is not valid!\n", gpio);
		return 0;
	}

	return 1;
}


#ifdef CONFIG_USE_IRQ
int rk_gpio_gpio_to_irq(unsigned gpio);
int rk_gpio_irq_to_gpio(unsigned irq);

static inline int gpio_to_irq(unsigned gpio)
{
	if (gpio_is_valid(gpio) == 0) {
		return INVALID_GPIO;
	}

	return rk_gpio_gpio_to_irq(gpio);
}

static inline int irq_to_gpio(unsigned irq)
{
	return rk_gpio_irq_to_gpio(irq);
}
#endif



static inline void rk_gpio_bit_op(void __iomem *regbase, unsigned int offset, uint32 bit, unsigned char flag)
{
	u32 val = __raw_readl(regbase + offset);

	if (flag)
		val |= bit;
	else
		val &= ~bit;

	__raw_writel(val, regbase + offset);
}

static inline unsigned pin_to_bit(unsigned pin)
{
	return 1u << pin;
}

static inline unsigned offset_to_bit(unsigned offset)
{
	return 1u << offset;
}


struct rk_gpio_bank *rk_gpio_get_bank(unsigned gpio);
struct rk_gpio_bank *rk_gpio_id_to_bank(unsigned int id);
int rk_gpio_base_to_bank(unsigned base);

int gpio_pull_updown(unsigned gpio, enum GPIOPullType type);
int gpio_drive_slector(unsigned gpio, enum GPIODriveSlector slector);
int gpio_set_value(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
int gpio_direction_input(unsigned gpio);

#endif	/* __RKXX_GPIO_H */

